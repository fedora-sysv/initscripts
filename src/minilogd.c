
/* minilogd.c
 * 
 * A pale imitation of syslogd. Most notably, doesn't write anything
 * anywhere except possibly back to syslogd.
 * 
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

static int we_own_log=0;
static char **buffer=NULL;
static int buflines=0;

void freeBuffer() {
   struct sockaddr_un addr;
   int sock;
   int x=0,conn;
   
   bzero(&addr,sizeof(addr));
   addr.sun_family = AF_LOCAL;
   strncpy(addr.sun_path,_PATH_LOG,sizeof(addr.sun_path)-1);
   /* wait for klogd to hit syslog */
   sleep(1);
   sock = socket(AF_LOCAL, SOCK_STREAM,0);
   conn=connect(sock,(struct sockaddr *) &addr,sizeof(addr));
   while (x<buflines) {
      if (!conn) write(sock,buffer[x],strlen(buffer[x])+1);
      free(buffer[x]);
      x++;
   }
}

void cleanup(int exitcode) {
   /* If we own the log, unlink it before trying to free our buffer.
    * Otherwise, sending the buffer to /dev/log doesn't make much sense.... */
   if (we_own_log) {
      perror("wol");
      unlink(_PATH_LOG);
   }
   /* Don't try to free buffer if we were called from a signal handler */
   perror("foo");
   if (exitcode<=0) {
       if (buffer) freeBuffer();
       exit(exitcode);
   } else
      exit(exitcode+128);
}

void runDaemon(int sock) {
   struct sockaddr_un addr;
   int x,len,addrlen,recvsock,done=0;
   char *message;
   struct stat s1,s2;
   struct pollfd pfds;
    FILE *foo;
    
    foo=fopen("./foo","a");
	
    daemon(0,-1);
    /* try not to leave stale sockets lying around */
    /* Hopefully, we won't actually get any of these */
    signal(SIGHUP,cleanup);
    signal(SIGINT,cleanup);
    signal(SIGQUIT,cleanup);
    signal(SIGILL,cleanup);
    signal(SIGABRT,cleanup);
    signal(SIGFPE,cleanup);
    signal(SIGSEGV,cleanup);
    signal(SIGPIPE,cleanup);
    signal(SIGBUS,cleanup);
    signal(SIGTERM,cleanup);
   done = 0;
   /* Get stat info on dev log so we can later check to make sure we
    * still own it... */
   stat(_PATH_LOG,&s1);
   while (!done) {
      pfds.fd = sock;
      pfds.events = POLLIN|POLLPRI;
      if ( ( (x=poll(&pfds,1,500))==-1) && errno !=EINTR) {
	 perror("poll");
	 cleanup(-1);
      }
      if ( (x>0) && pfds.revents & (POLLIN | POLLPRI)) {
	 printf("foo!\n");
	 message = calloc(8192,sizeof(char));
	 recvsock = accept(sock,(struct sockaddr *) &addr, &addrlen);
	 len = read(recvsock,message,8192);
	 if (buffer)
	   buffer = realloc(buffer,(buflines+1)*sizeof(char *));
	 else
	   buffer = malloc(sizeof(char *));
	 if (len>0) {
	     message[strlen(message)]='\n';
	     fprintf(foo,"%s",message);
	    buffer[buflines]=message;
	    buflines++;
	    close(recvsock);
	 }
	 else {
	    close(recvsock);
	    recvsock=-1;
	 }
      }
      if ( (x>0) && ( pfds.revents & (POLLHUP | POLLNVAL)) )
	done = 1;
      /* Check to see if syslogd's yanked our socket out from under us */
      if ( (stat(_PATH_LOG,&s2)!=0) ||
	    (s1.st_ino != s2.st_ino ) || (s1.st_ctime != s2.st_ctime) ||
	    (s1.st_mtime != s2.st_mtime) || (s1.st_atime != s2.st_atime) ) {
	 done = 1;
	 we_own_log = 0;
      }
   }
    fclose(foo);
   cleanup(0);
}

int main() {
   struct sockaddr_un addr;
   int sock;
   int pid;
    
   /* just in case */
   sock = open("/dev/null",O_RDWR);
   dup2(sock,0);
   dup2(sock,1);
   dup2(sock,2);
	
   bzero(&addr, sizeof(addr));
   addr.sun_family = AF_LOCAL;
   strncpy(addr.sun_path,_PATH_LOG,sizeof(addr.sun_path)-1);
   sock = socket(AF_LOCAL, SOCK_STREAM,0);
   unlink(_PATH_LOG);
   /* Bind socket before forking, so we know if the server started */
   if (!bind(sock,(struct sockaddr *) &addr, sizeof(addr))) {
      we_own_log = 1;
      listen(sock,5);
      if ((pid=fork())==-1) {
	 perror("fork");
	 exit(-1);
      }
      if (pid) {
	 exit(0);
      } else {
	  runDaemon(sock);
	  /* shouldn't get back here... */
	  exit(-1);
      }
   } else {
      exit(-1);
   }
}
