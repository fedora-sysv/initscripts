/*
 * Copyright (c) 1997-2003 Red Hat, Inc. All rights reserved.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Authors:
 *   Erik Troan <ewt@redhat.com>
 *   Preston Brown <pbrown@redhat.com>
 */
     

#include <ctype.h>
#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*!
  \def IPBITS
  \brief the number of bits in an IP address.
*/
#define IPBITS (sizeof(u_int32_t) * 8)
/*!
  \def IPBYTES
  \brief the number of bytes in an IP address.
*/
#define IPBYTES (sizeof(u_int32_t))


/*!
  \file ipcalc.c
  \brief provides utilities for manipulating IP addresses.

  ipcalc provides utilities and a front-end command line interface for
  manipulating IP addresses, and calculating various aspects of an ip
  address/netmask/network address/prefix/etc.

  Functionality can be accessed from other languages from the library
  interface, documented here.  To use ipcalc from the shell, read the
  ipcalc(1) manual page.

  When passing parameters to the various functions, take note of whether they
  take host byte order or network byte order.  Most take host byte order, and
  return host byte order, but there are some exceptions.
  
*/

/*!
  \fn unsigned int prefix2mask(int bits)
  \brief creates a netmask from a specified number of bits
  
  This function converts a prefix length to a netmask.  As CIDR (classless
  internet domain internet domain routing) has taken off, more an more IP
  addresses are being specified in the format address/prefix
  (i.e. 192.168.2.3/24, with a corresponding netmask 255.255.255.0).  If you
  need to see what netmask corresponds to the prefix part of the address, this
  is the function.  See also \ref mask2prefix.
  
  \param prefix is the number of bits to create a mask for.
  \return a network mask, in network byte order.
*/
unsigned int prefix2mask(int prefix) {
    return htonl(~((1 << (32 - prefix)) - 1));
}

/*!
  \fn int mask2prefix(unsigned int mask)
  \brief calculates the number of bits masked off by a netmask.

  This function calculates the significant bits in an IP address as specified by
  a netmask.  See also \ref prefix2mask.

  \param mask is the netmask, specified as an unsigned integer in network byte order.
  \return the number of significant bits.  */
int mask2prefix(unsigned int mask)
{
    int i;
    int count = IPBITS;
	
    for (i = 0; i < IPBITS; i++) {
	if (!(ntohl(mask) & ((2 << i) - 1)))
	    count--;
    }

    return count;
}

/*!
  \fn unsigned int default_netmask(unsigned int addr)

  \brief returns the default (canonical) netmask associated with specified IP
  address.

  When the Internet was originally set up, various ranges of IP addresses were
  segmented into three network classes: A, B, and C.  This function will return
  a netmask that is associated with the IP address specified defining where it
  falls in the predefined classes.

  \param addr an IP address in network byte order.
  \return a netmask in network byte order.  */
unsigned int default_netmask(unsigned int addr)
{
    if (((ntohl(addr) & 0xFF000000) >> 24) <= 127)
	return htonl(0xFF000000);
    else if (((ntohl(addr) & 0xFF000000) >> 24) <= 191)
	return htonl(0xFFFF0000);
    else
	return htonl(0xFFFFFF00);
}

/*!
  \fn unsigned int calc_broadcast(unsigned int addr, int prefix)

  \brief calculate broadcast address given an IP address and a prefix length.

  \param addr an IP address in network byte order.
  \param prefix a prefix length.
  
  \return the calculated broadcast address for the network, in network byte
  order.
*/
unsigned int calc_broadcast(unsigned int addr,
				 int prefix)
{  
    return (addr & prefix2mask(prefix)) | ~prefix2mask(prefix);
}

/*!
  \fn unsigned int calc_network(unsigned int addr, int prefix)
  \brief calculates the network address for a specified address and prefix.

  \param addr an IP address, in network byte order
  \param prefix the network prefix
  \return the base address of the network that addr is associated with, in
  network byte order.
*/
unsigned int calc_network(unsigned int addr, int prefix)
{
    return (addr & prefix2mask(prefix));
}

/*!
  \fn const char *get_hostname(unsigned int addr)
  \brief returns the hostname associated with the specified IP address

  \param addr an IP address to find a hostname for, in network byte order

  \return a hostname, or NULL if one cannot be determined.  Hostname is stored
  in a static buffer that may disappear at any time, the caller should copy the
  data if it needs permanent storage.
*/
const char *get_hostname(unsigned int addr)
{
    struct hostent * hostinfo;
    int x;
    
    hostinfo = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
    if (!hostinfo)
	return NULL;

    for (x=0; hostinfo->h_name[x]; x++) {
	hostinfo->h_name[x] = tolower(hostinfo->h_name[x]);
    }
    return hostinfo->h_name;
}

/*!
  \fn main(int argc, const char **argv)
  \brief wrapper program for ipcalc functions.
  
  This is a wrapper program for the functions that the ipcalc library provides.
  It can be used from shell scripts or directly from the command line.
  
  For more information, please see the ipcalc(1) man page.
*/
int main(int argc, const char **argv) {
    int showBroadcast = 0, showPrefix = 0, showNetwork = 0;
    int showHostname = 0, showNetmask = 0;
    int beSilent = 0;
    int rc;
    poptContext optCon;
    char *ipStr, *prefixStr, *netmaskStr, *hostName, *chptr;
    struct in_addr ip, netmask, network, broadcast;
    int prefix = 0;
    char errBuf[250];
    struct poptOption optionsTable[] = {
	    { "broadcast", 'b', 0, &showBroadcast, 0,
		"Display calculated broadcast address", },
	    { "hostname", 'h', 0, &showHostname, 0,
		"Show hostname determined via DNS" },
	    { "netmask", 'm', 0, &showNetmask, 0,
		"Display default netmask for IP (class A, B, or C)" },
	    { "network", 'n', 0, &showNetwork, 0,
		"Display network address", },
	    { "prefix", 'p', 0, &showPrefix, 0,
	      "Display network prefix", },
	    { "silent", 's', 0, &beSilent, 0,
		"Don't ever display error messages " },
	    POPT_AUTOHELP
	    { NULL, '\0', 0, 0, 0, NULL, NULL }
    };

    optCon = poptGetContext("ipcalc", argc, argv, optionsTable, 0);
    poptReadDefaultConfig(optCon, 1);

    if ((rc = poptGetNextOpt(optCon)) < -1) {
	if (!beSilent) {
	    fprintf(stderr, "ipcalc: bad argument %s: %s\n", 
		    poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		    poptStrerror(rc));
	    poptPrintHelp(optCon, stderr, 0);
	}
	return 1;
    }

    if (!(ipStr = (char *) poptGetArg(optCon))) {
	if (!beSilent) {
	    fprintf(stderr, "ipcalc: ip address expected\n");
	    poptPrintHelp(optCon, stderr, 0);
	}
	return 1;
    }

    if (strchr(ipStr,'/') != NULL) {
	prefixStr = strchr(ipStr, '/') + 1;
	prefixStr--;
	*prefixStr = '\0';  /* fix up ipStr */
	prefixStr++;
    } else
	prefixStr = NULL;
    
    if (prefixStr != NULL) {
	prefix = atoi(prefixStr);
	if (prefix == 0) {
	    if (!beSilent)
		fprintf(stderr, "ipcalc: bad prefix: %s\n",
			prefixStr);
	    return 1;
	}
    }
	
    if (showBroadcast || showNetwork || showPrefix) {
	if (!(netmaskStr = (char *) poptGetArg(optCon)) &&
	    (prefix == 0)) {
	    if (!beSilent) {
		fprintf(stderr, "ipcalc: netmask or prefix expected\n");
		poptPrintHelp(optCon, stderr, 0);
	    }
	    return 1;
	} else if (netmaskStr && prefix != 0) {
	    if (!beSilent) {
		    fprintf(stderr, "ipcalc: both netmask and prefix specified\n");
		    poptPrintHelp(optCon, stderr, 0);
	    }
	    return 1;
	} else if (netmaskStr) {
	    if (!inet_aton(netmaskStr, &netmask)) {
		if (!beSilent)
		    fprintf(stderr, "ipcalc: bad netmask: %s\n",
			    netmaskStr);
		return 1;
	    }
	    prefix = mask2prefix(netmask.s_addr);
	}
    }

    if ((chptr = (char *) poptGetArg(optCon))) {
	if (!beSilent) {
	    fprintf(stderr, "ipcalc: unexpected argument: %s\n", chptr);
	    poptPrintHelp(optCon, stderr, 0);
	}
	return 1;
    }

    /* Handle CIDR entries such as 172/8 */
    if (prefix) {
    	char *tmp = ipStr;
	int i;
	    
	for(i=3; i> 0; i--) {
		tmp = strchr(tmp,'.');
		if (!tmp) 
			break;
		else
			tmp++;
	}
	tmp = NULL;
	for (; i>0; i--) {
	   tmp = malloc(strlen(ipStr) + 3);
	   sprintf(tmp,"%s.0",ipStr);
	   ipStr = tmp;
	}
    }

    if (!inet_aton(ipStr, (struct in_addr *) &ip)) {
	if (!beSilent)
	    fprintf(stderr, "ipcalc: bad ip address: %s\n", ipStr);
	return 1;
    }

    
    if (!(showNetmask|showPrefix|showBroadcast|showNetwork|showHostname)) {
	    poptPrintHelp(optCon, stderr, 0);
	    return 1;
    }

    poptFreeContext(optCon);

    /* we know what we want to display now, so display it. */

    if (showNetmask) {
	if (prefix) {
	    netmask.s_addr = prefix2mask(prefix);
	} else {
	    netmask.s_addr = default_netmask(ip.s_addr);
	    prefix = mask2prefix(netmask.s_addr);
	}

	printf("NETMASK=%s\n", inet_ntoa(netmask));
    }

    if (showPrefix) {
	if (!prefix)
	    prefix = mask2prefix(ip.s_addr);
	printf("PREFIX=%d\n", prefix);
    }
    	    
    if (showBroadcast) {
	broadcast.s_addr = calc_broadcast(ip.s_addr, prefix);
	printf("BROADCAST=%s\n", inet_ntoa(broadcast));
    }

    if (showNetwork) {
	network.s_addr = calc_network(ip.s_addr, prefix);
	printf("NETWORK=%s\n", inet_ntoa(network));
    }
    
    if (showHostname) {	
	if ((hostName = (char *) get_hostname(ip.s_addr)) == NULL) {
	    if (!beSilent) {
		sprintf(errBuf, "ipcalc: cannot find hostname for %s", ipStr);
		herror(errBuf);
	    }
	    return 1;
	}
	
	printf("HOSTNAME=%s\n", hostName);
    }
	
    return 0;
}
