#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* this will be running setgid root, so be careful! */

void usage(void) {
    fprintf(stderr, "usage: netreport [-r]\n");
    exit(1);
}

#define ADD 1
#define DEL 0
int main(int argc, char ** argv) {
    int action = ADD;
    /* more than long enough for "/var/run/netreport/<pid>\0" */
    char netreport_name[64];
    int  netreport_file;

    if (argc > 2) usage();

    if ((argc > 1) && !strcmp(argv[1], "-r")) {
	action = DEL;
    }

    sprintf(netreport_name, "/var/run/netreport/%d", getppid());
    if (action == ADD) {
	netreport_file = creat(netreport_name, 0);
	if (netreport_file < 0) {
	    if (errno != EEXIST) {
		perror("Could not create netreport file");
		exit (1);
	    }
	} else {
	    close(netreport_file);
	}
    } else {
	/* ignore errors; not much we can do, won't hurt anything */
	unlink(netreport_name);
    }

    exit(0);
}
