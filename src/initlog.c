
#include <ctype.h>
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

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#define _(String) gettext((String))

#include <popt.h>

#include <regex.h>

#include "initlog.h"
#include "process.h"

static int logfacility=LOG_DAEMON;
static int logpriority=LOG_NOTICE;
static int reexec=0;
static int quiet=0;
int debug=0;

regex_t  **regList = NULL;

static int logEntries = 0;
struct logInfo *logData = NULL;

void readConfiguration(char *fname) {
    int fd,num=0;
    struct stat sbuf;
    char *data,*line;
    regex_t *regexp;
    int lfac=-1,lpri=-1;
    
    if ((fd=open(fname,O_RDONLY))==-1) return;
    if (fstat(fd,&sbuf)) {
	    close(fd);
	    return;
    }
    data=malloc(sbuf.st_size+1);
    if (read(fd,data,sbuf.st_size)!=sbuf.st_size) {
	    close(fd);
	    return;
    }
    close(fd);
    data[sbuf.st_size] = '\0';
    while ((line=getLine(&data))) {
	if (line[0]=='#') continue;
	if (!strncmp(line,"ignore ",7)) {
	    regexp = malloc(sizeof(regex_t));
	    if (!regcomp(regexp,line+7,REG_EXTENDED|REG_NOSUB|REG_NEWLINE)) {
		regList = realloc(regList,(num+2) * sizeof(regex_t *));
		regList[num] = regexp;
		regList[num+1] = NULL;
		num++;
	    }
	}
	if (!strncmp(line,"facility ",9)) {
	    lfac=atoi(line+9);
	    if ((lfac == 0) && strcmp(line+9,"0")) {
		int x =0;
		
		lfac = LOG_DAEMON;
		for (x=0;facilitynames[x].c_name;x++) {
		    if (!strcmp(line+9,facilitynames[x].c_name)) {
			lfac = facilitynames[x].c_val;
			break;
		    }
		}
	    }
	}
	if (!strncmp(line,"priority ",9)) {
	    lpri = atoi(line+9);
	    if ((lpri == 0) && strcmp(line+9,"0")) {
		int x=0;
		
		lpri = LOG_NOTICE;
		for (x=0;prioritynames[x].c_name;x++) {
		    if (!strcmp(line+9,prioritynames[x].c_name)) {
			lpri = prioritynames[x].c_val;
			break;
		    }
		}
	    }
	}
    }
    if (lfac!=-1) logfacility=lfac;
    if (lpri!=-1) logpriority=lpri;
}
    
char *getLine(char **data) {
    /* Get one line from data */
    /* Anything up to a carraige return (\r) or a backspace (\b) is discarded. */
    /* If this really bothers you, mail me and I might make it configurable. */
    /* It's here to avoid confilcts with fsck's progress bar. */
    char *x, *y;
    
    if (!*data) return NULL;
    x=*data;
    while (*x && (*x != '\n')) {
	while (*x && (*x != '\n') && (*x != '\r') && (*x != '\b')) x++;
	if (*x && (*x=='\r' || *x =='\b')) {
		*data = x+1;
		x++;
	}
    }
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
	if (WIFEXITED(rc)) {
	  DDEBUG("minilogd returned %d!\n",WEXITSTATUS(rc));
	  return WEXITSTATUS(rc);
	}
	else
	  return -1;
    } else {
	int fd;
	
	fd=open("/dev/null",O_RDWR);
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
        close(fd);
	/* kid */
	execlp("minilogd","minilogd",NULL);
	perror("exec");
	exit(-1);
    }
}

int trySocket() {
	int s;
	struct sockaddr_un addr;
 	
	s = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (s<0)
	  return 1;
   
	bzero(&addr,sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path,_PATH_LOG,sizeof(addr.sun_path)-1);

	if (connect(s,(struct sockaddr *) &addr,sizeof(addr))<0) {
		if (errno == EPROTOTYPE) {
			DDEBUG("connect failed (EPROTOTYPE), trying stream\n");
			close(s);
			s = socket(AF_LOCAL, SOCK_STREAM, 0);
			if (connect(s,(struct sockaddr *) &addr, sizeof(addr)) < 0) {
				DDEBUG("connect failed: %s\n",strerror(errno));
				close(s);
				return 1;
			} 
			close(s);
			return 0;
		}
		close(s);
		DDEBUG("connect failed: %s\n",strerror(errno));
		return 1;
	} else {
		close(s);
		return 0;
	}
}

int logLine(struct logInfo *logEnt) {
    /* Logs a line... somewhere. */
    int x;
    struct stat statbuf;
    
    /* Don't log empty or null lines */
    if (!logEnt->line || !strcmp(logEnt->line,"\n")) return 0;
    
	
    if  ( ((stat(_PATH_LOG,&statbuf)==-1) || trySocket())
	  && startDaemon()
	) {
	DDEBUG("starting daemon failed, pooling entry %d\n",logEntries);
	logData=realloc(logData,(logEntries+1)*sizeof(struct logInfo));
	logData[logEntries]= (*logEnt);
	logEntries++;
    } else {
	if (logEntries>0) {
	    for (x=0;x<logEntries;x++) {
		DDEBUG("flushing log entry %d =%s=\n",x,logData[x].line);
		openlog(logData[x].cmd,0,logData[x].fac);
		syslog(logData[x].pri,"%s",logData[x].line);
		closelog();
	    }
	    free(logData);
	    logEntries = 0;
	}
	DDEBUG("logging =%s= via syslog\n",logEnt->line);
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

int processArgs(int argc, char **argv, int silent) {
    char *cmdname=NULL;
    char *conffile=NULL;
    int cmdevent=0;
    char *cmd=NULL;
    char *logstring=NULL;
    char *fac=NULL,*pri=NULL;
    int lfac=-1, lpri=-1;
    poptContext context;
    int rc;
    struct poptOption optTable[] = {
	POPT_AUTOHELP
	{ "conf", 0, POPT_ARG_STRING, &conffile, 0,
	  "configuration file (default: /etc/initlog.conf)", NULL
	},
	{ "name", 'n', POPT_ARG_STRING, &cmdname, 0,
	  "name of service being logged", NULL 
	},
	{ "event", 'e', POPT_ARG_INT, &cmdevent, 0,
	  "event being logged (see man page)", NULL
	},
	{ "cmd", 'c', POPT_ARG_STRING, &cmd, 0,
	  "command to run, logging output", NULL
	},
        { "debug", 'd', POPT_ARG_NONE, &debug, 0,
	  "print lots of verbose debugging info", NULL
	},
	{ "run", 'r', POPT_ARG_STRING, &cmd, 3,
	  "command to run, accepting input on open fd", NULL
	},
	{ "string", 's', POPT_ARG_STRING, &logstring, 0,
	  "string to log", NULL
	},
	{ "facility", 'f', POPT_ARG_STRING, &fac, 1,
	  "facility to log at (default: 'local7')", NULL
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
	    lfac=atoi(fac);
	    if ((lfac == 0) && strcmp(fac,"0")) {
		int x =0;
		
		lfac = LOG_DAEMON;
		for (x=0;facilitynames[x].c_name;x++) {
		    if (!strcmp(fac,facilitynames[x].c_name)) {
			lfac = facilitynames[x].c_val;
			break;
		    }
		}
	    }
	    break;
	 case 2:
	    lpri = atoi(pri);
	    if ((lpri == 0) && strcmp(pri,"0")) {
		int x=0;
		
		lpri = LOG_NOTICE;
		for (x=0;prioritynames[x].c_name;x++) {
		    if (!strcmp(pri,prioritynames[x].c_name)) {
			lpri = prioritynames[x].c_val;
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
       if (!silent)
	 fprintf(stderr, "%s: %s\n",
		poptBadOption(context, POPT_BADOPTION_NOALIAS),
		poptStrerror(rc));
       
	return -1;
    }
    if ( (cmd && logstring) || (cmd && cmdname) ) {
        if (!silent)
	 fprintf(stderr, _("--cmd and --run are incompatible with --string or --name\n"));
	return -1;
    }
    if ( cmdname && (!logstring && !cmdevent)) {
        if (!silent)
	 fprintf(stderr, _("--name requires one of --event or --string\n"));
	return -1;
    }
    if (cmdevent && cmd) {
	    if (!silent)
	      fprintf(stderr, _("--cmd and --run are incompatible with --event\n"));
	    return -1;
    }
    if (conffile) {
	readConfiguration(conffile);
    } else {
	readConfiguration("/etc/initlog.conf");
    }
    if (cmd) {
	    while (isspace(*cmd)) cmd++;
    }
    if (lpri!=-1) logpriority=lpri;
    if (lfac!=-1) logfacility=lfac;
    if (cmdevent) {
	logEvent(cmdname,cmdevent,logstring);
    } else if (logstring) {
	logString(cmdname,logstring);
    } else if ( cmd && *cmd) {
	return(runCommand(cmd,reexec,quiet,debug));
    } else {
        if (!silent)
	 fprintf(stderr,"nothing to do!\n");
	return -1;
    }
   return 0;
}

int main(int argc, char **argv) {

    setlocale(LC_ALL,"");
    bindtextdomain("initlog","/etc/locale");
    textdomain("initlog");
    exit(processArgs(argc,argv,0));
}
