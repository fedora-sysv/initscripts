
/* Change the default console loglevel */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv) {
   int level;

   if (!argv[1]) exit(0);
   level=atoi(argv[1]);
   if ( (level<1) || (level>8) ) {
      fprintf(stderr,"invalid log level %d\n",level);
      exit(-1);
   }
   if (!syscall(SYS_syslog,8,NULL,level)) {
      exit(0);
   } else {
      perror("syslog");
      exit(-1);
   }
}
