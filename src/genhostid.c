/* Copyright (C) 2003 Red Hat, Inc.
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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
int
main (void)
{
  struct stat st;
  long int n;
  if (stat ("/etc/hostid", &st) == 0 && S_ISREG (st.st_mode)
      && st.st_size >= sizeof (n))
    return 0;
  int fd = open ("/dev/random", O_RDONLY);
  if (fd == -1 || read (fd, &n, sizeof (n)) != sizeof (n))
    {
      srand48 ((long int) time (NULL) ^ (long int) getpid ());
      n = lrand48 ();
    }
  return sethostid ((int32_t)n);
}
