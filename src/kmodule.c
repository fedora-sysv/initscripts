/* Copyright 2004 Red Hat, Inc.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <popt.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>

#include <kudzu/kudzu.h>

char *setupFile()
{
	struct stat sbuf;
	char path[512];
	int fd;
	struct utsname utsbuf;
	char *buf = NULL;
	
	uname(&utsbuf);
	snprintf(path,512,"/lib/modules/%s/modules.dep",utsbuf.release);
	if (!stat(path,&sbuf)) {
		fd = open(path,O_RDONLY);
		buf =  mmap(0,sbuf.st_size,PROT_READ,MAP_SHARED,fd,0);
		close(fd);
	}
	return buf;
}

int isAvailable(char *modulename)
{
	char mod_name[100];
	static char *buf = NULL;
	
	if (!buf) {
		buf = setupFile();
		if (!buf)
			return 0;
	}
	snprintf(mod_name,100,"/%s.ko:",modulename);
	if (strstr(buf,mod_name))
		return 1;
	snprintf(mod_name,100,"/%s.ko.gz:",modulename);
	if (strstr(buf,mod_name))
		return 1;
	return 0;
}

char *dumpDevices(struct device **devlist)
{
	int fds[2],x;
	FILE *tmp;
	char *buf = NULL;

	pipe(fds);
	tmp = fdopen(fds[1],"w");
	
	buf = NULL;
	for (x = 0; devlist[x]; x++) {
		char b[4096];
		
		devlist[x]->writeDevice(tmp,devlist[x]);
		fflush(tmp);
		memset(b,'\0',4096);
		while (read(fds[0],b,4096)) {
			if (!buf) {
				buf = calloc(strlen(b)+1,sizeof(char));
				strcpy(buf,b);
				buf[strlen(b)] = '\0';
			} else {
				buf = realloc(buf, strlen(buf)+strlen(b)+1);
				sprintf(buf,"%s%s",buf,b);
				buf[strlen(buf)+strlen(b)] = '\0';
			}
			if (strlen(b) != 4096)
				break;
		}
	}
	close(fds[0]);
	close(fds[1]);
	return buf;
}

void waitForConnection(char *buf)
{
	int sock, fd, socklen;
	struct sockaddr_un addr;
	
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) return;
        memset(addr.sun_path,'\0',sizeof(addr.sun_path));
	sprintf(addr.sun_path, "akudzu_config_socket");
	addr.sun_family= AF_UNIX;
	addr.sun_path[0] = '\0';
	if (bind(sock, &addr, sizeof(struct sockaddr_un)) == -1)
		return;
	if (listen(sock, 1) == -1)
		return;
	fd = accept(sock, &addr, &socklen);
	if (fd == -1)
		return;
	write(fd,buf,strlen(buf));
	close(fd);
	close(sock);
}

int main(int argc, char **argv) 
{
	char *bus = NULL, *class = NULL, *buf = NULL;
	int x, rc, isdaemon = 0;
	enum deviceBus probeBus = BUS_UNSPEC & ~BUS_SERIAL;
	enum deviceClass probeClass = CLASS_UNSPEC;
	poptContext context;
	struct device **devs;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{ "bus", 'b', POPT_ARG_STRING, &bus, 0,
		  "probe only the specified 'bus'",
			NULL
		},
		{ "class", 'c', POPT_ARG_STRING, &class, 0,
			"probe only for the specified 'class'",
			NULL
		},
		{ "daemon", 'd', POPT_ARG_NONE, &isdaemon, 0,
			NULL, NULL
		},
		{ 0, 0, 0, 0, 0, 0 }
	};

	context = poptGetContext("kmodule", argc, (const char **)argv, options, 0);
	while ((rc = poptGetNextOpt(context)) > 0) {
	}
	if (( rc < -1)) {
		fprintf(stderr, "%s: %s\n",
			poptBadOption(context, POPT_BADOPTION_NOALIAS),
			poptStrerror(rc));
		exit(-1);
	}
	
	if (bus) {
		for (x=0; bus[x]; x++)
		  bus[x] = toupper(bus[x]);
		for (x=0; buses[x].string && strcmp(buses[x].string,bus); x++);
		if (buses[x].string)
		  probeBus = buses[x].busType;
	}
	if (class) {
		for (x=0; class[x]; x++)
		  class[x] = toupper(class[x]);
		for (x=0; classes[x].string && strcmp(classes[x].string,class); x++);
		if (classes[x].string)
		  probeClass = classes[x].classType;
	}
	initializeBusDeviceList(probeBus);

	devs = probeDevices(probeClass, probeBus, PROBE_ALL|PROBE_NOLOAD|PROBE_SAFE);
	if (!devs)
		return 0;
	for (x = 0; devs[x]; x++) {
		if (devs[x]->driver && isAvailable(devs[x]->driver)) {
			int i;
			
			for (i = 0; classes[i].classType; i++)
				if (devs[x]->type == classes[i].classType) {
					break;
				}
			printf("%s %s\n",classes[i].string,devs[x]->driver);
		}
	}
	if (isdaemon) {
		buf = dumpDevices(devs);
		daemon(0,0);
		waitForConnection(buf);
	}
	return 0;
}
