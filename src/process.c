
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/poll.h>
#include <sys/wait.h>

#include <popt.h>

#include "initlog.h"
#include "process.h"

int forkCommand(char **args, int *outfd, int *errfd, int *cmdfd, int quiet) {
   /* Fork command 'cmd', returning pid, and optionally pointer
    * to open file descriptor fd */
    int fdin, fdout, fderr, fdcmd, pid;
    int outpipe[2], errpipe[2], fdpipe[2];
    
    if ( (pipe(outpipe)==-1) || (pipe(errpipe)==-1) || (pipe(fdpipe)==-1) ) {
	perror("pipe");
	return -1;
    }
    
    fdin=dup(0);
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
    fdcmd = fdpipe[1];
    if (cmdfd)
      *cmdfd = fdpipe[0];
    if ((pid = fork())==-1) {
	perror("fork");
	return -1;
    }
    if (pid) {
	/* parent */
	close(fdin);
	close(fdout);
	close(fderr);
	close(fdcmd);
	return pid;
    } else {
	/* kid */
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
	execvp(args[0],args);
	perror("execvp");
	exit(-1);
    }
}

int monitor(char *cmdname, int pid, int numfds, int *fds, int reexec, int quiet) {
    struct pollfd *pfds;
    char *buf=malloc(2048*sizeof(char));
    int outpipe[2];
    char *tmpstr=NULL;
    int x,y,rc=-1;
    int done=0;
    int output=0;
    
    pipe(outpipe);
   
    pfds = malloc(numfds*sizeof(struct pollfd));
    for (x=0;x<numfds;x++) {
	pfds[x].fd = fds[x];
	pfds[x].events = POLLIN | POLLPRI;
    }
	
    while (!done) {
       if (((x=poll(pfds,numfds,500))==-1)&&errno!=EINTR) {
	  perror("poll");
	  return -1;
       }
       if (waitpid(pid,&rc,WNOHANG))
	 done=1;
       y=0;
       while (y<numfds) {
	  if ( x && ((pfds[y].revents & (POLLIN | POLLPRI)) )) {
	     int bytesread = 0;
	     
	     do {
		buf=calloc(2048,sizeof(char));
		bytesread = read(pfds[y].fd,buf,2048);
		if (bytesread==-1) {
		   perror("read");
		   return -1;
		}
		if (bytesread) {
		  if (!quiet && !reexec)
		    write(1,buf,bytesread);
		  if (quiet) {
		     output = 1;
		     write(outpipe[1],buf,bytesread);
		  }
		  while ((tmpstr=getLine(&buf))) {
		     if (!reexec) {
			 if (getenv("IN_INITLOG")) {
			     char *buffer=calloc(2048,sizeof(char));
			     snprintf(buffer,2048,"-n %s -s \"%s\"",
				      cmdname,tmpstr);
			     write(CMD_FD,buffer,strlen(buffer));
			     free(buffer);
			 } else {
			     logString(cmdname,tmpstr);
			 }
		     }
		     else {
			char **cmdargs=NULL;
			char **tmpargs=NULL;
			int cmdargc,x;
			
			poptParseArgvString(tmpstr,&cmdargc,&tmpargs);
			cmdargs=malloc( (cmdargc++) * sizeof(char *) );
			cmdargs[0]=strdup("initlog");
			for (x=0;x<(cmdargc-1);x++) {
			   cmdargs[x+1]=tmpargs[x];
			}
			processArgs(cmdargc,cmdargs);
		     }
		  }
		}
	        
	     } while ( bytesread==2048 );
	  }
	  y++;
       }
    }
    if ((!WIFEXITED(rc)) || (rc=WEXITSTATUS(rc))) {
      /* If there was an error and we're quiet, be loud */
      int x;
      
      if (quiet && output) {
	 buf=calloc(2048,sizeof(char));
	 do {
	    x=read(outpipe[0],buf,2048);
	    write(1,"\n",1);
	    write(1,buf,x);
	    buf=calloc(2048,sizeof(char));
	 } while (x==2048);
      }
      return (rc);
   }
   return 0;
}

int runCommand(char *cmd, int reexec, int quiet) {
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
    if ((cmdname[0] =='K' || cmdname[0] == 'S') && ( 30 <= cmdname[1] <= 39 )
       && ( 30 <= cmdname[2] <= 39 ) )
      cmdname+=3;
    if (!reexec) {
       pid=forkCommand(args,&fds[0],&fds[1],NULL,quiet);
       x=monitor(cmdname,pid,2,fds,reexec,quiet);
    } else {
       setenv("IN_INITLOG","yes",1);
       pid=forkCommand(args,NULL,NULL,&fds[0],quiet);
       unsetenv("IN_INITLOG");
       x=monitor(cmdname,pid,1,&fds[0],reexec,quiet);
    }
    return x;
}
