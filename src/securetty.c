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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

void alarm_handler(int num) {
        return;
}

int open_and_lock_securetty() {
        int fd;
        struct flock lock;
        struct sigaction act, oldact;
        
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
      
        fd = open("/etc/securetty", O_RDWR);
        if (fd == -1) {
                syslog(LOG_ERR, "Couldn't open /etc/securetty: %s",strerror(errno));
                return -1;
        }
        act.sa_handler = alarm_handler;
        act.sa_flags = 0;
        sigaction(SIGALRM, &act, &oldact);
        alarm(2);
        while (fcntl(fd, F_SETLKW, &lock) == -1) {
                if (errno == EINTR) {
                        syslog(LOG_ERR, "Couldn't lock /etc/securetty: Timeout exceeded");
                } else {
                        syslog(LOG_ERR, "Couldn't lock /etc/securetty: %s",strerror(errno));
                }
                return -1;
        }
        alarm(0);
        sigaction(SIGALRM, &oldact, NULL);
        return fd;
}

int rewrite_securetty(char *terminal) {
        int fd;
        char *buf, *pos;
        struct stat sbuf;
        
        fd = open_and_lock_securetty();
        if (fd == -1)
                return 1;
        if (fstat(fd, &sbuf) == -1) {
                close(fd);
                syslog(LOG_ERR, "Couldn't stat /etc/securetty: %s",strerror(errno));
                return 1;
        }
        buf = malloc(sbuf.st_size + 1);
        if (read(fd, buf, sbuf.st_size) != sbuf.st_size) {
                close(fd);
                syslog(LOG_ERR, "Couldn't read /etc/securetty: %s",strerror(errno));
                return 1;
        }
        if (!strncmp(buf,terminal,strlen(terminal)) && buf[strlen(terminal)] == '\n')
                goto out_ok;
        if ((pos = strstr(buf, terminal))) {
                if (pos[strlen(terminal)] == '\n' && *(pos-1) == '\n')
                        goto out_ok;
        }
        if (lseek(fd, 0, SEEK_END) == -1) {
                close(fd);
                syslog(LOG_ERR, "Couldn't seek to end of /etc/securetty: %s",strerror(errno));
                return 1;
        }
        write(fd, terminal, strlen(terminal));
        write(fd, "\n", 1);
out_ok:
        close(fd);
        return 0;
}

int main(int argc, char **argv) {
        if (argc < 2 ) {
                fprintf(stderr, "Usage: securetty <device>\n");
                exit(1);
        }
        openlog("securetty", LOG_CONS, LOG_DAEMON);
        return rewrite_securetty(argv[1]);
}
