/*
 * Copyright (c) 1999-2009 Red Hat, Inc. All rights reserved.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

int main(int argc, char **argv)
{
    unsigned char twelve = 12;
    char *type;
    int maj, min, ret = 0, fg = -1;
    struct stat sb;
	
    fstat(0, &sb);
    maj = major(sb.st_rdev);
    min = minor(sb.st_rdev);
    if (maj != 3 && (maj < 136 || maj > 143)) {
	if ((fg = ioctl (0, TIOCLINUX, &twelve)) < 0) {
	    type = "serial";
	    ret = 1;
	} else {
#ifdef __powerpc__
	    int fd;
	    char buf[65536];
	    
	    fd = open("/proc/tty/drivers",O_RDONLY);
	    read(fd, buf, 65535);
	    if (strstr(buf,"vioconsole           /dev/tty")) {
		    type = "vio";
		    ret = 3;
	    } else {
		    type = "vt";
		    ret = 0;
	    }
#else
	    type = "vt";
	    ret = 0;
#endif
	}
    } else {
	type = "pty";
	ret = 2;
    }
    if (argc > 1 && !strcmp(argv[1],"fg")) {
	    if (fg < 0 || fg != (min-1))
		    return 1;
	    return 0;
    } else {
	    printf("%s\n",type);
	    return (argc > 1 && !strcmp(argv[1],"stdout")) ? 0 : ret;
    }
} 
