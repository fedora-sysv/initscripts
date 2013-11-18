#include <ctype.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shvar.h"

#define DEFAULT 0
#define FACILITY "kvm"
#define SYSCONFIG_KVM "/etc/sysconfig/kvm"

#define COUNT_MSG \
    "%d %s now active"

#define SUBSCRIPTION_MSG \
    "%d %s now active; your Red Hat Enterprise Linux subscription" \
    " limit is %d guests. Please review your Red Hat Enterprise Linux" \
    " subscription agreement or contact your Red Hat" \
    " support representative for more information. You" \
    " may review the Red Hat Enterprise subscription" \
    " limits at http://www.redhat.com/rhel-virt-limits"


int get_threshold()
{
    int val = 0;
    char *cval = NULL;

    shvarFile *file = svNewFile(SYSCONFIG_KVM);
    if (!file)
        return 0;

    cval = svGetValue(file, "THRESHOLD");

    svCloseFile(file);

    if (cval == NULL)
        return 0;

    val = atoi(cval);

    free(cval);
    
    return val;
}

const char *guest(int count)
{
    return (count == 1 ? "guest" : "guests");
}

void emit_count_message(int count)
{
    openlog(FACILITY, LOG_CONS, LOG_USER);
    syslog(LOG_INFO, COUNT_MSG, count, guest(count));
    closelog();
}

void emit_subscription_message(int count, int threshold)
{
    openlog(FACILITY, LOG_CONS, LOG_USER);
    syslog(LOG_WARNING, SUBSCRIPTION_MSG, count, guest(count), threshold);
    closelog();
}

int main(int argc, char **argv)
{
    int count, threshold;

    if (argc < 3)
        exit(1);

    count = atoi(argv[1]);
    threshold = get_threshold();

    if (!strcmp(argv[2], "create")) {
        if (threshold == 0) {
            emit_count_message(count);
        } else if (count > threshold) {
            emit_subscription_message(count, threshold);
        }
    } else {
        if (count >= threshold) {
            emit_count_message(count);
        }
    }

    return 0;
}
