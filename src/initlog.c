
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSLOG_NAMES
#include <syslog.h>

#include <sys/wait.h>

#define _(String) gettext((String))

#include <popt.h>

#include "initlog.h"
#include "process.h"

static int logfacility=LOG_DAEMON;
static int logpriority=LOG_NOTICE;
static int reexec=0;
static int quiet=0;

static int logEntries = 0;
struct logInfo *logData = NULL;

char *getLine(char **data) {
    /* Get one line from data */
    char *x, *y;
    
    if (!*data) return NULL;
    
    for (x = *data; *x && (*x != '\n'); x++);
    if (*x) {
	x++;
    } else {
	if (x-*data) {
	    y=malloc(x-*data+1);
	    y[x-*data] = 0;
	    y[x-*data-1] = '\n';
	    memcpy(y,*data,x-*data);
	} else {
	    y=NULL;
	}
	*data = NULL;
	return y;
    }
    y = malloc(x-*data);
    y[x-*data-1] = 0;
    memcpy(y,*data,x-*data-1);
    *data = x;
    return y;
}

char **toArray(char *line, int *num) {
    /* Converts a long string into an array of lines. */
    char **lines;
    char *tmpline;
    
    *num = 0;
    lines = NULL;
    
    while ((tmpline=getLine(&line))) {
	if (!*num)
	  lines = (char **) malloc(sizeof(char *));
	else
	  lines = (char **) realloc(lines, (*num+1)*sizeof(char *));
	lines[*num] = tmpline;
	(*num)++;
    }
    return lines;
}

int startDaemon() {
    int pid;
    int rc;
    
    if ( (pid = fork()) == -1 ) {
	perror("fork");
	return -1;
    }
    if ( pid ) {
	/* parent */
	waitpid(pid,&rc,0);
        if (rc)
	 return -1;
        else 
	 return 0;
    } else {
	int fd;
	
	fd=open("/dev/null",O_RDWR);
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
	/* kid */
	execlp("minilogd","minilogd",NULL);
	perror("exec");
	exit(-1);
    }
}

int logLine(struct logInfo *logEnt) {
    /* Logs a line... somewhere. */
    int x;
    
    /* Don't log empty or null lines */
    if (!logEnt->line || !strcmp(logEnt->line,"\n")) return 0;

    if ((x=access(_PATH_LOG,W_OK))) {
       /* syslog isn't running, so start something... */
       if ( (x=startDaemon()) ==-1) {
	    logData=realloc(logData,(logEntries+1)*sizeof(struct logInfo));
	    logData[logEntries]= (*logEnt);
	    logEntries++;
	} else {
	   if (logEntries>0) {
	      for (x=0;x<logEntries;x++) {
		 openlog(logData[x].cmd,0,logData[x].fac);
		 printf("flushing %s\n",logData[x].line);
		 syslog(logData[x].pri,"%s",logData[x].line);
		 closelog();
	      }
	      free(logData);
	      logEntries = 0;
	   }
	   openlog(logEnt->cmd,0,logEnt->fac);
	   syslog(logEnt->pri,"%s",logEnt->line);
	   closelog();
	}
    } else {
       if (logEntries>0) {
	  for (x=0;x<logEntries;x++) {
	     openlog(logData[x].cmd,0,logData[x].fac);
	     printf("flushing %s\n",logData[x].line);
	     syslog(logData[x].pri,"%s",logData[x].line);
	     closelog();
	  }
	  free(logData);
	  logEntries = 0;
       }
       openlog(logEnt->cmd,0,logEnt->fac);
       syslog(logEnt->pri,"%s",logEnt->line);
       closelog();
    }
    return 0;
}

int logEvent(char *cmd, int eventtype,char *string) {
    char *eventtable [] = {
	_("%s babbles incoherently"),
	_("%s succeeded"),
	_("%s failed"),
	_("%s cancelled at user request"),
	_("%s failed due to a failed dependency"),
	/* insert more here */
	NULL
    };
    int x=0,len;
    struct logInfo logentry;
    
    
    if (cmd) {
	logentry.cmd = strdup(basename(cmd));
	if ((logentry.cmd[0] =='K' || logentry.cmd[0] == 'S') && ( 30 <= logentry.cmd[1] <= 39 )
	    && ( 30 <= logentry.cmd[2] <= 39 ) )
	  logentry.cmd+=3;
    } else
      logentry.cmd = strdup(_("(none)"));
    if (!string)
      string = strdup(cmd);
    
    while (eventtable[x] && x<eventtype) x++;
    if (!(eventtable[x])) x=0;
    
    len=strlen(eventtable[x])+strlen(string);
    logentry.line=malloc(len);
    snprintf(logentry.line,len,eventtable[x],string);
    
    logentry.pri = logpriority;
    logentry.fac = logfacility;
    
    return logLine(&logentry);
}

int logString(char *cmd, char *string) {
    struct logInfo logentry;
    
    if (cmd) {
	logentry.cmd = strdup(basename(cmd));
	if ((logentry.cmd[0] =='K' || logentry.cmd[0] == 'S') && ( 30 <= logentry.cmd[1] <= 39 )
	    && ( 30 <= logentry.cmd[2] <= 39 ) )
	  logentry.cmd+=3;
    } else
      logentry.cmd = strdup(_(""));
    logentry.line = strdup(string);
    logentry.pri = logpriority;
    logentry.fac = logfacility;
    
    return logLine(&logentry);
}

void processArgs(int argc, char **argv) {
    char *cmdname=NULL;
    int cmdevent=0;
    char *cmd=NULL;
    char *logstring=NULL;
    char *fac=NULL,*pri=NULL;
    poptContext context;
    int rc;
    struct poptOption optTable[] = {
	POPT_AUTOHELP
	{ "name", 'n', POPT_ARG_STRING, &cmdname, 0,
	  "name of service being logged", NULL 
	},
	{ "event", 'e', POPT_ARG_INT, &cmdevent, 0,
	  "event being logged (see man page)", NULL
	},
	{ "cmd", 'c', POPT_ARG_STRING, &cmd, 0,
	  "command to run, logging output", NULL
	},
	{ "run", 'r', POPT_ARG_STRING, &cmd, 3,
	  "command to run, accepting input on open fd", NULL
	},
	{ "string", 's', POPT_ARG_STRING, &logstring, 0,
	  "string to log", NULL
	},
	{ "facility", 'f', POPT_ARG_STRING, &fac, 1,
	  "facility to log at (default: 'daemon')", NULL
	},
	{ "priority", 'p', POPT_ARG_STRING, &pri, 2,
	  "priority to log at (default: 'notice')", NULL
	},
        { "quiet", 'q', POPT_ARG_NONE, &quiet, 0,
	  "suppress stdout/stderr", NULL
	},
        { 0, 0, 0, 0, 0, 0 }
    };
    
    context = poptGetContext("initlog", argc, argv, optTable, 0);
    
    while ((rc = poptGetNextOpt(context)) > 0) {
	switch (rc) {
	 case 1:
	    logfacility=atoi(fac);
	    if ((logfacility == 0) && strcmp(fac,"0")) {
		int x =0;
		
		logfacility = LOG_DAEMON;
		for (x=0;facilitynames[x].c_name;x++) {
		    if (!strcmp(fac,facilitynames[x].c_name)) {
			logfacility = facilitynames[x].c_val;
			break;
		    }
		}
	    }
	    break;
	 case 2:
	    logpriority = atoi(pri);
	    if ((logpriority == 0) && strcmp(pri,"0")) {
		int x=0;
		
		logpriority = LOG_NOTICE;
		for (x=0;prioritynames[x].c_name;x++) {
		    if (!strcmp(pri,prioritynames[x].c_name)) {
			logpriority = prioritynames[x].c_val;
			break;
		    }
		}
	    }
	    break;
	 case 3:
	    reexec = 1;
	    break;
	 default:
	    break;
	}
    }
      
    if ((rc < -1)) {
	fprintf(stderr, "%s: %s\n",
		poptBadOption(context, POPT_BADOPTION_NOALIAS),
		poptStrerror(rc));
	exit(-1);
    }
    if ( (cmd && logstring) || (cmd && cmdname) ) {
	fprintf(stderr, _("--cmd and --run are incompatible with --string or --name\n"));
	exit(-1);
    }
    if ( cmdname && (!logstring && !cmdevent)) {
	fprintf(stderr, _("--name requires one of --event or --string\n"));
	exit(-1);
    }
    if (cmdevent) {
	logEvent(cmdname,cmdevent,logstring);
    } else if (logstring) {
	logString(cmdname,logstring);
    } else if ( cmd ) {
	exit(runCommand(cmd,reexec,quiet));
    } else {
	fprintf(stderr,"nothing to do!\n");
	exit(-1);
    }
}

int main(int argc, char **argv) {

    setlocale(LC_ALL,"");
    bindtextdomain("initlog","/etc/locale");
    textdomain("initlog");
    processArgs(argc,argv);
    exit (0);
}
