/*
 * Copyright (c) 1997-2007 Red Hat, Inc. All rights reserved.
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

#include <alloca.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/* This will be running setuid root, so be careful! */
static const char * safeEnviron[] = {
	"PATH=/bin:/sbin:/usr/bin:/usr/sbin",
	"HOME=/root",
	NULL
};

#define FOUND_FALSE -1
#define NOT_FOUND 0
#define FOUND_TRUE 1

static void
usage(void) {
    fprintf(stderr, "usage: usernetctl <interface-config> <up|down|report>\n");
    exit(1);
}

static size_t
testSafe(char *ifaceConfig, int fd) {
    struct stat sb;

    /* These shouldn't be symbolic links -- anal, but that's fine w/ mkj. */
    if (fstat(fd, &sb)) {
	fprintf(stderr, "failed to stat %s: %s\n", ifaceConfig, 
		strerror(errno));
	exit(1);
    }

    /* Safety/sanity checks. */
    if (!S_ISREG(sb.st_mode)) {
	fprintf(stderr, "%s is not a normal file\n", ifaceConfig);
	exit(1);
    }

    if (sb.st_uid) {
	fprintf(stderr, "%s should be owned by root\n", ifaceConfig);
	exit(1);
    }
    
    if (sb.st_mode & S_IWOTH) {
	fprintf(stderr, "%s should not be world writeable\n", ifaceConfig);
	exit(1);
    }

    return sb.st_size;
}


static int
userCtl(char *file) {
    char *buf;
    char *contents = NULL;
    char *chptr = NULL;
    char *next = NULL;
    int fd = -1, retval = NOT_FOUND;
    size_t size = 0;

    /* Open the file and then test it to see if we like it. This way
       we avoid switcheroo attacks. */
    if ((fd = open(file, O_RDONLY)) == -1) {
	fprintf(stderr, "failed to open %s: %s\n", file, strerror(errno));
	exit(1);
    }

    size = testSafe(file, fd);
    if (size > INT_MAX) {
        fprintf(stderr, "file %s is too big\n", file);
        exit(1);
    }

    buf = contents = malloc(size + 2);
    if (contents == NULL) {
        fprintf(stderr, "failed to allocate memory\n");
        exit(1);
    }

    if (read(fd, contents, size) != size) {
	perror("error reading device configuration");
	exit(1);
    }
    close(fd);

    contents[size] = '\n';
    contents[size + 1] = '\0';

    /* Each pass parses a single line (until an answer is found),  The contents
       pointer itself points to the beginning of the current line. */
    while (*contents) {
	chptr = contents;
	while (*chptr != '\n') chptr++;
	next = chptr + 1;
	while (chptr >= contents && isspace(*chptr)) chptr--;
	*(++chptr) = '\0';

	if (!strncasecmp(contents, "USERCTL=", 8)) {
	    contents += 8;
	    if ((contents[0] == '"' &&
		 contents[strlen(contents) - 1] == '"') ||
		(contents[0] == '\'' &&
		 contents[strlen(contents) - 1] == '\''))
		{
		contents++;
		contents[strlen(contents) - 1] = '\0';
	    }

	    if (!strcasecmp(contents, "yes") || !strcasecmp(contents, "true")) 
		retval = FOUND_TRUE;
	    else 
		retval = FOUND_FALSE;

	    break;
	}

	contents = next;
    }

    free(buf);

    return retval;
}

int
main(int argc, char ** argv) {
    char * ifaceConfig;
    char * chptr;
    char * cmd = NULL;
    int report = 0;
    char tmp;

    if (argc != 3) usage();

    if (!strcmp(argv[2], "up")) {
	cmd = "./ifup";
    } else if (!strcmp(argv[2], "down")) {
	cmd = "./ifdown";
    } else if (!strcmp(argv[2], "report")) {
	report = 1;
    } else {
	usage();
    }

    if (chdir("/etc/sysconfig/network-scripts")) {
	fprintf(stderr, "error switching to /etc/sysconfig/network-scripts: "
		"%s\n", strerror(errno));
	exit(1);
    }

    /* force the interface configuration to be in the current directory */
    chptr = ifaceConfig = argv[1];
    while (*chptr) {
	if (*chptr == '/')
	    ifaceConfig = chptr + 1;
	chptr++;
    }

    /* automatically prepend "ifcfg-" if it is not specified */
    if (strncmp(ifaceConfig, "ifcfg-", 6)) {
	char *temp;
        size_t len = strlen(ifaceConfig);

	/* Make sure a wise guys hasn't tried an integer wrap-around or
	   stack overflow attack. There's no way it could refer to anything 
	   bigger than the largest filename, so cut 'em off there. */
        if (len > PATH_MAX)
		exit(1);

	temp = (char *) alloca(len + 7);
	strcpy(temp, "ifcfg-");
	/* strcat is safe because we got the length from strlen */
	strcat(temp, ifaceConfig);
	ifaceConfig = temp;
    }
    
    if(getuid() != 0)
    switch (userCtl(ifaceConfig)) {
	char *dash;

	case NOT_FOUND:
	    /* a `-' will be found at least in "ifcfg-" */
	    dash = strrchr(ifaceConfig, '-');
	    if (*(dash-1) != 'g') {
		/* This was a clone configuration; ask the parent config */
		tmp = *dash;
		*dash = '\0';
		if (userCtl(ifaceConfig) == FOUND_TRUE) {
		    /* exit the switch; users are allowed to control */
		    *dash = tmp;
		    break;
		}
		*dash = tmp;
	    }
	    /* else fall through */
	case FOUND_FALSE:
	    if (! report)
	        fprintf(stderr,
			"Users are not allowed to control this interface.\n");
	    exit(1);
	    break;
    }

    /* looks good to me -- let's go for it if we are changing the interface,
     * report good status to the user otherwise */

    if (report)
	exit(0);

    /* pppd wants the real uid to be the same as the effective (god only
       knows why when it works fine setuid out of the box) */
    setuid(geteuid());
    /* Drop user gid (for temp files, SELinux) */
    setgid(0);

    execle(cmd, cmd, ifaceConfig, NULL, safeEnviron);
    fprintf(stderr, "exec of %s failed: %s\n", cmd, strerror(errno));
    
    exit(1);
}
