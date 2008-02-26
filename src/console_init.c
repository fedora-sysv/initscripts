/*
 * Copyright (c) 2008 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/kd.h>

#include "shvar.h"

static char *lang = NULL;
static char *font = NULL;
static char *acm = NULL;
static char *keymap = NULL;

static int linux_console(int fd) {
	unsigned char twelve = 12;

	if (ioctl(fd, TIOCLINUX, &twelve) >= 0) 
		return 1;
	return 0;
}

static int configured_as_utf8() {
        shvarFile *i18nfile = NULL;

        if ((i18nfile = svNewFile("/etc/sysconfig/i18n")) == NULL)
                return 1; /* assume UTF-8 */

        lang = svGetValue(i18nfile, "LANG");
        font = svGetValue(i18nfile, "SYSFONT");
        acm = svGetValue(i18nfile, "SYSFONTACM");
        svCloseFile(i18nfile);
        if (!lang)
                return 1;
        if (g_str_has_suffix(lang,".utf8") || g_str_has_suffix(lang,".UTF-8"))
                return 1;
	return 0;
}

static int read_keymap() {
	shvarFile *keyboard = NULL;
	char *tmp;
	struct stat sb;
	
	if (!stat("/etc/sysconfig/console/default.kmap",&sb)) {
		keymap = "/etc/sysconfig/console/default.kmap";
		return 0;
	}

	if ((keyboard = svNewFile("/etc/sysconfig/keyboard")) == NULL)
		return 0;

	tmp = svGetValue(keyboard, "KEYMAP");
	if (tmp)
		keymap = tmp;
	tmp = svGetValue(keyboard, "KEYTABLE");
	if (tmp) {
		if (keymap) free(keymap);
		asprintf(&keymap, "%s.map", tmp);
	}
	return 0;
}

static void set_font(int fd) {
	int pid, status;
	
	if ( (pid = fork()) == 0) {
		char *args[] = { "setfont", "latarcyrheb-sun16", NULL, NULL, NULL };

		if (font)
			args[1] = font;
		if (acm) {
			args[2] = "-u";
			args[3] = acm;
		}
		execv("/bin/setfont", args);
		exit(1);
	}
	waitpid(pid, &status, 0);
}

static void set_keyboard(int fd, int utf8) {
	if (ioctl(fd, KDSKBMODE, utf8 ? K_UNICODE : K_XLATE))
		perror("could not set keyboard mode");
}

static void set_terminal(int fd, int utf8) {
	if (utf8)
		write(fd, "\033%G", 3);
	else
		write(fd, "\033%@", 3);
}

static void set_keymap(int fd, int utf8) {
	int pid, status;
	
	if ((pid = fork()) == 0) {
		char *args[] = { "loadkeys", NULL, NULL, NULL };
		dup2(fd, 0);
		dup2(fd, 1);
		
		if (utf8) {
			args[1] = "-u";
			args[2] = keymap;
		} else {
			args[1] = keymap;
		}
		execv("/bin/loadkeys", args);
		exit(1);
	}
	waitpid(pid, &status, 0);
}

int main(int argc, char **argv) {
	char *device;
	int dev;

	if (argc < 2) {
		printf("usage: console_init <device>\n");
		exit(1);
	}
	chdir("/dev");
	device = argv[1];
	dev = open(device, O_RDWR);
	if (linux_console(dev)) {
		int utf8 = configured_as_utf8();

		set_keyboard(dev, utf8);
		set_terminal(dev, utf8);
		set_font(dev);
		read_keymap();
		if (keymap)
			set_keymap(dev,utf8);
	}
	return 0;
}
