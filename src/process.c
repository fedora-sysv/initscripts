
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/signal.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <popt.h>

#include <regex.h>

#include "initlog.h"
#include "process.h"

extern regex_t **regList;

int forkCommand(char **args, int *outfd, int *errfd, int *cmdfd, int quiet) {
   /* Fork command 'cmd', returning pid, and optionally pointer
    * to open file descriptor fd */
    int fdout, fderr, fdcmd, pid;
    int outpipe[2], errpipe[2], fdpipe[2];
    int ourpid;
    
    if ( (pipe(outpipe)==-1) || (pipe(errpipe)==-1) || (pipe(fdpipe)==-1) ) {
	perror("pipe");
	return -1;
    }
    
    if (outfd) {
       fdout = outpipe[1];
      *outfd = outpipe[0];
    } else {
       if (!quiet)
	 fdout=dup(1);
    }
    if (errfd) {
       fderr = errpipe[1];
      *errfd = errpipe[0];
    } else {
       if (!quiet)
	 fderr=dup(2);
    }
    
    if (cmdfd) {
	*cmdfd = fdpipe[0];
	fdcmd = fdpipe[1];
    } else {
        fdcmd = open("/dev/null",O_WRONLY);
    }
    ourpid = getpid();
    if ((pid = fork())==-1) {
	perror("fork");
	return -1;
    }
    /* We exec the command normally as the child. However, if we're getting passed
     * back arguments via an fd, we'll exec it as the parent. Therefore, if Bill
     * fucks up and we segfault or something, we don't kill rc.sysinit. */
    if ( (cmdfd&&!pid) || (pid &&!cmdfd)) {
	/* parent */
	close(fdout);
	close(fderr);
	close(fdcmd);
	if (!pid)
	  return ourpid;
	else
	  return pid;
    } else {
	/* kid */
       int sc_open_max;

       if (outfd) { 
	 if ( (dup2(fdout,1)==-1) ) {
	    perror("dup2");
	    exit(-1);
	 }
       } else if (quiet)
	    if ((dup2(open("/dev/null",O_WRONLY),1))==-1) {
	     perror("dup2");
	     exit(-1);
	    }

       if (errfd)  {
	 if ((dup2(fderr,2)==-1)) {
	    perror("dup2");
	    exit(-1);
	 }
       } else if (quiet) 
	    if ((dup2(open("/dev/null",O_WRONLY),2))==-1)  {
	       perror("dup2");
	       exit(-1);
	    }

 
       if ((dup2(fdcmd,CMD_FD)==-1)) {
	    perror("dup2");
	    exit(-1);
	}
	close(fdout);
	close(fderr);
	close(fdcmd);
	if (outfd)
	  close(*outfd);
	if (errfd)
	  close(*errfd);
	if (cmdfd)
	  close(*cmdfd);

        /* close up extra fds, and hope this doesn't break anything */
	sc_open_max = sysconf(_SC_OPEN_MAX);
	if(sc_open_max > 1) {
	    int fd;
	    for(fd = 3; fd < sc_open_max; fd++) {
		    if (!(cmdfd && fd == CMD_FD))
		      close(fd);
	    }
	}

	execvp(args[0],args);
	perror("execvp");
	exit(-1);
    }
}

int monitor(char *cmdname, int pid, int numfds, int *fds, int reexec, int quiet, int debug) {
    struct pollfd *pfds;
    char *buf=malloc(8192*sizeof(char));
    char *outbuf=NULL;
    char *tmpstr=NULL;
    int x,y,rc=-1;
    int done=0;
    int output=0;
    char **cmdargs=NULL;
    char **tmpargs=NULL;
    int cmdargc;
    char *procpath = NULL;
    
    if (reexec) {
	procpath=malloc(20*sizeof(char));
	snprintf(procpath,20,"/proc/%d",pid);
    }
    
    pfds = malloc(numfds*sizeof(struct pollfd));
    for (x=0;x<numfds;x++) {
	pfds[x].fd = fds[x];
	pfds[x].events = POLLIN | POLLPRI;
    }
	
    while (!done) {
       usleep(500);
       if (((x=poll(pfds,numfds,500))==-1)&&errno!=EINTR) {
	  perror("poll");
	  return -1;
       }
       if (!reexec) {
	  if (waitpid(pid,&rc,WNOHANG))
	    done=1;
       } else {
	   struct stat sbuf;
	   /* if /proc/pid ain't there and /proc is, it's dead... */
	   if (stat(procpath,&sbuf)&&!stat("/proc/cpuinfo",&sbuf))
	     done=1;
       }
       y=0;
       while (y<numfds) {
	  if ( x && ((pfds[y].revents & (POLLIN | POLLPRI)) )) {
	     int bytesread = 0;
	     
	     do {
		buf=calloc(8192,sizeof(char));
		bytesread = read(pfds[y].fd,buf,8192);
		if (bytesread==-1) {
		   perror("read");
		   return -1;
		}
		if (bytesread) {
		  if (!quiet && !reexec)
		    write(1,buf,bytesread);
		  if (quiet) {
			  outbuf=realloc(outbuf,(outbuf ? strlen(outbuf)+bytesread+1 : bytesread+1));
			  if (!output) outbuf[0]='\0';
			  strcat(outbuf,buf);
			  output = 1;
		  }
		  while ((tmpstr=getLine(&buf))) {
		      int ignore=0;
		      
		      if (regList) {
			  int count=0;
			 
			  while (regList[count]) {
			      if (!regexec(regList[count],tmpstr,0,NULL,0)) {
				  ignore=1;
				  break;
			      }
			      count++;
			  }
		      }
		      if (!ignore) {
			  if (!reexec) {
			      if (getenv("IN_INITLOG")) {
				  char *buffer=calloc(8192,sizeof(char));
				  DDEBUG("sending =%s= to initlog parent\n",tmpstr);
				  snprintf(buffer,8192,"-n %s -s \"%s\"\n",
					   cmdname,tmpstr);
				  /* don't blow up if parent isn't there */
				  signal(SIGPIPE,SIG_IGN);
				  write(CMD_FD,buffer,strlen(buffer));
				  signal(SIGPIPE,SIG_DFL);
				  free(buffer);
			      } else {
				  logString(cmdname,tmpstr);
			      }
			  } else {
			      int z; 
			
			      cmdargs=NULL;
			      tmpargs=NULL;
			      cmdargc=0;
			      
			      poptParseArgvString(tmpstr,&cmdargc,&tmpargs);
			      cmdargs=malloc( (cmdargc+2) * sizeof(char *) );
			      cmdargs[0]=strdup("initlog");
			      for (z=0;z<(cmdargc);z++) {
				  cmdargs[z+1]=tmpargs[z];
			      }
			      cmdargs[cmdargc+1]=NULL;
			      processArgs(cmdargc+1,cmdargs,1);
			  }
		      }
		  }
		}
	     } while ( bytesread==8192 );
	  }
	  y++;
       }
    }
    if ((!WIFEXITED(rc)) || (rc=WEXITSTATUS(rc))) {
      /* If there was an error and we're quiet, be loud */
      
      if (quiet && output) {
	    write(1,outbuf,strlen(outbuf));
      }
      return (rc);
   }
   return 0;
}

int runCommand(char *cmd, int reexec, int quiet, int debug) {
    int fds[2];
    int pid,x;
    char **args, **tmpargs;
    char *cmdname;
    
    poptParseArgvString(cmd,&x,&tmpargs);
    args = malloc((x+1)*sizeof(char *));
    for ( pid = 0; pid < x ; pid++) {
	args[pid] = strdup(tmpargs[pid]);
    }
    args[pid] = NULL;
    if (strcmp(args[0],"sh") && strcmp(args[0],"/bin/sh")) 
      cmdname = basename(args[0]);
    else
      cmdname = basename(args[1]);
    if ((cmdname[0] =='K' || cmdname[0] == 'S') && ( '0' <= cmdname[1] <= '9' )
       && ( '0' <= cmdname[2] <= '9' ) )
      cmdname+=3;
    if (!reexec) {
       pid=forkCommand(args,&fds[0],&fds[1],NULL,quiet);
       x=monitor(cmdname,pid,2,fds,reexec,quiet,debug);
    } else {
       setenv("IN_INITLOG","yes",1);
       pid=forkCommand(args,NULL,NULL,&fds[0],quiet);
       unsetenv("IN_INITLOG");
       x=monitor(cmdname,pid,1,&fds[0],reexec,quiet,debug);
    }
    return x;
}
