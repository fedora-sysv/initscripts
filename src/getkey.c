/*
 * Copyright (c) 1999-2003, 2006 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
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

static struct termios orig_tp;

static void reset_term(int x) {
	tcsetattr(0,TCSANOW,&orig_tp);
	_exit(x);
}

int main(int argc, char **argv) {
        static const char default_list[] = "";

	const char *list;
	char *waitmessage = NULL;
	char *waitprint, *waitsprint;
	int waitseconds=0;
	int alarmlen=0;
	int ignore_control=0;
	struct termios tp;
	int r;
	struct pollfd ufds; /* only one, no need for an array... */
        poptContext context;
	struct poptOption options[] = {
            { "wait", 'c', POPT_ARG_INT, &waitseconds, 0, "Number of seconds to wait for keypress", NULL },
            { "message", 'm', POPT_ARG_STRING, &waitmessage, 0, "Message to print out while waiting for string\nNOTE: The message may have a \"%d\" in it, to hold the number of seconds left to wait.", NULL },
            { "ignore-control-chars", 'i', POPT_ARG_NONE, &ignore_control, 0, "Ignore Control-C and Control-D", NULL },
            POPT_AUTOHELP
            POPT_TABLEEND
        };

	context = poptGetContext("getkey", argc, (const char **)argv, options,
                                POPT_CONTEXT_POSIXMEHARDER);
        poptSetOtherOptionHelp(context, "[keys]");

        r = poptGetNextOpt(context);
        if (r < -1) {
            fprintf(stderr, "%s: %s\n", 
                   poptBadOption(context, POPT_BADOPTION_NOALIAS),
                   poptStrerror(r));

            return -1;
        }
        list = poptGetArg(context);
	if (list != NULL) {
	    char *p;

	    p = strdup(list);
	    list = p;
	    while (*p != 0) {
		*p = toupper(*p);
		p++;
	    }
	} else
	    list = default_list;
	if (waitseconds) {
	    if (waitseconds < 0) {
		fprintf(stderr, "--wait: Invalid time %d seconds\n",
			waitseconds);
		return -1;
	    }
	    alarmlen = waitseconds;
	}

	tcgetattr(0,&tp);
	orig_tp = tp;
	signal(SIGTERM,reset_term);
	if (alarmlen != 0) {
	    signal(SIGALRM,reset_term);
	    alarm(alarmlen);
	}

	tp.c_iflag=0;
	tp.c_oflag &= ~OPOST;
	tp.c_lflag &= ~(ISIG | ICANON);
	tcsetattr(0,TCSANOW,&tp);

	ufds.events = POLLIN;
	ufds.fd = 0;

	if (waitmessage) {
	    waitprint = alloca (strlen(waitmessage)+15); /* long enough */
	    waitprint[0] = '\r';
	    waitsprint = waitprint + 1;
	}

	while (1) {
	    if (waitmessage) {
		sprintf (waitsprint, waitmessage, waitseconds);
		write (1, waitprint, strlen(waitprint));
	    }
	    r = poll(&ufds, 1, alarmlen ? 1000 : -1);
	    if (r == 0) {
		/* we have waited a whole second with no keystroke... */
		waitseconds--;
	    }
	    if (r > 0) {
		char ch;

		read(0, &ch, sizeof(ch));
		ch = toupper(ch);
		/* Die if we get a control-c or control-d */
                if (ignore_control == 0 && (ch == 3 || ch == 4))
		    reset_term(1);
		/* Don't let a null character be interpreted as a match
		   by strchr */
		if (ch != 0
		    && (strcmp(list, "") == 0 || strchr(list, ch) != NULL))
		    reset_term(0);
	    }
	}
}
