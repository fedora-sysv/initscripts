#!/usr/bin/python
#
#  Red Hat Enterprise Linux support level checker
#

# testing checklist:
#   i386 intel uni           {classdump, check}
#   i386 AMD   uni           {classdump, check}
#   i386 intel smp noht      {classdump, check}
#   i386 intel smp ht bigmem {classdump, check}
#   ia64   uni               {classdump, check}
#   ia64   smp               {classdump, check}
#   x86_64 uni               {classdump, check}
#   x86_64 smp               {classdump, check}

import re
import sys
import time
import os
import exceptions
from syslog import syslog, openlog

TRUE, FALSE = (1, 0)

# exception

class ArchException(exceptions.Exception):
    def __init__(self, args):
        self.args = args
# end

    
class CPU:

    # class CPU is an abstract interface which spackles over the
    #  differences between the various architectures as seen by
    #  outside callers.  There are Arch_* classes which implement
    #  the specifics.  Each time a CPU is instantiated, it instantiates
    #  an Arch_* instance of the appropriate architecture.  Subsequent
    #  calls on CPU methods are delegated to this instance.

    class Arch:
        def __init__(self, ref, procnum):
            self.container = ref
            self.processor = procnum
            self.smp = FALSE

        def getPhysicalID(self):
            """
            default case: no hyperthreading in Arch
            physical ID is the processor number
            """
            return self.processor

        def physicalCPUCount(self, cpus):
            """
            default case: no hyperthreading in Arch
            CPU count is the number of processors
            """
            return len(cpus.keys())
        
    # end class Arch


    class Arch_i386(Arch):
        def __init__(self, ref, procnum):
            CPU.Arch.__init__(self, ref, procnum)
            self.athlonBlankFlags = 0
            return
        
        def getPhysicalID(self):
            return self.physicalID

        def physicalCPUcount(self, cpus):
            """
            count in the presence of hyperthreading
            i386 has a physical CPU id which is identical for
            all ht cpu's sharing a physical package.  Add up
            the unique physical packages
            """            
            uniqPhysIDs = {}
            for cpu in cpus.keys():
                uniqPhysIDs[cpus[cpu].physicalID()] = 1

            return len(uniqPhysIDs.keys())


        def __repr__(self):
            f = "%-16s: %s\n"
            r = "[i386]\n"
            r = r + f % ("processor", self.processor)
            r = r + f % ("vendor_id", self.vendor)
            r = r + f % ("cpu family", self.family)
            r = r + f % ("model", self.model)
            r = r + f % ("model name", self.modelName)
            r = r + f % ("stepping", self.stepping)
            r = r + f % ("cpu MHz", self.cpuMHz)
            r = r + f % ("cache size", self.cacheSize + " KB")

            try:
                r = r + f % ("physical id", self.physicalID)
                r = r + f % ("siblings", self.siblings)
            except: pass

            r = r + f % ("fdiv_bug", self.fdivBug)
            r = r + f % ("hlt_bug", self.hltBug)
            r = r + f % ("f00f_bug", self.f00fBug)
            r = r + f % ("coma_bug", self.comaBug)
            r = r + f % ("fpu", self.fpu)
            r = r + f % ("fpu_exception", self.fpuException)
            r = r + f % ("cpuid level", self.cpuIDLevel)
            r = r + f % ("wp", self.wp)
            r = r + f % ("flags", self.flags)

            try:
                r = r + f % ("runqueue", self.athlonRunQueue)
            except: pass
            
            r = r + f % ("bogomips", self.bogoMips)
            return r

        def parseLine( self, s ):
            """
            i386 specific line parser for /proc/cpuinfo
            """
            m = re.match( r'^vendor_id\s+:\s+(.*)\n', s )
            if m:
                self.vendor = m.group(1)
                return

            m = re.match( r'^arch\s+:\s+(.*)\n', s )
            if m:
                self.arch = m.group(1)
                return
            
            m = re.match( r'^cpu family\s+:\s+(.*)', s )
            if m:
                self.family =  m.group(1)
                return

            m = re.match( r'^model\s+:\s+(\d).*', s )
            if m:
                self.model = m.group(1)
                return

            m = re.match( r'^model name\s+:\s+(.*)\n', s )
            if m:
                self.modelName =  m.group(1)
                return

            m = re.match( r'^revision\s+:\s+(\d+)', s )
            if m:
                self.revision = m.group(1)
                return

            m = re.match( r'^stepping\s+:\s+(\d+)', s )
            if m:
                self.stepping = m.group(1)
                return

            m = re.match( r'^cpu MHz\s+:\s+(\d+\.\d+)', s )
            if m:
                self.cpuMHz = m.group(1)
                return
            
            m = re.match( r'^cache size\s+:\s+(\d+)\s+(\S+).*\n', s )
            if m:
                self.cacheSize = asKB( m.group(1), m.group(2) )
                return

            m = re.match( r'^physical id\s+:\s+(\d)', s )
            if m:
                self.physicalID =  m.group(1)
                self.smp = 1
                return

            m = re.match( r'^siblings\s+:\s(\d+)', s )
            if m:
                self.siblings = m.group(1)
                self.smp = 1
                return

            m = re.match( r'^fdiv_bug\s+:\s+(yes|no).*\n', s )
            if m:
                self.fdivBug =  m.group(1)
                return

            m = re.match( r'^hlt_bug\s+:\s+(yes|no).*\n', s )
            if m:
                self.hltBug = m.group(1)
                return

            m = re.match( r'^f00f_bug\s+:\s+(yes|no).*\n', s )
            if m:
                self.f00fBug = m.group(1)
                return

            m = re.match( r'^coma_bug\s+:\s+(yes|no).*\n', s )
            if m:
                self.comaBug = m.group(1)
                return

            m = re.match( r'^fpu\s+:\s+(yes|no).*\n', s )
            if m:
                self.fpu = m.group(1)
                return

            m = re.match( r'^fpu_exception\s+:\s+(yes|no).*\n', s )
            if m:
                self.fpuException = m.group(1)
                return

            m = re.match( r'^cpuid level\s+:\s+(\d+)', s )
            if m:
                self.cpuIDLevel = m.group(1)
                return

            m = re.match( r'^wp\s+:\s+(yes|no).*\n', s )
            if m:
                self.wp = m.group(1)
                return

            m = re.match( r'^flags\s+:\s+(.*)\n', s )
            if m:
                self.flags = m.group(1)
                return

            m = re.match( r'^flags\s+:\s*\n', s )
            if m:
                self.athlonBlankFlags = 1
                return
            
            m = re.match( r'^runqueue\s+:\s+(.*)\n', s )
            if m:
                self.athlonRunQueue = m.group(1)
                self.athlonBlankFlags = self.athlonBlankFlags + 1
                return

            if self.athlonBlankFlags == 2:
                # this is a hack for garblage on athlon
                m = re.match( r'^\s+(.*)', s )
                self.flags = m.group(1)
                self.athlonBlankFlags = 0
                return
            
            m = re.match( r'^bogomips\s+:\s+(\d+\.\d+)', s )
            if m:
                self.bogoMips = m.group(1)
                return
            
            print "Unmatched Line: %s" % s

        # end def parseLine
        
    # end class Arch_i386
            
    
    class Arch_ia64(Arch):
        def __init__(self, ref, procnum):
            # do I even need to have this if I'm merely delegating upwards?
            CPU.Arch.__init__(self, ref, procnum)
        
        def __repr__(self):
            f = "%-12s"
            r = "[ia64]\n"
            r = r + (f + ": %s\n") % ("processor", self.processor)
            r = r + (f + ": %s\n") % ("vendor", self.vendor)
            r = r + (f + ": %s\n") % ("arch", self.arch)
            r = r + (f + ": %s\n") % ("family", self.family)
            r = r + (f + ": %s\n") % ("model", self.model)
            r = r + (f + ": %s\n") % ("revision", self.revision)
            r = r + (f + ": %s\n") % ("archrev", self.archrev)
            r = r + (f + ": %s\n") % ("features", self.features)
            r = r + (f + ": %s\n") % ("cpu number", self.cpuNumber)
            r = r + (f + ": %s\n") % ("cpu regs", self.cpuRegs)
            r = r + (f + ": %s\n") % ("cpu MHz", self.cpuMHz)
            r = r + (f + ": %s\n") % ("itc MHz", self.itcMHz)
            r = r + (f + ": %s\n") % ("BogoMIPS", self.bogoMips)
            return r

        def parseLine( self, s ):
            
            m = re.match( r'^vendor\s+:\s+(.*)\n', s )
            if m:
                self.vendor = m.group(1)
                return

            m = re.match( r'^arch\s+:\s+(.*)\n', s )
            if m:
                self.arch = m.group(1)
                return
            
            m = re.match( r'^family\s+:\s+(.*)', s )
            if m:
                self.family =  m.group(1)
                return

            m = re.match( r'^model\s+:\s+(\d).*', s )
            if m:
                self.model = m.group(1)
                return

            m = re.match( r'^revision\s+:\s+(\d+)', s )
            if m:
                self.revision = m.group(1)
                return

            m = re.match( r'^archrev\s+:\s+(\d+)', s )
            if m:
                self.archrev = m.group(1)
                return

            m = re.match( r'^features\s+:\s+(.*)\n', s )
            if m:
                self.features =  m.group(1)
                return

            m = re.match( r'^cpu number\s+:\s+(\d)', s )
            if m:
                self.cpuNumber =  m.group(1)
                return

            m = re.match( r'^cpu regs\s+:\s+(\d)', s )
            if m:
                self.cpuRegs =  m.group(1)
                return

            m = re.match( r'^cpu MHz\s+:\s+(\d+\.\d+)', s )
            if m:
                self.cpuMHz = m.group(1)
                return

            m = re.match( r'^itc MHz\s+:\s+(\d+\.\d+)', s )
            if m:
                self.itcMHz = m.group(1)
                return
            
            m = re.match( r'^BogoMIPS\s+:\s+(\d+\.\d+)', s )
            if m:
                self.bogoMips = m.group(1)
                return
            
            print "Unmatched Line: %s" % s

        # end def parseLine
        
    # end class Arch_ia64

    class Arch_x86_64(Arch):
        def __init__(self, ref, procnum):
            CPU.Arch.__init__(self,ref,procnum)

        def __repr__(self):
            f = "%-16s"
            r = "[x84_64]\n"
            r = r + (f + ": %s\n") % ("processor", self.processor)
            r = r + (f + ": %s\n") % ("vendor_id", self.vendor)
            r = r + (f + ": %s\n") % ("cpu family", self.family)
            r = r + (f + ": %s\n") % ("model", self.model)
            r = r + (f + ": %s\n") % ("model name", self.modelName)
            r = r + (f + ": %s\n") % ("stepping", self.stepping)            
            r = r + (f + ": %s\n") % ("cpu MHz", self.cpuMHz)
            r = r + (f + ": %s KB\n") % ("cache size", self.cacheSize)
            r = r + (f + ": %s\n") % ("fpu", self.fpu)
            r = r + (f + ": %s\n") % ("fpu_exception", self.fpuException)
            r = r + (f + ": %s\n") % ("cpuid level", self.CPUIDLevel)
            r = r + (f + ": %s\n") % ("wp", self.wp)
            r = r + (f + ": %s\n") % ("flags", self.flags)

            r = r + (f + ": %s\n") % ("bogomips", self.bogoMips)
            r = r + (f + ": %s\n") % ("TLB size", self.tlbSize)
            r = r + (f + ": %s\n") % ("clflush size", self.clflushSize)
            r = r + (f + ": %s\n") % ("address sizes", self.addrSizes)
            r = r + (f + ": %s\n") % ("power management", self.powerMgmt)
            return r


        def parseLine( self, s ):
            
            m = re.match( r'^vendor_id\s+:\s+(.*)\n', s )
            if m:
                self.vendor = m.group(1)
                return

            m = re.match( r'^cpu family\s+:\s+(\d+)', s )
            if m:
                self.family =  m.group(1)
                return

            m = re.match( r'^model\s+:\s+(\d+)', s )
            if m:
                self.model = m.group(1)
                return

            m = re.match( r'^model name\s+:\s+(.*)\n', s )
            if m:
                self.modelName =  m.group(1)
                return

            m = re.match( r'^stepping\s+:\s+(\d+)', s )
            if m:
                self.stepping = m.group(1)
                return

            m = re.match( r'^cpu MHz\s+:\s+(\d+\.\d+)', s )
            if m:
                self.cpuMHz = m.group(1)
                return
            
            m = re.match( r'^cache size\s+:\s+(\d+)\s+(\S+).*\n', s )
            if m:
                self.cacheSize = asKB( m.group(1), m.group(2) )
                return

            m = re.match( r'^fpu\s+:\s+(yes|no).*\n', s )
            if m:
                self.fpu = m.group(1)
                return

            m = re.match( r'^fpu_exception\s+:\s+(yes|no).*\n', s )
            if m:
                self.fpuException = m.group(1)
                return

            m = re.match( r'^cpuid level\s+:\s+(\d+)', s )
            if m:
                self.CPUIDLevel = m.group(1)
                return

            m = re.match( r'^wp\s+:\s+(yes|no).*\n', s )
            if m:
                self.wp = m.group(1)
                return

            m = re.match( r'^flags\s+:\s+(.*)\n', s )
            if m:
                self.flags = m.group(1)
                return

            m = re.match( r'^bogomips\s+:\s+(\d+\.\d+)', s )
            if m:
                self.bogoMips = m.group(1)
                return

            m = re.match( r'TLB size\s+:\s+(\d+\s+\d+K pages)', s )
            if m:
                self.tlbSize = m.group(1)
                return
            
            m = re.match( r'clflush size\s+:\s+(\d+)', s )
            if m:
                self.clflushSize = m.group(1)
                return
            
            m = re.match( r'address sizes\s+:\s+(.*)\n', s )
            if m:
                self.addrSizes = m.group(1)
                return
            
            m = re.match( r'power management\s*:\s+(.*)\n', s )
            if m:
                self.powerMgmt = m.group(1)
                return
            
            print "Unmatched Line: %s" % s

        # end def parseLine
        
    # end class Arch_x86_64


    # this is the actual CPU class implementation
    def __init__(self, procnum, arch):
        self.procnum = procnum
        self.arch = arch
        
        if arch == "ia64":
            self.delegate = self.Arch_ia64(self, procnum)
        elif arch == "i386":
            self.delegate = self.Arch_i386(self, procnum)
        elif arch == "x86_64":
            self.delegate = self.Arch_x86_64(self, procnum)
        else:
            raise RuntimeError ("unknown arch %s" % arch)

    def physicalCPUcount(self, cpus):
        return self.delegate.physicalCPUcount( cpus )

    def physicalID(self):
        self.delegate.getPhysicalID()
    
    def __repr__(self):
        return self.delegate.__repr__()
        
    def parseLine( self, s ):
        self.delegate.parseLine(s)

# END Class CPU


# CPUInfo - visible class from this package
#  acts as a container for the existing cpus

class CPUInfo:

    def setArch(self):
        self.arch = os.uname()[4]

        if self.arch != "ia64" and self.arch != "x86_64":
            # must be an x86
            if self.arch[2:4] == "86":
                self.arch = "i386"
            else:
                raise ArchException( "architecture not interesting" )
    # end def


    def __init__(self):
        self.cpus = {}
        self.cpuCount = 0;
        self.physCPU  = 0;
        self.setArch()
        
        f = open("/proc/cpuinfo");
        l = f.readlines()
        f.close()
        
        for s in l:
            
            m = re.match( r'^$', s );
            if m: continue

            # we hit one of these at each new processor
            #
            m = re.match( r'^processor\s+:\s+(\d+).*$', s )
            if m:
                procnum = int(m.group(1))
                cpu = self.cpus[procnum] = CPU(procnum, self.arch)
                
                self.cpuCount = self.cpuCount + 1;
                continue

            if not self.cpuCount:
                raise RuntimeError, 'processor keyword not found'

            cpu.parseLine( s )

        # end for

     # end __init__
        
        
    def __repr__( self ):
        r = ""
        
        for key in self.cpus.keys():
            if r: r = r + "\n"
            r = r + self.cpus[key].__repr__()
            
        return r

    def physicalCPUcount(self):
        """ return number of physical CPUs """

        # easy stuff first
        if self.cpuCount == 1: return 1

        if self.physCPU != 0:
            # already computed
            return self.physCPU

        cpuKey = self.cpus.keys()[0]
        self.physCPU = self.cpus[cpuKey].physicalCPUcount(self.cpus)

        return self.physCPU
        
    # end def physicalCPUcount
    
# end class CPUInfo


class MemInfo:
    def __init__(self):
        f = open("/proc/meminfo")
        l = f.readlines()
        f.close()

        exp = r'\s+(\d+)\s+(\S+)'

        # yes.  It is inefficient.  But if the kernel reorders
        #  the item for any reason, we don't have to fix it.
        #
        for s in l[3:]:
            if re.match( r'^$', s ):
                continue
            
            m = re.match( r'^MemTotal:' + exp, s )
            if m:
                self.memTotal     = asKB( int(m.group(1)), m.group(2) )
                continue

            m = re.match( r'^MemFree:' + exp, s )
            if m:
                self.memFree = asKB( int( m.group(1) ), m.group(2) )
                continue
            
            m = re.match( r'^MemShared:' + exp, s )
            if m:
                self.memShared = asKB( int( m.group(1) ), m.group(2) )
                continue
            
            m = re.match( r'^Buffers:' + exp, s )
            if m:
                self.buffers = asKB( int( m.group(1) ), m.group(2) )
                continue
            
            m = re.match( r'^Cached:' + exp, s )
            if m:
                self.cached = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^SwapCached:' + exp, s )
            if m:
                self.swapCached = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'Active:' + exp, s )
            if m:
                self.active = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^ActiveAnon:' + exp, s )
            if m:
                self.activeAnon = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^ActiveCache:' + exp, s )
            if m:
                self.activeCache = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^Inact_dirty:' + exp, s )
            if m:
                self.inactDirty = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^Inact_laundry:' + exp, s )
            if m:
                self.inactLaundry = asKB( int( m.group(1) ), m.group(2) )
                continue
            
            m = re.match( r'^Inact_clean:' + exp, s )
            if m:
                self.inactClean = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^Inact_target:' + exp, s )
            if m:
                self.inactTarget = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^HighTotal:' + exp, s )
            if m:
                self.highTotal = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^HighFree:' + exp, s )
            if m:
                self.highFree = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^LowTotal:' + exp, s )
            if m:
                self.lowTotal = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^LowFree:' + exp, s )
            if m:
                self.lowFree = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^SwapTotal:' + exp, s )
            if m:
                self.swapTotal = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'^SwapFree:' + exp, s )
            if m:
                self.swapFree = asKB( int(m.group(1)), m.group(2) )
                continue

            m = re.match( r'Committed_AS:' + exp, s )
            if m:
                self.committedAS = asKB( int(m.group(1)), m.group(2) )
                continue
            
            m = re.match( r'HugePages_Total:\s+(\d+)', s )
            if m:
                self.hugePagesTotal = int(m.group(1))
                continue
            
            m = re.match( r'HugePages_Free:\s+(\d+)', s )
            if m:
                self.hugePagesFree = int(m.group(1))
                continue
            
            m = re.match( r'Hugepagesize:' + exp, s )
            if m:
                self.hugePageSize = asKB( int(m.group(1)), m.group(2) )
                continue
            
            print 'Unmatched meminfo property: ' + s

        # end for s in lines
                
    # end __init__

    def __repr__(self):
        """generate string representation for debugging purposes"""
        r = ""
        f = "%-14s%8d KB\n"
        
        r =     f % ("MemTotal:",      self.memTotal)
        r = r + f % ("MemFree:",       self.memFree)
        r = r + f % ("MemShared:",     self.memShared)
        r = r + f % ("Buffers:",       self.buffers)
        r = r + f % ("Cached:",        self.cached)
        r = r + f % ("SwapCached:",    self.swapCached)
        r = r + f % ("Active:",        self.active)

        try:
            r = r + f % ("ActiveAnon:",    self.activeAnon)
            r = r + f % ("ActiveCache:",   self.activeCache)
        except: pass

        r = r + f % ("Inact_dirty:",   self.inactDirty)

        try:
            r = r + f % ("Inact_laundry:", self.inactLaundry)
        except: pass
        
        r = r + f % ("Inact_clean:",   self.inactClean)
        r = r + f % ("Inact_target:",  self.inactTarget)
        r = r + f % ("HighTotal:",     self.highTotal)
        r = r + f % ("HighFree:",      self.highFree)
        r = r + f % ("LowTotal:",      self.lowTotal)
        r = r + f % ("LowFree:",       self.lowFree)
        r = r + f % ("SwapTotal:",     self.swapTotal)
        r = r + f % ("SwapFree:",      self.swapFree)

        try:
            r = r + "%-14s%8d\n" % ("HugePages_Total:", self.hugePagesTotal)
            r = r + "%-14s%8d\n" % ("HugePages_Free:",  self.hugePagesFree)
            r = r + f % ("HugePagesize:",    self.hugePageSize)
        except: pass
        
        return r
    
    # end def __repr__


# end class MemInfo


def unitToVal( unit ):
    """ Map a unit string to a power of 2.
    Why a power of 2?
    It keeps us from generating big numbers that could overflow.
    """
    
    m = re.match( r'[kK][bB]', unit )
    if m: return 10

    m = re.match( r'[mM][bB]', unit )
    if m: return 20

    m = re.match( r'[gG][bB]', unit )
    if m: return 30

    raise RuntimError, "unknown unit type: " + unit

# end def unitToVal


def asUnit( val, inunit, outunit ):
    """ normalize val in inunit to units in outunit"""

    insize  = unitToVal(inunit)
    outsize = unitToVal(outunit)
    ratio   = insize - outsize

#    print "insize = %d\noutsize = %d\nratio = %d\n" % (insize, outsize, ratio)

    return val * (2 ** ratio)

# end def asUnit

    
def asKB( val, units ):
    """normalize val from units to KB"""

    return asUnit( val, units, "KB" )

# end def asKB


def asMB( val, units ):
    """normalize val from units to MB"""

    return asUnit( val, units, "MB" )

# end def asMB



supportinfo = "/var/lib/supportinfo"
defaults = ["ES", 262144, 4194304, 2]

class SupportLevel:
    """internalizes the support level file"""

    def __init__(self):


        try:
            f = open(supportinfo)
        except:
            print "supportinfo file is missing"
            syslog("supportinfo file is missing")
            raise
        
        lines = f.readlines()
        f.close()
        exp = r'\s*:\s*(\d+)\s*(\S+)'
        found = 0
        (self.variant, self.minRAM, self.maxRAM, self.maxCPU) = defaults
        
        for line in lines:
            m = re.match( r'(^\s*#|^\s*$)', line )
            if m: continue

            m = re.match( r'^\s*Variant\s*:\s*(\S+)\s*$', line )
            if m:
                self.setVariant( m.group(1) )
                found = found | 0x01
                continue
            
            m = re.match( r'^\s*MinRAM' + exp, line )
            if m:
                self.minRAM = asKB( int(m.group(1)), m.group(2) )
                found = found | 0x02
                continue

            m = re.match( r'^\s*MaxRAM' + exp, line )
            if m:
                self.maxRAM = asKB( int(m.group(1)), m.group(2) )
                found = found | 0x04
                continue

            m = re.match( r'^\s*MaxCPU\s*:\s*(\d+)', line )
            if m:
                self.maxCPU = int( m.group(1) )
                found = found | 0x08
                continue

            print "Unmatched in SupportLevel: " + line

        # end for line...
        if found != 0x0F:
            for item in ['Variant', 'MinRAM', 'MaxRAM', 'MaxCPU']:
                if not found & 0x01:
                    print "supportinfo tag %s is missing\n" % item
                found = found >> 1
                
    # end def __init__:

    def setVariant(self, var):
        if var in ['AS', 'ES', 'WS']:
            self.variant = var
        else:
            print """
%s: unknown Variant %s in support info file: assuming %s
            """ % (sys.argv[0], var, self.variant)

    # end setVariant
        
    def __repr__(self):
        f = '%-15s:%10d'

        s =     (f +    "\n") % ("Variant", self.variant)
        s = s + (f + " KB\n") % ("MinRAM",  self.minRAM)
        s = s + (f + " KB\n") % ("MaxRAM",  self.maxRAM)
        s = s + (f +    "\n") % ("MaxCPU",  self.maxCPU)
        return s
    
    # end __repr__

    def classDump(self):
        """ instantiate and print both MemInfo and CPUInfo instances """

        print """*** CPU INFO ***"""
        print CPUInfo()
        print """*** MEM INFO ***"""
        print MemInfo()

        
    # end classDump()

    
    def check(self):
        meminfo = MemInfo()

        try:   cpuinfo = CPUInfo()
        except ArchException: return 0
            
        notes = 0
        
        # print self

        msgtop = [
            "***",
            "*** WARNING: Red Hat Enterprise Linux %s" % self.variant
            ]
        msgbot = [
            "***    to run as a supported configuration",
            "***"
            ]
        
        if meminfo.memTotal < self.minRAM:
            message = msgtop + [
                "***    requires at least %d MB RAM" % asMB(self.minRAM,"KB")
                ] + msgbot


            for line in message:
                print line
                syslog(line)

            notes = notes + 1


        if self.maxRAM > 0 and meminfo.memTotal > self.maxRAM:
            message = msgtop + [
                "***    allows no more than %d MB RAM" % asMB(self.maxRAM,"KB")
                ] + msgbot

            for line in message:
                print line
                syslog(line)

            notes = notes + 1


        if self.maxCPU > 0 and cpuinfo.physicalCPUcount() > self.maxCPU:
            plural = ""
            if self.maxCPU > 1: plural = "s"
            
            message = msgtop + [
                "***    allows no more than %d physical CPU%s" %
                (self.maxCPU, plural)
                ] + msgbot

            for line in message:
                print line
                syslog(line)

            notes = notes + 1



        if notes:
            plural = ""
            if notes > 1: plural = "s"
            print "%d support check notification%s issued" % (notes, plural)
            
        return notes
   
# end class SupportLevel:

openlog( 'redhat-support-check' )

if len(sys.argv) > 1 and sys.argv[1] == "-d":
    sys.exit( SupportLevel().classDump() )
else:    
    sys.exit( SupportLevel().check() )
