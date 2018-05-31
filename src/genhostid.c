/* Copyright (C) 2003-2007 Red Hat, Inc.
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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

int main (void)
{
  struct stat st;
  long int n;

  fprintf(stderr, "warning: genhostid is now deprecated, and will be removed in the near future!\n"
                  "warning: use systemd-machine-id-setup (1) instead! More info: 'man 5 machine-id'\n");

  /*
   * NOTE: gethostid() always returns 32-bit identifier, and st_size field
   *       of stat structure represents total size of file, in bytes.
   */

  if (stat ("/etc/hostid", &st) == 0 &&
      S_ISREG (st.st_mode) && st.st_size == 4) {
    return 0;
  }

  int fd = open ("/dev/random", O_RDONLY);

  if (fd == -1 || read (fd, &n, sizeof (n)) != sizeof (n)) {
    srand48 ((long int) time (NULL) ^ (long int) getpid ());
    n = lrand48 ();
  }

  return sethostid ((int32_t) n);
}
