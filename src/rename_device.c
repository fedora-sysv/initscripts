
/* 
 * rename_device.c: udev helper to rename ethernet devices.
 *
 * Copyright (C) 2006 Red Hat, Inc. All rights reserved.
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

struct netdev *configs = NULL;
struct netdev *devs = NULL;

struct netdev *get_devs() {
	DIR *dir;
	struct dirent *entry;
	struct netdev *ret = NULL, *tmpdev;
	
	dir = opendir("/sys/class/net");
	if (!dir)
		return NULL;
	while ((entry = readdir(dir))) {
		char *path;
		gchar *contents;
		
		contents = NULL;
		
		if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) {
			continue;
		}
		if (asprintf(&path,"/sys/class/net/%s/type",entry->d_name) == -1)
			continue;
		g_file_get_contents(path, &contents, NULL, NULL);
		if (!contents) continue;
		if (atoi(contents) >= 256) {
			g_free(contents);
			continue;
		}
		g_free(contents);
		contents = NULL;
		if (asprintf(&path,"/sys/class/net/%s/address",entry->d_name) == -1)
			continue;
		g_file_get_contents(path, &contents, NULL, NULL);
		if (!contents) continue;
		contents = g_strstrip(contents);
		tmpdev = calloc(1, sizeof(struct netdev));
		tmpdev->dev = g_strstrip(g_strdup(entry->d_name));
		tmpdev->hwaddr = g_strstrip(g_strdup(contents));
		if (ret)
			tmpdev->next = ret;
		ret = tmpdev;
		g_free(contents);
	}
	return ret;
}

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
	if (!strncmp(dent->d_name+len-5,".bak",4))
		return 0;
	return 1;
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
				devname = lines[i] + 7;
				/* ignore alias devices */
				if (strchr(devname,':'))
					devname = NULL;
			}
			if (g_str_has_prefix(lines[i],"HWADDR=")) {
				hwaddr = lines[i] + 7;
			}
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
	struct netdev *dev;
	
	for (dev = devs; dev; dev = dev->next)
		if (!strcmp(dev->dev, device))
			return dev->hwaddr;
	return NULL;
}
		
char *get_config_by_hwaddr(char *hwaddr) {
	struct netdev *config;
	
	if (!hwaddr) return NULL;
	
	for (config = configs; config; config = config->next)
		if (!strcasecmp(config->hwaddr, hwaddr))
			return config->dev;
	return NULL;
}

char *get_device_by_hwaddr(char *hwaddr) {
	struct netdev *dev;
	
	if (!hwaddr) return NULL;
	
	for (dev = devs; dev; dev = dev->next)
		if (!strcasecmp(dev->hwaddr, hwaddr))
			return dev->dev;
	return NULL;
}

int do_rename(char *src, char *target) {
	int sock;
	struct ifreq ifr;
	int ret;
	
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		return 1;
	
	memset(&ifr,'\0', sizeof(struct ifreq));
	g_strlcpy(ifr.ifr_name, src, IFNAMSIZ);
	g_strlcpy(ifr.ifr_newname, target, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIFNAME, &ifr);
	return ret;
}
	

void rename_device(char *src, char *target, struct netdev *current) {
	int rc;
	
	if (!current) {
		current = calloc(1, sizeof(struct netdev));
		current->dev = src;
	}
	
	rc = do_rename(src, target);
	if (rc && errno != ENODEV) {
		char *hw;
		char *nconfig;
		char *curdev;
		char *dev = NULL;
		struct netdev *i, *tmpdev;
		
		hw = get_hwaddr(target);
		if (!hw) {
			devs = get_devs();
			hw = get_hwaddr(target);
			if (!hw)
				return;
		}
		
		nconfig = get_config_by_hwaddr(hw);
		curdev = get_device_by_hwaddr(hw);

		if (nconfig) {
			dev = nconfig;
			for (i = current; i; i = i->next) {
				if (!strcmp(i->dev,dev))
					dev = NULL;
			}
		}
		if (!dev)
			asprintf(&dev,"dev%d",rand());
		if (!dev)
			return;
		tmpdev = calloc(1,sizeof(struct netdev));
		tmpdev->dev = curdev;
		if (current)
			tmpdev->next = current;
		current = tmpdev;
		rename_device(curdev, dev, current);
		do_rename(src,target);
	}
}

void take_lock() {
	int count = 0;
	int lockfd;
	
	while (1) {
		lockfd = open(LOCKFILE, O_RDWR|O_CREAT|O_EXCL);
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

int main(int argc, char **argv) {
	char *src, *target, *hw;
		
	take_lock();
	
	signal(SIGSEGV,sighandler);
	signal(SIGKILL,sighandler);
	signal(SIGTERM,sighandler);
	signal(SIGSEGV,sighandler);
	signal(SIGALRM,sighandler);
	alarm(10);
	
	configs = get_configs();
	devs = get_devs();
	
	src = getenv("INTERFACE");
	if (!src)
		goto out_unlock;
	hw = get_hwaddr(src);
	if (!hw)
		goto out_unlock;
	target = get_config_by_hwaddr(hw);
	if (!target || !strcmp(src,target))
		goto out_unlock;
	
	rename_device(src, target, NULL);
	printf("INTERFACE=%s\n",target);
out_unlock:
	unlink(LOCKFILE);
	exit(0);
}
