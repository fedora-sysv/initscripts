/*
 * Copyright (c) 1997-1999 Red Hat, Inc. All rights reserved.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <unistd.h>

int main(int argc, char ** argv) {
    if (argc<2) return 1;
    execvp(argv[1], argv + 2);
    return 1;
}
