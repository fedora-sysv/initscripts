#include <popt.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef unsigned int int32;

int main(int argc, char ** argv) {
    int showBroadcast = 0, showNetwork = 0;
    int rc;
    poptContext optCon;
    char * ipStr, * netmaskStr, * chptr;
    int32 ip, netmask, network, broadcast;
    struct poptOption optionsTable[] = {
	    { "broadcast", '\0', 0, &showBroadcast, 0 },
	    { "network", '\0', 0, &showNetwork, 0 },
	    { NULL, '\0', 0, 0, 0 },
    };

    optCon = poptGetContext("ipcalc", argc, argv, optionsTable,0);
    poptReadDefaultConfig(optCon, 1);

    if ((rc = poptGetNextOpt(optCon)) < -1) {
	fprintf(stderr, "ipcalc: bad argument %s: %s\n", 
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(rc));
	return 1;
    }

    if (!(ipStr = poptGetArg(optCon))) {
	fprintf(stderr, "ipcalc: ip address expected\n");
	return 1;
    }

    if (!(netmaskStr = poptGetArg(optCon))) {
	fprintf(stderr, "ipcalc: netmask expected\n");
	return 1;
    }

    if ((chptr = poptGetArg(optCon))) {
	fprintf(stderr, "ipcalc: unexpected argument: %s\n", chptr);
	return 1;
    }

    poptFreeContext(optCon);

    if (!inet_aton(ipStr, (struct in_addr *) &ip)) {
	fprintf(stderr, "ipcalc: bad ip address: %s\n", ipStr);
	return 1;
    }

    if (!inet_aton(netmaskStr, (struct in_addr *) &netmask)) {
	fprintf(stderr, "ipcalc: bad netmask: %s\n", netmaskStr);
	return 1;
    }

    if (showBroadcast) {
	broadcast = (ip & netmask) | ~netmask;
	printf("BROADCAST=%s\n", inet_ntoa(*((struct in_addr *) &broadcast)));
    }

    if (showNetwork) {
	network = ip & netmask;
	printf("NETWORK=%s\n", inet_ntoa(*((struct in_addr *) &network)));
    }

    return 0;
}
