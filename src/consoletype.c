#include <stdio.h>
#include <string.h>
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
	    type = "vt";
	    ret = 0;
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
