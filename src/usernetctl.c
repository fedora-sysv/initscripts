#include <alloca.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* this will be running setuid root, so be careful! */

void usage(void) {
    fprintf(stderr, "usage: usernetctl <interface-config> <up|down>\n");
    exit(1);
}

static char * safeEnviron[] = {
	"PATH=/bin:/sbin:/usr/bin:/usr/sbin",
	"HOME=/root",
	NULL
};

int userCtl(char * file, int size) {
    char * contents;
    char * chptr;
    char * end;
    int fd;

    contents = alloca(size + 2);

    if ((fd = open(file, O_RDONLY)) < 0) {
	fprintf(stderr, "failed to open %s: %s\n", file, strerror(errno));
	exit(1);
    }

    if (read(fd, contents, size) != size) {
	perror("error reading device configuration");
	exit(1);
    }
    close(fd);

    contents[size] = '\n';
    contents[size + 1] = '\0';

    /* each pass parses a single line (until an answer is found), contents
       itself points to the beginning of the current line */
    while (*contents) {
	chptr = contents;
	while (*chptr != '\n') chptr++;
	end = chptr + 1;
	while (chptr > contents && isspace(*chptr)) chptr--;
	*(++chptr) = '\0';

	if (!strcmp(contents, "USERCTL=yes")) return 1;

	contents = end;
    }

    return 0;
}

int main(int argc, char ** argv) {
    char * ifaceConfig;
    char * chptr;
    struct stat sb;
    char * cmd;

    if (argc != 3) usage();

    if (!strcmp(argv[2], "up")) {
	cmd = "./ifup";
    } else if (!strcmp(argv[2], "down")) {
	cmd = "./ifdown";
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
    if (!strncmp(ifaceConfig, "ifcfg-", 6)) {
	char *temp;
	temp = (char *) malloc(strlen(ifaceConfig) + 6);
	strcpy(temp, "ifcfg-");
	/* strcat is safe because we got the length from strlen */
	strcat(temp, ifaceConfig);
	ifaceConfig = temp;
    }
    
    /* these shouldn't be symbolic links -- anal, but that's fine w/ me */
    if (lstat(ifaceConfig, &sb)) {
	fprintf(stderr, "failed to stat %s: %s\n", ifaceConfig, 
		strerror(errno));
	exit(1);
    }

    /* safety checks */
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

    if (!userCtl(ifaceConfig, sb.st_size)) {
	fprintf(stderr, "Users are not allowed to control this interface.\n");
	exit(1);
    }

    /* looks good to me -- let's go for it */

    /* pppd wants the real uid to be the same as the effective (god only
       knows why when it works fine setuid out of the box) */
    setuid(geteuid());

    execle(cmd, cmd, ifaceConfig, NULL, safeEnviron);
    fprintf(stderr, "exec of %s failed: %s\n", cmd, strerror(errno));
    
    exit(1);
}
