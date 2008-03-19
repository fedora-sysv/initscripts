
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/serial.h>

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
	    a->c_ispeed != b->c_ispeed || a->c_ospeed != b->c_ospeed)
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

char *check_serial_console(int *speed) {
	int fd;
	char *ret = NULL, *device;
	char twelve = 12;
	struct serial_struct si, si2;

	memset(&si, 0, sizeof(si));
	memset(&si2, 0, sizeof(si));

	fd = open("/dev/console", O_RDWR);
	if (ioctl (fd, TIOCLINUX, &twelve) >= 0)
		goto out;

	if (ioctl(fd, TIOCGSERIAL, &si) < 0)
		goto out;
	close(fd);

	asprintf(&device, "ttyS%d", si.line);
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
	
	args[4] = dev;
	if (speed)
		asprintf(&args[5],"%d",speed);
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
