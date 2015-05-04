
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
#include <ctype.h>

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
	char *c;
	//remove comments and trailing whitespace
	for (c = start; c && *c; c++) {
		c = strchr(c, '#');
		if (!c)
			break;
		if (c > start && isblank(*(c-1))) {
			*c = '\0';
			break;
		}
	}

	g_strchomp(start);

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
		int vlan;
		gchar *contents, **lines;
		int i;
		
		devname = hwaddr = contents = NULL;
		lines = NULL;
		vlan = 0;
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
			} else if (g_str_has_prefix(lines[i],"HWADDR=")) {
				hwaddr = dequote(lines[i] + 7, NULL);
			}
#else
			if (g_str_has_prefix(lines[i],"HWADDR=")) {
				hwaddr = dequote(lines[i] + 7, NULL);
			}
#endif
			if (g_str_has_prefix(lines[i],"VLAN=yes")) {
				vlan=1;
			}
		}
		if (!devname || !hwaddr || vlan) {
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

	if (contents == NULL) {
		if (asprintf(&path, "/sys/class/net/%s/address", device) == -1)
			return NULL;
		g_file_get_contents(path, &contents, NULL, NULL);
	}

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

void take_lock() {
	int count = 0;
	int lockfd;
	
	while (1) {
		lockfd = open(LOCKFILE, O_RDWR|O_CREAT|O_EXCL, 0644);
		if (lockfd != -1) {
			char buf[32];

			snprintf(buf,32,"%d\n",getpid());
			write(lockfd,buf,strlen(buf));
			close(lockfd);
			break;
		} else if (errno == EACCES)
                        break;

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

int main(int argc, char **argv) {
	char *src, *target, *hw;
	struct timeval tv;

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
	if (!target)
		goto out_unlock;

	printf("%s", target);
	
out_unlock:
	unlink(LOCKFILE);
	exit(0);
}
