/* 
 * usleep
 * 
 * Written by Donald Barnes <djb@redhat.com> for Red Hat Software.
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
  unsigned long count;
  poptContext optCon;
  int showVersion = 0;
  int rc;
  char * countStr;
  struct poptOption options[] = {
            { "version", 'v', POPT_ARG_NONE, &showVersion, 0, 
			"Display the version of this program, and exit" },
	    POPT_AUTOHELP
            { 0, 0, 0, 0, 0 }
        };

  optCon = poptGetContext("usleep", argc, argv, options,0);
  /*poptReadDefaultConfig(optCon, 1);*/
  poptSetOtherOptionHelp(optCon, "[microseconds]");

  if ((rc = poptGetNextOpt(optCon)) < -1) {
	fprintf(stderr, "usleep: bad argument %s: %s\n", 
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(rc));
	return 2;
  }

  if (showVersion) {
      printf("usleep version 1.1 by Donald Barnes <djb@redhat.com>\n	usleep --help for more info\n");
      return 0;
  }

  countStr = poptGetArg(optCon);
  if (countStr && poptGetArg(optCon)) {
      fprintf(stderr, "usleep: exactly one argument (number of microseconds) "
      		"must be used\n");
      return 2;
  }

  count = strtoul(countStr, NULL, 0); 

  usleep(count);
  return 0;
} 
