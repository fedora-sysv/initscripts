
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "shvar.h"

char *read_name() {
    gchar *name = NULL;
    char *c;

    g_file_get_contents("/var/lib/rhsm/branded_name", &name, NULL, NULL);
    if (name) {
        c = strchr(name, '\n');
        if (c)
                *c = '\0';
        name = g_strstrip(name);
    }
    return name;
}

int main(int argc, char **argv) {
    shvarFile *osrelease;
    char *newname, *oldname;
    int rc = 0;

    newname = read_name();
    if (!newname)
        return 0;
    osrelease = svNewFile("/etc/os-release");
    if (!osrelease)
        return 0;
    oldname = svGetValue(osrelease, "PRETTY_NAME");
    if (oldname && !strcmp(oldname, newname))
        return 0;
    svSetValue(osrelease, "PRETTY_NAME", newname);
    rc += svWriteFile(osrelease, 0644);
    svCloseFile(osrelease);
    return rc;
}
