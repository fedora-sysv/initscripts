/*
 * Copyright (c) 1999-2003 Red Hat, Inc. All rights reserved.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	    return ret;
    }
} 
