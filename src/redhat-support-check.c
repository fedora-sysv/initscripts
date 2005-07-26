/*
 * Copyright (c) 2003-2004 Red Hat, Inc. All rights reserved.
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <libintl.h>
#include <locale.h>

#include <sys/types.h>

#include <glib.h>
#include <popt.h>

#define _(String) gettext((String))

#define SUPPORTINFO 	"/var/lib/supportinfo"
#define CPUINFO		"/proc/cpuinfo"

static int min_mem = 0;
static int max_mem = 0;
static int max_cpus = 0;
static char *variant = NULL;

static char *release_name = NULL;

static int test = 0;

#if defined(__i386__)
static inline void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
	__asm__("pushl %%ebx; cpuid; movl %%ebx,%1; popl %%ebx"
		: "=a"(*eax), "=r"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "0" (op));
}
#endif

#if defined(__x86_64__)
static inline void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
	__asm__("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1"
		: "=a"(*eax), "=r"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "0" (op));
}
#endif

guint64 get_memory() {
	guint64 npages = sysconf(_SC_PHYS_PAGES);
	guint64 pgsize = sysconf(_SC_PAGESIZE);
	
	return (guint64)(npages * pgsize / 1048576);
}

unsigned int get_num_cpus() {
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	
#if defined(__i386__) || defined(__x86_64__)
	u_int32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
	
	cpuid(0, &eax, &ebx, &ecx, &edx);
	if (ebx == 0x756e6547) { /* Intel */
		cpuid(1, &eax, &ebx, &ecx, &edx);
		if (edx & (1 << 28)) {  /* has HT */
			int nsibs = (ebx & 0xff0000) >> 16;
			return ncpus / nsibs;
		}
	}
	if (ebx==0x68747541 && edx==0x69746e65 && ecx==0x444d4163) {
		cpuid(1, &eax, &ebx, &ecx, &edx);
		if (edx & (1 << 28)) {  /* has HT */
			int nsibs = (ebx & 0xff0000) >> 16;
			if (nsibs == 0) nsibs = 1;
			return ncpus / nsibs;
		}
	}
	if (ebx==0x68747541 && edx==0x69746e65 && ecx==0x444d4163) {
		cpuid(1, &eax, &ebx, &ecx, &edx);
		if (edx & (1 << 28)) {  /* has HT */
			int nsibs = (ebx & 0xff0000) >> 16;
			if (nsibs == 0) nsibs = 1;
			return ncpus / nsibs;
		}
	}
#endif
#if defined(__ia64__)
	gchar *contents, **lines;
	gsize len;
	int x;
	GList *list = NULL;
	
	g_file_get_contents(CPUINFO, &contents, &len, NULL);
	if (!contents)
		return ncpus;
	lines = g_strsplit(contents,"\n", 0);
	
	len = 0;
	for (x = 0; lines[x]; x++) {
		if (g_str_has_prefix(lines[x],"physical id")) {
			if (!g_list_find_custom(list, lines[x],g_ascii_strcasecmp))
				list = g_list_prepend(list, lines[x]);
		}
	}
	len = g_list_length(list);
	if (len) {
		g_list_free(list);
		ncpus = len;
	}
#endif
	return ncpus;
}

int parse_supportinfo() {
	gchar *contents, **lines;
	gsize len;
	int x;
	
	g_file_get_contents(SUPPORTINFO, &contents, &len, NULL);
	if (!contents)
		return -1;
	lines = g_strsplit(contents,"\n", 0);
	for (x = 0; lines[x]; x++) {
		if (!strncmp(lines[x],"Variant:",8))
			variant = strdup(lines[x]+9);
		if (!strncmp(lines[x],"MinRAM:",7))
			min_mem = atoi(lines[x]+8);
		if (!strncmp(lines[x],"MaxRAM:",7))
			max_mem = atoi(lines[x]+8);
		if (!strncmp(lines[x],"MaxCPU:",7))
			max_cpus = atoi(lines[x]+8);
	}
	g_free(contents);
	g_strfreev(lines);
	return 0;
}

void get_release_info() {
	gchar *contents;
	gsize len;
	
	g_file_get_contents("/etc/redhat-release", &contents, &len, NULL);
	if (!contents)
		release_name = "Red Hat Linux release 11.7 (Foo)";
	else
		release_name = g_strstrip(contents);
}
		
int main(int argc, char **argv) {
	const struct poptOption options[] = {
		POPT_AUTOHELP
		{ "test", 't', POPT_ARG_NONE, &test, 0,
		   _("Test mode (assumes 256MB, 1GB, 2 CPUs)"), NULL
		},
		{ 0, 0, 0, 0, 0, 0 }
	};
	poptContext context;
	guint64 memory;
	int cpus;
	int rc = 0;
	
	setlocale(LC_ALL, "");
	bindtextdomain("redhat-support-check", "/usr/share/locale");
	textdomain("redhat-support-check");
	
	context = poptGetContext("redhat-support-check", argc, 
				 (const char **)argv, options, 0);
	while ((rc = poptGetNextOpt (context)) > 0);
	if (rc < -1) {
		fprintf(stderr, "%s: %s\n",
			poptBadOption(context, POPT_BADOPTION_NOALIAS),
			poptStrerror(rc));
		return 1;
	}
	rc = 0;
	if (test) {
		max_mem = 1024;
		min_mem = 256;
		max_cpus = 2;
		variant = "XS";
	} else {
		if (parse_supportinfo()) {
			fprintf(stderr,
			        _("Failed to parse supportinfo file.\n"));
			return 1;
		}
	}
	
	memory = get_memory();
	cpus = get_num_cpus();
	get_release_info();
	
	openlog("redhat-support-check", 0, LOG_USER);
	if (min_mem && memory < (0.9 * min_mem)) {
		printf(_("WARNING: %s requires at least %dMB RAM to run as a supported configuration. (%lluMB detected)\n"),
		       release_name, min_mem, memory);
		syslog(LOG_NOTICE,_("WARNING: %s requires at least %dMB RAM to run as a supported configuration. (%lluMB detected)\n"),
		       release_name, min_mem, memory);
		rc++;
	}
	if (max_mem && memory > max_mem) {
		printf(_("WARNING: %s requires no more than %dMB RAM to run as a supported configuration. (%lluMB detected)\n"),
		       release_name, max_mem, memory);
		syslog(LOG_NOTICE,_("WARNING: %s requires no more than %dMB RAM to run as a supported configuration. (%lluMB detected)\n"),
		       release_name, max_mem, memory);
		rc++;
	}
	if (max_cpus && cpus > max_cpus) {
		printf(_("WARNING: %s requires no more than %d CPUs to run as a supported configuration. (%d detected)\n"),
		       release_name, max_cpus, cpus);
		syslog(LOG_NOTICE,_("WARNING: %s requires no more than %d CPUs to run as a supported configuration. (%d detected)\n"),
		       release_name, max_cpus, cpus);
		rc++;
	}
	return rc;
}
