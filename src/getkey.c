
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* A very simple keygrabber. */

struct termios tp;

void reset_term(int x) {
	tcsetattr(0,TCSANOW,&tp);
	exit(x);
}

int main(int argc, char **argv) {
	char foo[2];
	char *list = NULL;
	int alarmlen=0;
	int tp_if,tp_of,tp_lf;
	int x;
	
	if (argc>1) {
		if (argv[1][0]=='-' && isdigit(argv[1][1])) {
			alarmlen = atoi(argv[1]+1);
			list = argv[2];
		} else {
			list = argv[1];
		}
		for (x=0;list[x];x++) list[x]=toupper(list[x]);
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
	while (1) {
		read(0,foo,1);
		foo[0]=toupper(foo[0]);
		/* Die if we get a control-c or control-d */
		if (foo[0]==3 || foo[0]==4) reset_term(1);
		if ((!list) || strstr(list,foo)) {
			reset_term(0);
		}
	}
}
