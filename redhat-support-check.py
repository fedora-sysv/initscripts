#!/usr/bin/python
#
#  Red Hat Enterprise Linux support level checker
#

import re
import sys
import time
from syslog import syslog, openlog

true, false = (1, 0)

# CPUInfo - visible class from this package
#  acts as a container for the existing cpus

class CPUInfo:

    class CPU:
        def __init__(self, procnum):
            self.procnum = procnum
            self.smp = false
            
        def __repr__(self):
            out = ""
            out += "processor\t: %s\n"     % self.procnum
            out += "vendor\t\t: %s\n"      % self.vendor
            out += "cpu family\t: %s\n"    % self.family
            out += "model\t\t: %s\n"       % self.model
            out += "model name\t: %s\n"    % self.modelName
            out += "stepping\t: %s\n"      % self.stepping
            out += "cpu MHz\t\t: %s\n"     % self.MHz
            out += "cache size\t: %s %s\n" % (
                self.cacheSize, self.cacheUnit)
            if self.smp:
                out += "physical id\t: %s\n"   % self.physicalID
                out += "siblings\t: %s\n"      % self.siblings

            out += "fdiv_bug\t: %s\n"      % self.fdivBug
            out += "hlt_bug\t\t: %s\n"     % self.hltBug
            out += "f00f_bug\t: %s\n"      % self.f00fBug
            out += "coma_bug\t: %s\n"      % self.comaBug
            out += "fpu\t\t: %s\n"         % self.fpu
            out += "fpu_exception\t: %s\n" % self.fpuException
            out += "cpuid level\t: %s\n"   % self.CPUIDLevel
            out += "wp\t\t: %s\n"          % self.wp
            out += "flags\t\t: %s\n"       % self.flags
            out += "bogomips\t: %s\n"      % self.bogoMips
            return out
            
        # END Class CPU

    def __init__(self):
        self.cpus = {}
        self.ncpu = 0;
        self.physCPU = 0;
        
        f = open("/proc/cpuinfo");
        l = f.readlines()
        f.close()
        
        havecpu = 0

        for s in l:
            
            ms = re.match( r'^$', s );
            if ms: continue
            
            ms = re.match( r'^processor\s+:\s+(\d+).*$', s )
            if ms:
                procnum = int(ms.group(1))
                curcpu = self.cpus[procnum] = self.CPU(procnum)
                
                self.ncpu += 1;
                continue

            if not self.ncpu:
                raise RuntimeError, 'processor keyword not found'
            
            ms = re.match( r'^vendor_id\s+:\s+(.*)\n', s )
            if ms:
                curcpu.vendor = ms.group(1)
                continue
                
            ms = re.match( r'^cpu family\s+:\s+(\d+).*', s )
            if ms:
                curcpu.family =  ms.group(1)
                continue

            ms = re.match( r'^model\s+:\s+(\d).*', s )
            if ms:
                curcpu.model = ms.group(1)
                continue

            ms = re.match( r'^model name\s+:\s+(.*)\n', s )
            if ms:
                curcpu.modelName =  ms.group(1)
                continue

            ms = re.match( r'^stepping\s+:\s+(\d+)', s )
            if ms:
                curcpu.stepping = ms.group(1)
                continue

            ms = re.match( r'^cpu MHz\s+:\s+(\d+\.\d+)', s )
            if ms:
                curcpu.setMHz = ms.group(1)
                continue
            
            ms = re.match( r'^cache size\s+:\s+(\d+)\s+(\S+).*\n', s )
            if ms:
                curcpu.setCacheSize = asKB( ms.group(1), ms.group(2) )
                continue

            ms = re.match( r'^physical id\s+:\s+(\d)', s )
            if ms:
                curcpu.physicalID =  ms.group(1)
                curcpu.smp = 1
                continue

            ms = re.match( r'^siblings\s+:\s(\d+)', s )
            if ms:
                curcpu.siblings = ms.group(1)
                curcpu.smp = 1
                continue

            ms = re.match( r'^fdiv_bug\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.fdivBug =  ms.group(1)
                continue

            ms = re.match( r'^hlt_bug\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.hltBug = ms.group(1)
                continue

            ms = re.match( r'^f00f_bug\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.f00fBug = ms.group(1)
                continue

            ms = re.match( r'^coma_bug\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.comaBug = ms.group(1)
                continue

            ms = re.match( r'^fpu\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.fpu = ms.group(1)
                continue

            ms = re.match( r'^fpu_exception\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.fpuException = ms.group(1)
                continue

            ms = re.match( r'^cpuid level\s+:\s+(\d+)', s )
            if ms:
                curcpu.cpuIDLevel = ms.group(1)
                continue

            ms = re.match( r'^wp\s+:\s+(yes|no).*\n', s )
            if ms:
                curcpu.wp = ms.group(1)
                continue

            ms = re.match( r'^flags\s+:\s+(.*)\n', s )
            if ms:
                curcpu.flags = ms.group(1)
                continue

            ms = re.match( r'^bogomips\s+:\s+(\d+\.\d+)', s )
            if ms:
                curcpu.bogoMips = ms.group(1)
                continue
            
            print "Unmatched Line: %s" % s

        # end __init__
        
        
    def __repr__( self ):
        out = ""
        
        for key in self.cpus.keys():
            if out: out += "\n"
            curcpu = self.cpus[key]
            out += curcpu.__repr__()
            
        return out

    def physicalCPUcount(self):
        """ return number of physical CPUs """

        # easy stuff first
        if self.ncpu == 1: return 1

        if self.physCPU != 0:
            # already computed
            return self.physCPU
        
        self.physCPU = 0

        # this is currently coded for PII and greater i386
        #  CPUs only...  we're going to count up the number
        #  of unique physical ID's

        uniqPhysIDs = {}
        for cpu in self.cpus.keys():
            uniqPhysIDs[self.cpus[cpu].physicalID] = 1

        self.physCPU = len(uniqPhysIDs.keys())

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
        #  the items for any reason, we don't have to fix it.
        #
        for s in l[3:]:
            if re.match( r'^$', s ):
                continue
            
            ms = re.match( r'^MemTotal:' + exp, s )
            if ms:
                self.memTotal     = asKB( int(ms.group(1)), ms.group(2) )
                continue

            ms = re.match( r'^MemFree:' + exp, s )
            if ms:
                self.memFree = asKB( int( ms.group(1) ), ms.group(2) )
                continue
            
            ms = re.match( r'^MemShared:' + exp, s )
            if ms:
                self.memShared = asKB( int( ms.group(1) ), ms.group(2) )
                continue
            
            ms = re.match( r'^Buffers:' + exp, s )
            if ms:
                self.buffers = asKB( int( ms.group(1) ), ms.group(2) )
                continue
            
            ms = re.match( r'^Cached:' + exp, s )
            if ms:
                self.cached = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^SwapCached:' + exp, s )
            if ms:
                self.swapCached = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'Active:' + exp, s )
            if ms:
                self.active = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^ActiveAnon:' + exp, s )
            if ms:
                self.activeAnon = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^ActiveCache:' + exp, s )
            if ms:
                self.activeCache = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^Inact_dirty:' + exp, s )
            if ms:
                self.inactDirty = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^Inact_laundry:' + exp, s )
            if ms:
                self.inactLaundry = asKB( int( ms.group(1) ), ms.group(2) )
                continue
            
            ms = re.match( r'^Inact_clean:' + exp, s )
            if ms:
                self.inactClean = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^Inact_target:' + exp, s )
            if ms:
                self.inactTarget = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^HighTotal:' + exp, s )
            if ms:
                self.highTotal = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^HighFree:' + exp, s )
            if ms:
                self.highFree = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^LowTotal:' + exp, s )
            if ms:
                self.lowTotal = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^LowFree:' + exp, s )
            if ms:
                self.lowFree = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^SwapTotal:' + exp, s )
            if ms:
                self.swapTotal = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            ms = re.match( r'^SwapFree:' + exp, s )
            if ms:
                self.swapFree = asKB( int(ms.group(1)), ms.group(2) )
                continue
            
            print 'Unmatched meminfo property: ' + s

        # end for s in lines
                
    # end __init__


    def __repr__(self):
        """generate string representation for debugging purposes"""
        r = ""
        f = "%-14s%8d KB\n"
        
        r =  f % ("MemTotal:",      self.memTotal)
        r += f % ("MemFree:",       self.memFree)
        r += f % ("MemShared:",     self.memShared)
        r += f % ("Buffers:",       self.buffers)
        r += f % ("Cached:",        self.cached)
        r += f % ("SwapCached:",    self.swapCached)
        r += f % ("Active:",        self.active)
        r += f % ("ActiveAnon:",    self.activeAnon)
        r += f % ("ActiveCache:",   self.activeCache)
        r += f % ("Inact_dirty:",   self.inactDirty)
        r += f % ("Inact_laundry:", self.inactLaundry)
        r += f % ("Inact_clean:",   self.inactClean)
        r += f % ("Inact_target:",  self.inactTarget)
        r += f % ("HighTotal:",     self.highTotal)
        r += f % ("HighFree:",      self.highFree)
        r += f % ("LowTotal:",      self.lowTotal)
        r += f % ("LowFree:",       self.lowFree)
        r += f % ("SwapTotal:",     self.swapTotal)
        r += f % ("SwapFree:",      self.swapFree)
        return r
    
    # end def __repr__

# end class MemInfo


def unitToVal( unit ):
    """ Map a unit string to a power of 2.
    Why a power of 2?
    It keeps us from generating big numbers that could overflow.
    """
    
    ms = re.match( r'[kK][bB]', unit )
    if ms: return 10

    ms = re.match( r'[mM][bB]', unit )
    if ms: return 20

    ms = re.match( r'[gG][bB]', unit )
    if ms: return 30

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
        f = open(supportinfo)
        lines = f.readlines()
        f.close()
        exp = r'\s*:\s*(\d+)\s*(\S+)'
        found = 0
        (self.variant, self.minRAM, self.maxRAM, self.maxCPU) = defaults
        
        for line in lines:
            ms = re.match( r'(^\s*#|^\s*$)', line )
            if ms: continue

            ms = re.match( r'^\s*Variant\s*:\s*(\S+)\s*$', line )
            if ms:
                self.variant = ms.group(1)
                found |= 0x01
                continue
            
            ms = re.match( r'^\s*MinRAM' + exp, line )
            if ms:
                self.minRAM = asKB( int(ms.group(1)), ms.group(2) )
                found |= 0x02
                continue

            ms = re.match( r'^\s*MaxRAM' + exp, line )
            if ms:
                self.maxRAM = asKB( int(ms.group(1)), ms.group(2) )
                found |= 0x04
                continue

            ms = re.match( r'^\s*MaxCPU\s*:\s*(\d+)', line )
            if ms:
                self.maxCPU = int( ms.group(1) )
                found |= 0x08
                continue

            print "Unmatched in SupportLevel: " + line

        # end for line...
        if found != 0x0F:
            for item in ['Variant', 'MinRAM', 'MaxRAM', 'MaxCPU']:
                if not found & 0x01:
                    print "supportinfo tag %s is missing\n" % item
                found = found >> 1
                
    # end def __init__:

    def __repr__(self):
        f = '%-15s:%10d'

        s =  (f + " KB\n") % ("MinRAM", self.minRAM)
        s += (f + " KB\n") % ("MaxRAM", self.maxRAM)
        s += (f +    "\n") % ("MaxCPU", self.maxCPU)
        return s
    
    # end __repr__

    def check(self):
        meminfo = MemInfo()
        cpuinfo = CPUInfo()
        notes = 0
        
        # print self
        openlog( 'redhat-support-check' )

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

            notes += 1


        if self.maxRAM > 0 and meminfo.memTotal > self.maxRAM:
            message = msgtop + [
                "***    allows no more than %d MB RAM" % asMB(self.maxRAM,"KB")
                ] + msgbot

            for line in message:
                print line
                syslog(line)

            notes += 1


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

            notes += 1



        if notes:
            print "%d support check notifications issued" % notes
            print "startup will continue in 30 seconds"
            time.sleep(30)
            
        return notes
   
# end class SupportLevel:
            

sys.exit( SupportLevel().check() )
