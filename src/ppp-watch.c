/*
 * ppp-watch.c
 *
 * Bring up a PPP connection and Do The Right Thing[tm] to make bringing
 * the connection up or down with ifup and ifdown syncronous.  Takes
 * one argument: the logical name of the device to bring up.  Does not
 * detach until the interface is up or has permanently failed to come up.
 *
 * Copyright 1999 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Algorithm:
 *   fork
 *   if child:
 *     Register with netreport.  (now exit implies deregister first)
 *     fork/exec ifup-ppp daemon <interface>
 *   else:
 *     while (1):
 *       sigsuspend()
 *       if SIGTERM:
 *         kill pppd pgrp
 *         exit
 *       if SIGHUP:
 *         reload ifcfg files
 *         kill pppd pgrp
 *         wait for SIGCHLD to redial
 *       if SIGIO:
 *         if no physical device found: continue
 *         elif physical device is down:
 *           wait for pppd to exit to redial if appropriate
 *         else: (physical device is up)
 *           detach; continue
 *       if SIGCHLD: (pppd exited)
 *         wait()
 *         if pppd exited:
 *           if PERSIST: redial
 *           else: exit
 *         else: (pppd was killed)
 *           exit
 *
 *
 * When ppp-watch itself dies for reasons of its own, it uses a return code
 * higher than 30 so as not to clash with pppd return codes, which, as of
 * this writing, range from 0 to 19.
 */

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <net/if.h>

#include "shvar.h"

static int theSigterm = 0;
static int theSigint = 0;
static int theSighup = 0;
static int theSigio = 0;
static int theSigchld = 0;
static int theSigalrm = 0;



static void
cleanExit(int exitCode);




static void
detach(int now, int parentExitCode, char *device) {
    static int pipeArray[2];
    char exitCode;

    if (now) {
	/* execute -- ignore errors in case called more than once */
	exitCode = parentExitCode;
	write(pipeArray[1], &exitCode, 1);
	close(pipeArray[1]);

    } else {
	/* set up */
	int child;

	if (pipe(pipeArray)) exit (25);

	child = fork();
	if (child < 0) exit(26);
	if (child) {
	    /* parent process */
	    close (pipeArray[1]);
	    while (read (pipeArray[0], &exitCode, 1) < 0) {
		switch (errno) {
		    case EINTR: continue;
		    default: exit (27); /* this will catch EIO in particular */
		}
	    }
	    switch (exitCode) {
	    case 0:
		break;
	    case 33:
		fprintf(stderr, "%s already up, initiating redial\n", device);
	    case 34:
		fprintf(stderr, "Failed to activate %s, retrying in the background\n", device);
		break;
	    default:
		fprintf(stderr, "Failed to activate %s\n", device);
		break;
	    }
	    exit(exitCode);

	} else {
	    /* child process */
	    close (pipeArray[0]);
	    /* become a daemon */
	    close (0);
	    close (1);
	    close (2);
	    setsid();
	    setpgid(0, 0);
	}
    }
}



static void
doPidFile(char *device) {
    static char *pidFileName = NULL;
    char *pidFilePath;
    int fd; FILE *f;
    pid_t pid = 0;

    if (pidFileName) {
	/* remove it */
	pidFilePath = alloca(strlen(pidFileName) + 25);
	sprintf(pidFilePath, "/var/run/pppwatch-%s.pid", pidFileName);
	unlink(pidFilePath); /* not much we can do in case of error... */
    }

    if (device) {
	/* create it */
	pidFileName = device;
	pidFilePath = alloca(strlen(pidFileName) + 25);
	sprintf(pidFilePath, "/var/run/pppwatch-%s.pid", pidFileName);
restart:
	fd = open(pidFilePath, O_WRONLY|O_TRUNC|O_CREAT|O_EXCL,
		  S_IRUSR|S_IWUSR | S_IRGRP | S_IROTH);

	if (fd == -1) {
	    /* file already existed, or terrible things have happened... */
	    fd = open(pidFilePath, O_RDONLY);
	    if (fd == -1)
		cleanExit(36); /* terrible things have happened */
	    /* already running, send a SIGHUP (we presume that they
	     * are calling ifup for a reason, so they probably want
	     * to redial) and then exit cleanly and let things go
	     * on in the background
	     */
	    f = fdopen(fd, "r");
	    if (!f) cleanExit(37);
	    fscanf(f, "%d", &pid);
	    fclose(f);
	    if (pid) {
		if (kill(pid, SIGHUP)) {
		    unlink(pidFilePath);
		    goto restart;
		} else {
		    cleanExit(33);
		}
	    }
	}

	f = fdopen(fd, "w");
	if (!f)
	    cleanExit(31);
	fprintf(f, "%d\n", getpid());
	fclose(f);
    }
}





int
fork_exec(int wait, char *path, char *arg1, char *arg2, char *arg3)
{
    pid_t child;
    int status;

    if (!(child = fork())) {
	/* child */

	if (!wait) {
	    /* make sure that pppd is in its own process group */
	    setsid();
	    setpgid(0, 0);
	}

	execl(path, path, arg1, arg2, arg3, NULL);
	perror(path);
	_exit (1);
    }

    if (wait) {
	wait4 (child, &status, 0, NULL);
	if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
	    return 0;
	} else {
	    return 1;
	}
    } else {
	return 0;
    }
}



static void
cleanExit(int exitCode) {
    fork_exec(1, "/sbin/netreport", "-r", NULL, NULL);
    detach(1, exitCode, NULL);
    doPidFile(NULL);
    exit(exitCode);
}



static void
signal_handler (int signum) {
    switch(signum) {
    case SIGTERM:
	theSigterm = 1; break;
    case SIGINT:
	theSigint = 1; break;
    case SIGHUP:
	theSighup = 1; break;
    case SIGIO:
	theSigio = 1; break;
    case SIGCHLD:
	theSigchld = 1; break;
    case SIGALRM:
	theSigalrm = 1; break;
    }
}


static shvarFile *
shvarfilesGet(char *interfaceName) {
    shvarFile *ifcfg;
    char *ifcfgName, *ifcfgParentName, *ifcfgParentDiff;
    static char ifcfgPrefix[] = "/etc/sysconfig/network-scripts/ifcfg-";

    ifcfgName = alloca(sizeof(ifcfgPrefix)+strlen(interfaceName)+1);
    sprintf(ifcfgName, "%s%s", ifcfgPrefix, interfaceName);
    ifcfg = svNewFile(ifcfgName);
    if (!ifcfg) return NULL;

    /* Do we have a parent interface to inherit? */
    ifcfgParentDiff = strchr(interfaceName, '-');
    if (ifcfgParentDiff) {
        /* allocate more than enough memory... */
	ifcfgParentName = alloca(sizeof(ifcfgPrefix)+strlen(interfaceName)+1);
	strcpy(ifcfgParentName, ifcfgPrefix);
	strncat(ifcfgParentName, interfaceName, ifcfgParentDiff-interfaceName);
	ifcfg->parent = svNewFile(ifcfgParentName);
    }

    return ifcfg;
}



static char *
pppLogicalToPhysical(int *pppdPid, char *logicalName) {
    char *mapFileName;
    char buffer[20]; /* more than enough space for ppp<n> */
    char *p, *q;
    int f, n;
    char *physicalDevice = NULL;

    mapFileName = alloca (strlen(logicalName)+20);
    sprintf(mapFileName, "/var/run/ppp-%s.pid", logicalName);
    if ((f = open(mapFileName, O_RDONLY)) >= 0) {
	n = read(f, buffer, 20);
	if (n > 0) {
	    buffer[n] = '\0';
	    /* get past pid */
	    p = buffer; while (*p && *p != '\n') p++; *p = '\0'; p++;
	    if (pppdPid) *pppdPid = atoi(buffer);
	    /* get rid of \n */
	    q = p; while (*q && *q != '\n' && q < buffer+n) q++; *q = '\0';
	    if (*p) physicalDevice = strdup(p);
	}
	close(f);
    }

    return physicalDevice;
}


static int
interfaceStatus(char *device) {
    int sock = 0;
    int pfs[] = {AF_INET, AF_IPX, AF_AX25, AF_APPLETALK, 0};
    int p = 0;
    struct ifreq ifr;
    int retcode = 0;

    while (!sock && pfs[p]) {
        sock = socket(pfs[p++], SOCK_DGRAM, 0);
    }
    if (!sock) return 0;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        retcode = 0;
    } else if (ifr.ifr_flags & IFF_UP) {
        retcode = 1;
    }

    close(sock);
    return retcode;
}


/* very, very minimal hangup function.  This is just to attempt to
 * hang up a device that should already be hung up, so it does not
 * need to be bulletproof.
 */
void
hangup(shvarFile *ifcfg) {
    int fd;
    char *filename;
    struct termios ots, ts;

    filename = svGetValue(ifcfg, "MODEMPORT");
    if (!filename) return;
    fd = open(filename, O_RDWR|O_NOCTTY|O_NONBLOCK);
    if (fd == -1) goto clean; 
    if (tcgetattr(fd, &ts)) goto clean;
    ots = ts;
    write(fd, "\r", 1); /* tickle modems that do not like dropped DTR */
    usleep(1000);
    cfsetospeed(&ts, B0);
    tcsetattr(fd, TCSANOW, &ts);
    usleep(100000);
    tcsetattr(fd, TCSANOW, &ots);

clean:
    free(filename);
}









int
main(int argc, char **argv) {
    int status, waited;
    char *device, *physicalDevice = NULL;
    char *theBoot = NULL;
    shvarFile *ifcfg;
    sigset_t sigs;
    int pppdPid;
    int timeout = 30;
    char *temp;
    struct timeval tv;
    int dieing = 0;
    int sendsig;
    int connectedOnce = 0;

    if (argc < 2) {
	fprintf (stderr, "usage: ppp-watch [ifcfg-]<logical-name> [boot]");
	exit(30);
    }

    if (!strncmp(argv[1], "ifcfg-", 6)) {
	device = argv[1] + 6;
    } else {
	device = argv[1];
    }

    detach(0, 0, device); /* prepare */

    if (argc > 2 && !strcmp("boot", argv[2])) {
	theBoot = argv[2];
    }

    doPidFile(device);

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGIO, signal_handler);
    signal(SIGCHLD, signal_handler);
    if (theBoot) {
	signal(SIGALRM, signal_handler);
	alarm(30);
    }

    fork_exec(1, "/sbin/netreport", NULL, NULL, NULL);
    theSigchld = 0;

    /* don't set up the procmask until after we have received the netreport
     * signal
     */
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGTERM);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGHUP);
    sigaddset(&sigs, SIGIO);
    sigaddset(&sigs, SIGCHLD);
    if (theBoot) sigaddset(&sigs, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigs, NULL);

    /* prepare for sigsuspend later */
    sigfillset(&sigs);
    sigdelset(&sigs, SIGTERM);
    sigdelset(&sigs, SIGINT);
    sigdelset(&sigs, SIGHUP);
    sigdelset(&sigs, SIGIO);
    sigdelset(&sigs, SIGCHLD);
    if (theBoot) sigdelset(&sigs, SIGALRM);


    ifcfg = shvarfilesGet(device);
    if (!ifcfg) cleanExit(28);

    fork_exec(0, "/etc/sysconfig/network-scripts/ifup-ppp", "daemon", device, theBoot);
    temp = svGetValue(ifcfg, "RETRYTIMEOUT");
    if (temp) {
	timeout = atoi(temp);
	free(temp);
    } else {
	timeout = 30;
    }

    while (1) {
	sigsuspend(&sigs);
	if (theSigterm || theSigint) {
	    theSigterm = theSigint = 0;

	    if (dieing) sendsig = SIGKILL;
	    else sendsig = SIGTERM;
	    dieing = 1;

	    if (physicalDevice) { free(physicalDevice); physicalDevice = NULL; }
	    physicalDevice = pppLogicalToPhysical(&pppdPid, device);
	    if (physicalDevice) { free(physicalDevice); physicalDevice = NULL; }
	    if (!pppdPid) cleanExit(35);
	    kill(pppdPid, sendsig);
	    if (sendsig == SIGKILL) {
		kill(-pppdPid, SIGTERM); /* give it a chance to die nicely */
		usleep(2500000);
		kill(-pppdPid, sendsig);
		hangup(ifcfg);
		cleanExit(32);
	    }
	}
	if (theSighup) {
	    theSighup = 0;
	    if (ifcfg->parent) svCloseFile(ifcfg->parent);
	    svCloseFile(ifcfg);
	    ifcfg = shvarfilesGet(device);
	    physicalDevice = pppLogicalToPhysical(&pppdPid, device);
	    if (physicalDevice) { free(physicalDevice); physicalDevice = NULL; }
	    kill(pppdPid, SIGTERM);
	    /* redial when SIGCHLD arrives, even if !PERSIST */
	    connectedOnce = 0;
	    timeout = 0; /* redial immediately */
	}
	if (theSigio) {
	    theSigio = 0;
	    if (connectedOnce) {
		if (physicalDevice) { free(physicalDevice); physicalDevice = NULL; }
		temp = svGetValue(ifcfg, "DISCONNECTTIMEOUT");
		if (temp) {
		    timeout = atoi(temp);
		    free(temp);
		} else {
		    timeout = 2;
		}
	    }
	    physicalDevice = pppLogicalToPhysical(NULL, device);
	    if (physicalDevice) {
		if (interfaceStatus(physicalDevice)) {
		    /* device is up */
		    detach(1, 0, NULL);
		    connectedOnce = 1;
		}
	    }
	}
	if (theSigchld) {
	    theSigchld = 0;
	    waited = wait3(&status, 0, NULL);
	    if (waited < 0) continue;

	    /* now, we need to kill any children of pppd still in pppd's
	     * process group, in case they are hanging around.
	     * pppd is dead (we just waited for it) but there is no
	     * guarantee that its children are dead, and they will
	     * hold the modem if we do not get rid of them.
	     * We have kept the old pid/pgrp around in pppdPid.
	     */
	    if (pppdPid) {
		kill(-pppdPid, SIGTERM); /* give it a chance to die nicely */
		usleep(2500000);
		kill(-pppdPid, SIGKILL);
		hangup(ifcfg);
	    }
	    pppdPid = 0;

	    if (!WIFEXITED(status)) cleanExit(29);
	    if (dieing) cleanExit(WEXITSTATUS(status));

	    if (!connectedOnce || svTrueValue(ifcfg, "PERSIST", 0)) {
		temp = svGetValue(ifcfg, "RETRYTIMEOUT");
		if (temp) {
		    timeout = atoi(temp);
		    free(temp);
		} else {
		    timeout = 30;
		}
		if (connectedOnce) {
		    memset(&tv, 0, sizeof(tv));
		    tv.tv_sec = timeout;
		    select(0, NULL, NULL, NULL, &tv);
		}
		fork_exec(0, "/etc/sysconfig/network-scripts/ifup-ppp", "daemon", device, theBoot);
	    } else {
		cleanExit(WEXITSTATUS(status));
	    }
	}
	if (theSigalrm) {
	    detach(1, 34, NULL);
	}
    }
}
