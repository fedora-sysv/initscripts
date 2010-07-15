
/* 
 * rename_device.c: udev helper to rename ethernet devices.
 *
 * Copyright (C) 2006-2009 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions of the
 * GNU General Public License v.2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/sockios.h>

#include <net/if.h>

#include <glib.h>

#define LOCKFILE "/dev/.rename_device.lock"

void sighandler(int dummy) {
	unlink(LOCKFILE);
	_exit(1);
}

struct netdev {
	char *hwaddr;
	char *dev;
	struct netdev *next;
};

struct tmp {
	char *src;
	char *target;
	struct tmp *next;
};

struct netdev *configs = NULL;
struct tmp *tmplist = NULL;

#if defined(__s390__) || defined(__s390x__)
static int is_cdev(const struct dirent *dent) {
        char *end = NULL;
        
        if (strncmp(dent->d_name,"cdev",4))
                return 0;
        strtoul(dent->d_name+4,&end, 10);
        if (*end)
                return 0;
        return 1;
}

static inline char *getdev(char *path, char *ent) {
        char *a, *b, *ret;
        
        asprintf(&a,"%s/%s",path,ent);
        b = canonicalize_file_name(a);
        ret = strdup(basename(b));
        free(b);
        free(a);
        return ret;
}

char *read_subchannels(char *path) {
	char *tmp, *ret;
	int n, x;
	struct dirent **cdevs;
		
	if ((n = scandir(path, &cdevs, is_cdev, alphasort)) <= 0)
		return NULL;
		
	ret = getdev(path,cdevs[0]->d_name);
	for (x = 1 ; x < n ; x++ ) { 
		if (asprintf(&tmp, "%s,%s", ret, getdev(path, cdevs[x]->d_name)) == -1) 
			return NULL;
		free(ret);
		ret = tmp;
	}
	return ret;
}

#endif

int isCfg(const struct dirent *dent) {
	int len = strlen(dent->d_name);
	
	if (strncmp(dent->d_name,"ifcfg-",6))
		return 0;
	if (strstr(dent->d_name,"rpmnew") ||
	    strstr(dent->d_name,"rpmsave") ||
	    strstr(dent->d_name,"rpmorig"))
		return 0;
	if (dent->d_name[len-1] == '~')
		return 0;
	if (!strncmp(dent->d_name+len-4,".bak",4))
		return 0;
	return 1;
}

static inline char *dequote(char *start, char *end) {
	if (end==NULL) {
		end=start;
		while(*end) end++;
	}
	if (end > start) end--;
	if ((*start == '\'' || *start == '\"') && ( *start == *end ) ) {
		*end='\0';
		if (start<end) start++;
	}
	return start;
}

struct netdev *get_configs() {
	int ncfgs = 0;
	struct netdev *ret, *tmpdev;
	struct dirent **cfgs;
	int x;
	
	ret = NULL;
	
	if ((ncfgs = scandir("/etc/sysconfig/network-scripts",&cfgs,
			     isCfg, alphasort)) == -1) {
		return ret;
	}
	for (x = 0; x < ncfgs; x++ ) {
		char *path;
		char *devname, *hwaddr;
		gchar *contents, **lines;
		int i;
		
		devname = hwaddr = contents = NULL;
		lines = NULL;
		if (asprintf(&path,"/etc/sysconfig/network-scripts/%s",
			     cfgs[x]->d_name) == -1)
			continue;
		g_file_get_contents(path, &contents, NULL, NULL);
		if (!contents)
			continue;
		lines = g_strsplit(contents,"\n", 0);
		for (i = 0; lines[i]; i++) {
			if (g_str_has_prefix(lines[i],"DEVICE=")) {
				devname = dequote(lines[i] + 7, NULL);
				/* ignore alias devices */
				if (strchr(devname,':'))
					devname = NULL;
			}
#if defined(__s390__) || defined(__s390x__)
			if (g_str_has_prefix(lines[i],"SUBCHANNELS=")) {
				hwaddr = dequote(lines[i] + 12, NULL);
			}
#else
			if (g_str_has_prefix(lines[i],"HWADDR=")) {
				hwaddr = dequote(lines[i] + 7, NULL);
			}
#endif
		}
		if (!devname || !hwaddr) {
			g_free(contents);
			g_strfreev(lines);
			continue;
		}
		tmpdev = calloc(1, sizeof(struct netdev));
		tmpdev->dev = g_strstrip(g_strdup(devname));
		tmpdev->hwaddr = g_strstrip(g_strdup(hwaddr));
		if (ret)
			tmpdev->next = ret;
		ret = tmpdev;
		g_free(contents);
		g_strfreev(lines);
	}
	free(cfgs);
	return ret;
}
		
char *get_hwaddr(char *device) {
	char *path = NULL;
	char *contents = NULL;

#if defined(__s390__) || defined(__s390x__)
	if (asprintf(&path, "/sys/class/net/%s/device/.", device) == -1)
		return NULL;
	contents = read_subchannels(path);
#else
	if (asprintf(&path, "/sys/class/net/%s/address", device) == -1)
		return NULL;
	g_file_get_contents(path, &contents, NULL, NULL);
#endif
	free(path);

	return g_strstrip(contents);
}
		
char *get_config_by_hwaddr(char *hwaddr, char *current) {
	struct netdev *config;
	char *first = NULL;
	
	if (!hwaddr) return NULL;
	
	for (config = configs; config; config = config->next) {
		if (strcasecmp(config->hwaddr, hwaddr) != 0)
			continue;
		if (!current || !strcasecmp(config->dev, current))
			return config->dev;
		if (!first)
			first = config->dev;
	}
	return first;
}

struct netdev *get_config_by_name(char *name) {
	struct netdev *config;
	
	if (!name) return NULL;
	
	for (config = configs; config; config = config->next) {
		if (strcasecmp(config->dev, name) == 0)
			return config;
	}
	return NULL;
}

int check_persistent_by_name(char *name) {
	FILE *fptr;
	int ret = 1;
	char *scanstr=NULL;

	asprintf(&scanstr, "NAME=\"%s\"", name);
	if (scanstr == NULL)
		return 1;

	fptr = fopen("/etc/udev/rules.d/70-persistent-net.rules", "r");
	if (fptr == NULL)
		return 1;
	
	while(!feof(fptr)) {
		char line[4096];
		line[0]=0;
		if (fgets(line, sizeof(line), fptr) == NULL)
			break;

		/* ignore lines hinted from us */
		if (strstr(line, "INTERFACE_NAME"))
			continue;

		if (strstr(line, scanstr)) {
			ret = 0;
			break;
		}
	}

	fclose(fptr);
	return ret;
}

char *get_persistent_by_hwaddr(char *hw) {
	FILE *fptr;
	char *scanstr=NULL;
	char target[256];

	target[0]=0;

	asprintf(&scanstr, "ATTR{address}==\"%s\"", hw);
	if (scanstr == NULL)
		return NULL;

	fptr = fopen("/etc/udev/rules.d/70-persistent-net.rules", "r");
	if (fptr == NULL)
		return NULL;
	
	while(!feof(fptr)) {
		char line[4096];
		line[0]=0;
		if (fgets(line, sizeof(line), fptr) == NULL)
			break;

		if (strstr(line, scanstr)) {
			char *cptr;
#define NAME_STR "NAME=\""
			cptr = strstr(line, NAME_STR);
			if (cptr == NULL)
				continue;
			if(sscanf(cptr+sizeof(NAME_STR)-1, "%255[^\"]\"", target) == 1) {
				fclose(fptr);
				return strdup(target);			
			}	
		}
	}

	fclose(fptr);
	return NULL;
}

void take_lock() {
	int count = 0;
	int lockfd;
	
	while (1) {
		lockfd = open(LOCKFILE, O_RDWR|O_CREAT|O_EXCL, 0644);
		if (lockfd != -1) {
			write(lockfd,"%d\n",getpid());
			close(lockfd);
			break;
		}
		count++;
		/* If we've slept for 20 seconds, break the lock. */
		if (count >= 200) {
			int fd;
			char buf[32];
			int pid;
				
			fd = open(LOCKFILE, O_RDONLY);
			if (fd == -1)
				break;
			read(fd,buf,32);
			close(fd);
			pid = atoi(buf);
			if (pid && pid != 1) {
				kill(pid,SIGKILL);
			}
		}
		usleep(100000);
		continue;

	}
	return;
}

int check_and_set_claimed(char *src, char *name)
{
	int fd;
	char *fname;
	if (src == NULL || name == NULL)
		return 1;

	asprintf(&fname, "/dev/.udev/.%s.claimed", name);
	fd = open(fname, O_CREAT|O_EXCL|O_WRONLY, 0600);
	if (fd < 0 )
		return 1;

	write(fd, src, strlen(src));
	close(fd);
	return 0;
}

char *get_next_free(char *name)
{
	char *target=NULL;
  	int num;
	char inamebase[256];
	char inamesuffix[256];
	struct netdev *config;

	inamebase[0]=0;
	inamesuffix[0]=0;
	if (sscanf(name, "%[^0-9]%d%s", inamebase, &num, inamesuffix) < 2)
		return NULL;

	num = -1;

	/* we need to stop at some point */
	while(num < 32000) {
		num++;

		if (target)
			free(target);

		asprintf(&target, "%s%d%s", inamebase, num, inamesuffix);

		config = get_config_by_name(target);
		if (config && config->hwaddr)
			continue;

		/* Parse /etc/udev/rules.d/70-persistent-net.rules. */
		if (check_persistent_by_name(target) == 0)
			continue;

		if (check_and_set_claimed(name, target) != 0)
			continue;	       

		return target;
	}	
	return name;
}

int main(int argc, char **argv) {
	char *src, *target, *hw;
	struct timeval tv;
	struct netdev *config;

	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);	
	take_lock();
	
	signal(SIGSEGV,sighandler);
	signal(SIGKILL,sighandler);
	signal(SIGTERM,sighandler);
	signal(SIGSEGV,sighandler);
	signal(SIGALRM,sighandler);
	alarm(10);
	
	src = getenv("INTERFACE");
	if (!src)
		goto out_unlock;

	configs = get_configs();

	hw = get_hwaddr(src);
	if (!hw)
		goto out_unlock;
	target = get_config_by_hwaddr(hw, src);

	if (target) {
		printf("INTERFACE_NAME=%s", target);
		goto out_unlock;
	}

#if !defined(__s390__) &&  !defined(__s390x__)

	/* There is a hw, but no corresponding interface.
	   Now, check if there is a ifcfg file, which claims
	   the interface name
	*/
	config = get_config_by_name(src);
	if ((config == NULL) || (config->hwaddr == NULL)) {
		/* found no config or the config has no HWADDR */
		if (check_and_set_claimed(src, src) == 0)
			goto out_unlock;
	}

	/* Interface name is taken already, now try to find
	   a free slot.
	*/
	/* Parse /etc/udev/rules.d/70-persistent-net.rules. */
	target = get_persistent_by_hwaddr(hw);

	if (target) {
		/* Check if persistent-net.rules interface is taken in ifcfg. */
		config = get_config_by_name(target);
		if ((config == NULL) || (config->hwaddr == NULL)) {
			if (check_and_set_claimed(src, target) == 0)
				goto out_unlock;
		}
	}

	/* Find a free name slot */
	target = get_next_free(src);	
	if (target)
		printf("INTERFACE_NAME=%s", target);
#endif
	
out_unlock:
	unlink(LOCKFILE);
	exit(0);
}
