
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/poll.h>

/* A very simple keygrabber. */

struct termios tp;

void reset_term(int x) {
	tcsetattr(0,TCSANOW,&tp);
	exit(x);
}

int main(int argc, char **argv) {
	char foo[2];
	char *list = NULL;
	char *waitmessage = NULL;
	char *waitprint, *waitsprint;
	int waitseconds=0;
	int alarmlen=0;
	int tp_if,tp_of,tp_lf;
	int x, r;
	struct pollfd ufds; /* only one, no need for an array... */
	
	for (++argv, --argc; argc; argc--, argv++) {
	    if (argv[0][0]=='-') {
		if (argv[0][1]=='c') {
		    argc--; argv++;
		    waitseconds = atoi(argv[0]);
		} else if (argv[0][1]=='m') {
		    argc--; argv++;
		    waitmessage=argv[0];
		} else if (isdigit(argv[0][1])) {
		    waitseconds = atoi(argv[0]);
		}
	    } else {
		list = argv[0];
		for (x=0;list[x];x++) list[x]=toupper(list[x]);
	    }
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
	    if (waitseconds) {
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
		if (foo[0]==3 || foo[0]==4) reset_term(1);
		if ((!list) || strstr(list,foo)) {
			reset_term(0);
		}
	    }
	}
}
