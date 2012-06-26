/*
 * ppp-watch.c
 *
 * Bring up a PPP connection and Do The Right Thing[tm] to make bringing
 * the connection up or down with ifup and ifdown synchronous.  Takes
 * one argument: the logical name of the device to bring up.  Does not
 * detach until the interface is up or has permanently failed to come up.
 *
 * Copyright 1999-2007 Red Hat, Inc.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
 *       if SIGTERM or SIGINT:
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
 * higher than 25 so as not to clash with pppd return codes, which, as of
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
#include <glib.h>
#include "shvar.h"

#define IFCFGPREFIX "/etc/sysconfig/network-scripts/ifcfg-"
#define IFUP_PPP "/etc/sysconfig/network-scripts/ifup-ppp"

static int theSigterm = 0;
static int theSigint = 0;
static int theSighup = 0;
static int theSigio = 0;
static int theSigchld = 0;
static int theSigalrm = 0;
static int pipeArray[2];

// patch to respect the maxfail parameter to ppp
// Scott Sharkey <ssharkey@linux-no-limits.com>
static int dialCount = 0;
static pid_t theChild;
static void failureExit(int exitCode);

static void
interrupt_child(int signo) {
    kill(theChild, SIGINT);
}

static void
set_signal(int signo, void (*handler)(int)) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(signo, &act, NULL);
}

/* Create a pipe, and fork off a child.  This is the end of the road for
 * the parent, which will wait for an exit status byte on the pipe (which
 * is written by the child). */
static void
detach(char *device) {
    pid_t childpid;
    unsigned char exitCode;
    int fd;

    if (pipe(pipeArray) == -1)
	exit(25);

    childpid = fork();
    if (childpid == -1)
	exit(26);

    if (childpid != 0) {
        /* The parent only cares about notifications from the child. */
        close (pipeArray[1]);

        /* Certain signals are meant for our child, the watcher process. */
        theChild = childpid;
        set_signal(SIGINT, interrupt_child);
        set_signal(SIGTERM, interrupt_child);
        set_signal(SIGHUP, interrupt_child);

        /* Read the pipe until the child gives us an exit code as a byte. */
        while (read (pipeArray[0], &exitCode, 1) == -1) {
	    switch (errno) {
	        case EINTR: continue;
	        default: exit (27); /* this will catch EIO in particular */
	    }
        }
        switch (exitCode) {
	case 0:
	    break;
	    
	case 1:
	    fprintf(stderr, "Device %s: An immediately fatal error of some kind  occurred,  such  as  an "
		    "essential system call failing, or running out of virtual memory.\n", device);
	    break;
	  
	case 2:
	    fprintf(stderr, "Device %s: An error was detected in processing the options given, such as "
		    "two mutually exclusive options being used.\n", device);
	    break;
	  
	case 3:
	    fprintf(stderr, "Device %s: Pppd is not setuid-root and the invoking user is not root.\n", device);
	    break;
	  
	case 4:
	    fprintf(stderr, "Device %s: The  kernel  does  not  support PPP, for example, the PPP kernel "
		    "driver is not included or cannot be loaded.\n", device);
	    break;
	  
	case 5:
	    fprintf(stderr, "Device %s: Pppd terminated because it was sent a SIGINT, SIGTERM or SIGHUP signal.\n", device);
	    break;
	  
	case 6:
	    fprintf(stderr, "Device %s: The serial port could not be locked.\n", device);
	    break;
	  
	case 7:
	    fprintf(stderr, "Device %s: The serial port could not be opened.\n", device);
	    break;
	  
	case 8:
	    fprintf(stderr, "Device %s: The connect script failed (maybe wrong password or wrong phone number).\n", device);
	    break;
	  
	case 9:
	    fprintf(stderr, "Device %s: The command specified as the argument to the pty option could not be run.\n", device);
	    break;
	  
	case 10:
	    fprintf(stderr, "Device %s: The PPP negotiation failed, that is, it didn't reach the point "
		    "where at least one network protocol (e.g. IP) was running.\n", device);
	    break;
	  
	case 11:
	    fprintf(stderr, "Device %s: The peer system failed (or refused) to authenticate itself.\n", device);
	    break;
	  
	case 12:
	    fprintf(stderr, "Device %s: The link was established successfully and terminated because it was idle.\n", device);
	    break;
	  
	case 13:
	    fprintf(stderr, "Device %s: The link was established successfully and terminated because the connect time limit was reached.\n", device);
	    break;

	case 14:
	    fprintf(stderr, "Device %s: Callback was negotiated and an incoming  call should arrive shortly.\n", device);
	    break;

	case 15:
	    fprintf(stderr, "Device %s: The link was terminated because the peer is not responding to echo requests.\n", device);
	    break;

	case 16:
	    fprintf(stderr, "Device %s: The link was terminated by the modem hanging up.\n", device);
	    break;

	case 18:
	    fprintf(stderr, "Device %s: The init script failed (returned a non-zero exit status).\n", device);
	    break;

	case 19:
	    fprintf(stderr, "Device %s: We failed to authenticate ourselves to the peer.\n", device);
	    break;

	case 33:
	    fprintf(stderr, "%s already up, initiating redial\n", device);
	    break;
	case 34:
	    fprintf(stderr, "Failed to activate %s, retrying in the background\n", device);
	    break;
	default:
	    fprintf(stderr, "Failed to activate %s with error %d\n", device, exitCode);
	    break;
	}
	exit(exitCode);
    }

    /* We're in the child process, which only writes the exit status
     * of the pppd process to its parent (i.e., it reads nothing). */
    close (pipeArray[0]);

    /* Don't leak this into programs we call. */
    fcntl(pipeArray[1], F_SETFD, FD_CLOEXEC);

    /* Redirect stdio to /dev/null. */
    fd = open("/dev/null", O_RDONLY);
    dup2(fd, STDIN_FILENO);
    close(fd);

    fd = open("/dev/null", O_WRONLY);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    /* Become session and process group leader. */
    setsid();
    setpgid(0, 0);
}

/* Do magic with the pid file (/var/run/pppwatch-$DEVICE.pid):
 * Try to open it for writing.  If it exists, send a SIGHUP to whatever PID
 * is already listed in it and remove it.  Repeat until we can open it.
 * Write out our PID, and return. */
static void
doPidFile(char *device) {
    static char pidFilePath[PATH_MAX] = "";
    int fd = -1;
    FILE *f = NULL;
    pid_t pid = 0;

    if (device == NULL) {
        /* Remove an existing pid file -- we're exiting. */
	if(strlen(pidFilePath) > 0) {
            unlink(pidFilePath);
	}
    } else  {
	/* Set up the name of the pid file, used only the first time. */
        snprintf(pidFilePath, sizeof(pidFilePath), "/var/run/pppwatch-%s.pid",
	         device);

	/* Create the pid file. */
        do {
	    fd = open(pidFilePath, O_WRONLY|O_TRUNC|O_CREAT|O_EXCL|O_NOFOLLOW,
		      S_IRUSR|S_IWUSR | S_IRGRP | S_IROTH);
	    if(fd == -1) {
	        /* Try to open the file for read. */
	        fd = open(pidFilePath, O_RDONLY);
	        if(fd == -1)
		    failureExit(36); /* This is not good. */

	        /* We're already running, send a SIGHUP (we presume that they
	         * are calling ifup for a reason, so they probably want to
		 * redial) and then exit cleanly and let things go on in the
		 * background.  Muck with the filename so that we don't go
		 * deleting the pid file for the already-running instance.
	         */
	        f = fdopen(fd, "r");
	        if(f == NULL)
		    failureExit(37);

		pid = 0;
	        fscanf(f, "%d", &pid);
	        fclose(f);

	        if(pid) {
                    /* Try to kill it. */
		    if (kill(pid, SIGHUP) == -1) {
		        /* No such pid, remove the bogus pid file. */
		        unlink(pidFilePath);
		    } else {
		        /* Got it.  Don't mess with the pid file on
			 * our way out. */
			memset(pidFilePath, '\0', sizeof(pidFilePath));
                        failureExit(33);
		    }
	        }
	    }
	} while(fd == -1);

	f = fdopen(fd, "w");
	if(f == NULL)
	    failureExit(31);
	fprintf(f, "%d\n", getpid());
	fclose(f);
    }
}

/* Fork off and exec() a child process.  If reap_child is non-zero,
 * wait for the child to exit and return 0 if it ran successfully,
 * otherwise return 0 right away and let the SIGCHLD handler deal. */
static int
fork_exec(gboolean reap, char *path, char *arg1, char *arg2, char *arg3)
{
    pid_t childpid;
    int status;

    sigset_t sigs;

    childpid = fork();
    if (childpid == -1)
	exit(26);

    if (childpid == 0) {
	/* Do the exec magic.  Prepare by clearing the signal mask for pppd. */
	sigemptyset(&sigs);
	sigprocmask(SIG_SETMASK, &sigs, NULL);

	if (!reap) {
	    /* Make sure that the pppd is the leader for its process group. */
	    setsid();
	    setpgid(0, 0);
	}

	execl(path, path, arg1, arg2, arg3, NULL);
	perror(path);
	_exit (1);
    }

    if (reap) {
	waitpid (childpid, &status, 0);
	if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
	    return 0;
	} else {
	    return 1;
	}
    } else {
	return 0;
    }
}

/* Relay the pppd's exit code up to the parent -- can only be called once,
 * because the parent exits as soon as it reads a byte. */
static void
relay_exitcode(unsigned char code)
{
    unsigned char exitCode;
    exitCode = code;
    write(pipeArray[1], &exitCode, 1);
    close(pipeArray[1]);
}

/* Unregister with netreport, relay a status byte to the parent, clean up
 * the pid file, and bail. */
static void
failureExit(int exitCode) {
    fork_exec(TRUE, "/sbin/netreport", "-r", NULL, NULL);
    relay_exitcode(exitCode);
    doPidFile(NULL);
    exit(exitCode);
}

/* Keeps track of which signals we've seen so far. */
static void
signal_tracker (int signum) {
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

/* Return a shvarFile for this interface, taking into account one level of
 * inheritance (eeewww). */
static shvarFile *
shvarfilesGet(const char *interfaceName) {
    shvarFile *ifcfg = NULL;
    char ifcfgName[PATH_MAX];
    char *ifcfgParentDiff = NULL;

    /* Start with the basic configuration. */
    snprintf(ifcfgName, sizeof(ifcfgName), "%s%s", IFCFGPREFIX, interfaceName);
    ifcfg = svNewFile(ifcfgName);
    if (ifcfg == NULL)
	    return NULL;

    /* Do we have a parent interface (i.e., for ppp0-blah, ppp0) to inherit? */
    ifcfgParentDiff = strchr(ifcfgName + sizeof(IFCFGPREFIX), '-');
    if (ifcfgParentDiff) {
        *ifcfgParentDiff = '\0';
	ifcfg->parent = svNewFile(ifcfgName);
    }

    /* This is very unclean, but we have to close the shvar descriptors in
     * case they've been numbered STDOUT_FILENO or STDERR_FILENO, which would
     * be disastrous if inherited by a child process. */
    close (ifcfg->fd);
    ifcfg->fd = 0;

    if (ifcfg->parent) {
	close (ifcfg->parent->fd);
	ifcfg->parent->fd = 0;
    }

    return ifcfg;
}

/* Convert a logical interface name to a real one by reading the lock
 * file created by pppd. */
static void
pppLogicalToPhysical(int *pppdPid, char *logicalName, char **physicalName) {
    char mapFileName[PATH_MAX];
    char buffer[20];
    char *p, *q;
    int fd, n;
    char *physicalDevice = NULL;

    snprintf(mapFileName, sizeof(mapFileName), "/var/run/ppp-%s.pid",
	     logicalName);
    fd = open(mapFileName, O_RDONLY);
    if (fd != -1) {
	n = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (n > 0) {
	    buffer[n] = '\0';
	    /* Split up the file at the first line break -- the PID is on the
	     * first line. */
	    if((p = strchr(buffer, '\n')) != NULL) {
		*p = '\0';
		p++;
	        if (pppdPid) {
		    *pppdPid = atoi(buffer);
	        }
	        /* The physical device name is on the second line. */
	        if((q = strchr(p, '\n')) != NULL) {
		    *q = '\0';
		    physicalDevice = strdup(p);
		}
	    }
	}
    }

    if (physicalDevice) {
        if (physicalName) {
            *physicalName = physicalDevice;
	} else {
            free(physicalDevice);
	}
    } else {
        if (physicalName) {
            *physicalName = NULL;
	}
    }
}

/* Return a boolean value indicating if the interface is up.  If not, or
 * if we don't know, return FALSE. */
static gboolean
interfaceIsUp(char *device) {
    int sock = -1;
    int family[] = {PF_INET, PF_IPX, PF_AX25, PF_APPLETALK, 0};
    int p = 0;
    struct ifreq ifr;
    gboolean retcode = FALSE;

    /* Create a socket suitable for doing routing ioctls. */
    for (p = 0; (sock == -1) && family[p]; p++) {
        sock = socket(family[p], SOCK_DGRAM, 0);
    }
    if (sock == -1)
	return FALSE;

    /* Populate the request structure for getting the interface's status. */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

    /* We return TRUE iff the ioctl succeeded and the interface is UP. */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
        retcode = FALSE;
    } else if (ifr.ifr_flags & IFF_UP) {
        retcode = TRUE;
    }

    close(sock);

    return retcode;
}

/* Very, very minimal hangup function.  This just attempts to hang up a device
 * that should already be hung up, so it does not need to be bulletproof.  */
static void
hangup(shvarFile *ifcfg) {
    int fd;
    char *line;
    struct termios original_ts, ts;

    line = svGetValue(ifcfg, "MODEMPORT");
    if (line == NULL)
	return;

    fd = open(line, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd != -1) {
        if (tcgetattr(fd, &ts) != -1) {
            original_ts = ts;
            write(fd, "\r", 1); /* tickle modems that do not like dropped DTR */
            usleep(1000);
            cfsetospeed(&ts, B0);
            tcsetattr(fd, TCSANOW, &ts);
            usleep(100000);
            tcsetattr(fd, TCSANOW, &original_ts);
	}
	close(fd);
    }
    free(line);
}

int
main(int argc, char **argv) {
    int status;
    pid_t waited;
    char *device, *real_device, *physicalDevice = NULL;
    char *boot = NULL;
    shvarFile *ifcfg;
    sigset_t blockedsigs, unblockedsigs;
    int pppdPid = 0;
    int timeout = 30;
    char *temp;
    gboolean dying = FALSE;
    int sendsig;
    gboolean connectedOnce = FALSE;
    int maxfail = 0;		// MAXFAIL Patch <ssharkey@linux-no-limits.com>

    if (argc < 2) {
	fprintf (stderr, "usage: ppp-watch <interface-name> [boot]\n");
	exit(30);
    }

    if (strncmp(argv[1], "ifcfg-", 6) == 0) {
	device = argv[1] + 6;
    } else {
	device = argv[1];
    }

    detach(device); /* Prepare a child process to monitor pppd.  When we
		       return, we'll be in the child. */

    if ((argc > 2) && (strcmp("boot", argv[2]) == 0)) {
	boot = argv[2];
    }

    ifcfg = shvarfilesGet(device);
    if (ifcfg == NULL)
	failureExit(28);

    real_device = svGetValue(ifcfg, "DEVICE");
    if (real_device == NULL)
	real_device = device;

    doPidFile(real_device);

    /* We'll want to know which signal interrupted our sleep below, so
     * attach a signal handler to these. */
    set_signal(SIGTERM, signal_tracker);
    set_signal(SIGINT, signal_tracker);
    set_signal(SIGHUP, signal_tracker);
    set_signal(SIGIO, signal_tracker);
    set_signal(SIGCHLD, signal_tracker);

    /* We time out only if we're being run at boot-time. */
    if (boot) {
	temp = svGetValue(ifcfg, "BOOTTIMEOUT");
	if (temp) {
	    timeout = atoi(temp);
	    if (timeout < 1) timeout = 1;
	    free(temp);
	} else {
	    timeout = 30;
	}
	set_signal(SIGALRM, signal_tracker);
	alarm(timeout);
    }

    /* Register us to get a signal when something changes. Yes, that's vague. */
    fork_exec(TRUE, "/sbin/netreport", NULL, NULL, NULL);

    /* Reset theSigchld, which should have been triggered by netreport. */
    theSigchld = 0;

    /* We don't set up the procmask until after we have received the netreport
     * signal.  Do so now. */
    sigemptyset(&blockedsigs);
    sigaddset(&blockedsigs, SIGTERM);
    sigaddset(&blockedsigs, SIGINT);
    sigaddset(&blockedsigs, SIGHUP);
    sigaddset(&blockedsigs, SIGIO);
    sigaddset(&blockedsigs, SIGCHLD);
    if (boot) {
	sigaddset(&blockedsigs, SIGALRM);
    }
    sigprocmask(SIG_BLOCK, &blockedsigs, NULL);

    sigfillset(&unblockedsigs);
    sigdelset(&unblockedsigs, SIGTERM);
    sigdelset(&unblockedsigs, SIGINT);
    sigdelset(&unblockedsigs, SIGHUP);
    sigdelset(&unblockedsigs, SIGIO);
    sigdelset(&unblockedsigs, SIGCHLD);
    if (boot) {
	sigdelset(&unblockedsigs, SIGALRM);
    }
    sigprocmask(SIG_UNBLOCK, &unblockedsigs, NULL);

    /* Initialize the retry timeout using the RETRYTIMEOUT setting. */
    temp = svGetValue(ifcfg, "RETRYTIMEOUT");
    if (temp) {
	timeout = atoi(temp);
	free(temp);
    } else {
	timeout = 30;
    }

    /* Start trying to bring the interface up. */
    fork_exec(FALSE, IFUP_PPP, "daemon", device, boot);

    while (TRUE) {
	/* Wait for a signal. */
	if (!theSigterm &&
	    !theSigint &&
	    !theSighup &&
	    !theSigio &&
	    !theSigchld &&
	    !theSigalrm) {
	    sigsuspend(&unblockedsigs);
	}

	/* If we got SIGTERM or SIGINT, give up and hang up. */
	if (theSigterm || theSigint) {
	    theSigterm = theSigint = 0;

	    /* If we've already tried to exit this way, use SIGKILL instead
	     * of SIGTERM, because pppd's just being stubborn. */
	    if (dying) {
		sendsig = SIGKILL;
	    } else {
		sendsig = SIGTERM;
	    }
	    dying = TRUE;

	    /* Get the pid of our child pppd. */
	    pppLogicalToPhysical(&pppdPid, device, NULL);

	    /* We don't know what our child pid is.  This is very confusing. */
	    if (!pppdPid) {
		failureExit(35);
	    }

	    /* Die, pppd, die. */
	    kill(pppdPid, sendsig);
	    if (sendsig == SIGKILL) {
		kill(-pppdPid, SIGTERM); /* Give it a chance to die nicely, then
					    kill its whole process group. */
		usleep(2500000);
		kill(-pppdPid, sendsig);
		hangup(ifcfg);
		failureExit(32);
	    }
	}

	/* If we got SIGHUP, reload and redial. */
	if (theSighup) {
	    theSighup = 0;

	    /* Free and reload the configuration structure. */
	    if (ifcfg->parent)
		svCloseFile(ifcfg->parent);
	    svCloseFile(ifcfg);
	    ifcfg = shvarfilesGet(device);

	    /* Get the PID of our child pppd. */
	    pppLogicalToPhysical(&pppdPid, device, NULL);
	    kill(pppdPid, SIGTERM);

	    /* We'll redial when the SIGCHLD arrives, even if PERSIST is
	     * not set (the latter handled by clearing the "we've connected
	     * at least once" flag). */
	    connectedOnce = FALSE;

 	    /* We don't want to delay before redialing, either, so cut
	     * the retry timeout to zero. */
	    timeout = 0;
	}

	/* If we got a SIGIO (from netreport, presumably), check if the
	 * interface is up and return zero (via our parent) if it is. */
	if (theSigio) {
	    theSigio = 0;

	    pppLogicalToPhysical(NULL, device, &physicalDevice);
	    if (physicalDevice) {
		if (interfaceIsUp(physicalDevice)) {
		    /* The interface is up, so report a success to a parent if
		     * we have one.  Any errors after this we just swallow. */
		    relay_exitcode(0);
		    connectedOnce = TRUE;
		    alarm(0);
		}
		free(physicalDevice);
	    }
	}

	/* If we got a SIGCHLD, then pppd died (possibly because we killed it),
	 * and we need to restart it after timeout seconds. */
	if (theSigchld) {
	    theSigchld = 0;

	    /* Find its pid, which is also its process group ID. */
	    waited = waitpid(-1, &status, 0);
	    if (waited == -1) {
		continue;
	    }

	    /* Now, we need to kill any children of pppd still in pppd's
	     * process group, in case they are hanging around.
	     * pppd is dead (we just waited for it) but there is no
	     * guarantee that its children are dead, and they will
	     * hold the modem if we do not get rid of them.
	     * We have kept the old pid/pgrp around in pppdPid.  */
	    if (pppdPid) {
		kill(-pppdPid, SIGTERM); /* give it a chance to die nicely */
		usleep(2500000);
		kill(-pppdPid, SIGKILL);
		hangup(ifcfg);
	    }
	    pppdPid = 0;

	    /* Bail if the child exitted abnormally or we were already
	     * signalled to kill it. */
	    if (!WIFEXITED(status)) {
		failureExit(29);
	    }
	    if (dying) {
		failureExit(WEXITSTATUS(status));
	    }

	    /* Error conditions from which we do not expect to recover
	     * without user intervention -- do not fill up the logs.  */
	    switch (WEXITSTATUS(status)) {
	    case 1: case 2: case 3: case 4: case 6:
	    case 7: case 9: case 14: case 17:
		failureExit(WEXITSTATUS(status));
		break;
	    default:
		break;
	    }

            /* PGB 08/20/02: We no longer retry connecting MAXFAIL
	       times on a failed connect script unless RETRYCONNECT is
	       true. */
	    if ((WEXITSTATUS(status) == 8) &&
		!svTrueValue(ifcfg, "RETRYCONNECT", FALSE)) {
                failureExit(WEXITSTATUS(status));
            }

	    /* If we've never connected, or PERSIST is set, dial again, up
	     * to MAXFAIL times. */
	    if ((WEXITSTATUS(status) == 8) ||
	        !connectedOnce ||
		svTrueValue(ifcfg, "PERSIST", FALSE)) {
		/* If we've been connected (i.e., if we didn't force a redial,
		 * but the connection went down) wait for DISCONNECTTIMEOUT
		 * seconds before redialing. */
		if (connectedOnce) {
		    connectedOnce = FALSE;
		    temp = svGetValue(ifcfg, "DISCONNECTTIMEOUT");
		    if (temp) {
			timeout = atoi(temp);
			free(temp);
		    } else {
			timeout = 2;
		    }
		}
		sigprocmask(SIG_UNBLOCK, &blockedsigs, NULL);
		sleep(timeout);
		sigprocmask(SIG_BLOCK, &blockedsigs, NULL);
		if (!theSigterm &&
		    !theSigint &&
		    !theSighup &&
		    !theSigio &&
		    !theSigchld &&
		    !theSigalrm) {
		    fork_exec(FALSE, IFUP_PPP, "daemon", device, boot);
		}
		/* Reinitialize the retry timeout. */
		temp = svGetValue(ifcfg, "RETRYTIMEOUT");
		if (temp) {
		    timeout = atoi(temp);
		    free(temp);
		} else {
		    timeout = 30;
		}
// Scott Sharkey <ssharkey@linux-no-limits.com>
// MAXFAIL Patch...
		temp = svGetValue(ifcfg, "MAXFAIL");
		if (temp) {
		    maxfail = atoi(temp);
		    free(temp);
		} else {
		    maxfail = 0;
		}
		if ( maxfail != 0 ) {
		    dialCount++;
		    if ( dialCount >= maxfail )
			failureExit(WEXITSTATUS(status));
		} 
	    } else {
		failureExit(WEXITSTATUS(status));
	    }
	}

	/* We timed out, and we're running at boot-time. */
	if (theSigalrm) {
	    failureExit(34);
	}
    }
}
