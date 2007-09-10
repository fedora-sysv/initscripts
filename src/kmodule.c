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
#include <sys/utsname.h>

#include <kudzu/kudzu.h>


int isAvailable(char *modulename)
{
	struct utsname utsbuf;
	struct stat sbuf;
	char path[512], mod_name[100];
	char *buf;
	int fd;
	
	uname(&utsbuf);
	snprintf(path,512,"/lib/modules/%s/modules.dep",utsbuf.release);
	if (!stat(path,&sbuf)) {
		fd = open(path, O_RDONLY);
		buf = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (!buf || buf == MAP_FAILED)
			return 0;
		close(fd);
		snprintf(mod_name,100,"/%s.ko:", modulename);
		if (strstr(buf, mod_name)) {
			munmap(buf, sbuf.st_size);
			return 1;
		}
		snprintf(mod_name,100,"/%s.ko.gz:", modulename);
		if (strstr(buf, mod_name)) {
			munmap(buf, sbuf.st_size);
			return 1;
		}
		munmap(buf,sbuf.st_size);
	}
	return 0;
}


int main(int argc, char **argv) 
{
	char *bus = NULL, *class = NULL;
	int x, rc;
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

	devs = probeDevices(probeClass, probeBus, PROBE_NOLOAD|PROBE_SAFE);
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
	return 0;
}
