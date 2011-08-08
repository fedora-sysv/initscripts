/*
 * Copyright (c) 2008-2009 Red Hat, Inc. All rights reserved.
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
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/serial.h>
#include <linux/serial_core.h>

#ifndef PORT_OMAP
/* from linux-2.6/include/linux/serial_core.h commit b612633b */
#define PORT_OMAP 96
#endif

struct speeds
{
	speed_t speed;
	unsigned long value;
};

struct speeds speed_map[] =
{
	{B50, 50},
	{B75, 75},
	{B110, 110},
        {B134, 134},
        {B150, 150},
        {B200, 200},
        {B300, 300},
        {B600, 600},
        {B1200, 1200},
        {B1800, 1800},
        {B2400, 2400},
        {B4800, 4800},
        {B9600, 9600},
        {B19200, 19200},
        {B38400, 38400},
#ifdef B57600
	{B57600, 57600},
#endif
#ifdef B115200
	{B115200, 115200},
#endif
#ifdef B230400
  	{B230400, 230400},
#endif
#ifdef B460800
	{B460800, 460800},
#endif
	{0, 0}
};

int termcmp(struct termios *a, struct termios *b) {
	if (a->c_iflag != b->c_iflag || a->c_oflag != b->c_oflag ||
	    a->c_cflag != b->c_cflag || a->c_lflag != b->c_lflag ||
	    cfgetispeed(a) != cfgetispeed(b) || cfgetospeed(a) != cfgetospeed(b))
		return 1;
	return memcmp(a->c_cc, b->c_cc, sizeof(a->c_cc));
}       

int get_serial_speed(int fd) {
	struct termios mode;
	
	if (!tcgetattr(fd, &mode)) {
		int i;
		speed_t speed;
		
		speed = cfgetospeed(&mode);
		for (i = 0; speed_map[i].value != 0; i++)
			if (speed_map[i].speed == speed)
				return speed_map[i].value;
	}
	return 0;
}

int compare_termios_to_console(char *dev, int *speed) {
	struct termios cmode, mode;
	int fd, cfd;

	cfd = open ("/dev/console", O_RDONLY);
	tcgetattr(cfd, &cmode);
	close(cfd);

	fd = open(dev, O_RDONLY|O_NONBLOCK);
	tcgetattr(fd, &mode);

	if (!termcmp(&cmode, &mode)) {
		*speed = get_serial_speed(fd);
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

char *serial_tty_name(int type) {
	switch (type) {
		case PORT_8250...PORT_MAX_8250:
			return "ttyS";
		case PORT_PMAC_ZILOG:
			return "ttyPZ";
		case PORT_MPSC:
			return "ttyMM";
		case PORT_CPM:
			return "ttyCPM";
		case PORT_MPC52xx:
			return "ttyPSC";
		case PORT_IMX:
			return "ttymxc";
		case PORT_OMAP:
			return "ttyO";
		default:
			return NULL;
	}
}

char *check_serial_console(int *speed) {
	int fd;
	char *ret = NULL, *device;
	char twelve = 12;
	struct serial_struct si, si2;
	char *tty_name;

	memset(&si, 0, sizeof(si));
	memset(&si2, 0, sizeof(si));

	fd = open("/dev/console", O_RDWR);
	if (ioctl (fd, TIOCLINUX, &twelve) >= 0)
		goto out;

	if (ioctl(fd, TIOCGSERIAL, &si) < 0)
		goto out;
	close(fd);

	tty_name = serial_tty_name(si.type);
	if (!tty_name)
		goto out;

	asprintf(&device, "%s%d", tty_name, si.line);
	fd = open(device, O_RDWR|O_NONBLOCK);
	if (fd == -1)
		goto out;

	if (ioctl(fd, TIOCGSERIAL, &si2) < 0)
		goto out;

	if (memcmp(&si,&si2, sizeof(si)))
		goto out;

	*speed = get_serial_speed(fd);
	ret = device;
out:
	close(fd);
	return ret;
}

int emit_console_event(char *dev, int speed) {
	char *args[] = { "initctl", "emit", "--no-wait", "fedora.serial-console-available", NULL, NULL, NULL };
	
	if (dev)
		asprintf(&args[4],"DEV=%s",dev);
	if (speed)
		asprintf(&args[5],"SPEED=%d",speed);
	execv("/sbin/initctl", args);
	return 1;	
}

int main(int argc, char **argv) {
	char *device;
	int speed;
	
	if (argc < 2) {
		printf("usage: console_check <device>\n");
		exit(1);
	}
	chdir("/dev");
	device = argv[1];
	if (!strcmp(device, "console")) {
		device = check_serial_console(&speed);
		if (device)
			return emit_console_event(device, speed);
	} else if (compare_termios_to_console(device, &speed)) {
		return emit_console_event(device, speed);
	}
	return 0;
}
