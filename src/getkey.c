/*
 * Copyright (c) 1999-2003 Red Hat, Inc. All rights reserved.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * getkey
 *
 * A very simple keygrabber.
 *
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/poll.h>
#include "popt.h"

struct termios tp;

void reset_term(int x) {
	tcsetattr(0,TCSANOW,&tp);
	exit(x);
}

int main(int argc, char **argv) {
	char foo[2];
	char list[100]; /* should be enough */
	char *waitmessage = NULL;
	char *waitprint, *waitsprint;
	const char *fooptr;
	int waitseconds=0;
	int alarmlen=0;
	int ignore_control=0;
	int tp_if,tp_of,tp_lf;
	int x, r;
	struct pollfd ufds; /* only one, no need for an array... */
        poptContext context;
	struct poptOption options[] = {
            { "wait", 'c', POPT_ARG_INT, &waitseconds, 0, "Number of seconds to wait for keypress", NULL },
	    /*            { "message", 'm', POPT_ARG_STRING, &waitmessage, 0, "Message to print out while waiting for string", "NOTE: argument must have a \"%d\" in it so the number of seconds\nleft until getkey times out can be printed" },*/
            { "message", 'm', POPT_ARG_STRING, &waitmessage, 0, "Message to print out while waiting for string\nNOTE: message must have a \"%d\" in it, to hold the number of seconds left to wait", NULL },
            { "ignore-control-chars", 'i', POPT_ARG_NONE, &ignore_control, 0, "Ignore Control-C and Control-D", NULL },
            POPT_AUTOHELP
            POPT_TABLEEND
        };

	strcpy(list, "");
        context = poptGetContext("getkey", argc, argv, options, 
                                POPT_CONTEXT_POSIXMEHARDER);
        poptSetOtherOptionHelp(context, "[keys]");

        r = poptGetNextOpt(context);
        if (r < -1) {
            fprintf(stderr, "%s: %s\n", 
                   poptBadOption(context, POPT_BADOPTION_NOALIAS),
                   poptStrerror(r));

            return -1;
        }
        fooptr = poptGetArg(context);
	if (fooptr != NULL) {
	    strncpy(list, fooptr, sizeof(list) - 1);
	    list[99] = '\0';
	    for (x=0;list[x];x++) list[x]=toupper(list[x]);
	}
	if (waitseconds) {
	    alarmlen = waitseconds;
	}
	foo[0]=foo[1]='\0';

	signal(SIGTERM,reset_term);
	alarm(alarmlen);
	signal(SIGALRM,reset_term);

	tcgetattr(0,&tp);
	tp_if=tp.c_iflag;
	tp_of=tp.c_oflag;
	tp_lf=tp.c_lflag;
	tp.c_iflag=0;
	tp.c_oflag &= ~OPOST;
	tp.c_lflag &= ~(ISIG | ICANON);
	tcsetattr(0,TCSANOW,&tp);
	tp.c_iflag=tp_if;
	tp.c_oflag=tp_of;
	tp.c_lflag=tp_lf;

	ufds.events = POLLIN;
	ufds.fd = 0;

	if (waitseconds && waitmessage) {
	    waitprint = alloca (strlen(waitmessage)+15); /* long enough */
	    waitprint[0] = '\r';
	    waitsprint = waitprint + 1;
	}

	while (1) {
	    if (waitseconds && waitmessage) {
		sprintf (waitsprint, waitmessage, waitseconds);
		write (1, waitprint, strlen(waitprint));
	    }
	    r = poll(&ufds, 1, alarmlen ? 1000 : -1);
	    if (r == 0) {
		/* we have waited a whole second with no keystroke... */
		waitseconds--;
	    }
	    if (r > 0) {
		read(0,foo,1);
		foo[0]=toupper(foo[0]);
		/* Die if we get a control-c or control-d */
                if (ignore_control == 0) {
		    if (foo[0]==3 || foo[0]==4) reset_term(1);
                }
		/* Don't let a null character be interpreted as a match
		   by strstr */
		if (foo[0] != 0) {
		    if (strcmp(list, "") == 0 || strstr(list,foo)) {
		      reset_term(0);
		    }
		}
	    }
	}
}
