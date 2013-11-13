#include <ctype.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int get_threshold_from_file(FILE *fp)
{
    static const char key[] = "THRESHOLD=";
    int pos = 0;
    int thres;
    char ch;

start:
    /* State START - at beginning of line, search for beginning of "THRESHOLD="
     * string.
     */
    ch = getc(fp);
    if (ch == EOF) {
        return DEFAULT;
    }
    if (isspace(ch)) {
        goto start;
    }
    if (ch == 'T') {
        pos = 1;
        goto key;
    }
    goto eol;

eol:
    /* State EOL - loop until end of line */
    ch = getc(fp);
    if (ch == EOF) {
        return DEFAULT;
    }
    if (ch == '\n') {
        goto start;
    }
    goto eol;

key:
    /* State KEY - match "THRESHOLD=" string, go to THRESHOLD if found */
    ch = getc(fp);
    if (ch == EOF) {
        return DEFAULT;
    }
    if (ch == key[pos]) {
        pos++;
        if (key[pos] == 0) {
            goto threshold;
        } else {
            goto key;
        }
    }
    goto eol;

threshold:
    /* State THRESHOLD - parse number using fscanf, expect comment or space
     * or EOL.
     */
    ch = getc(fp);
    if (ch == EOF) {
        return DEFAULT;
    }
    if (!isdigit(ch)) {
        goto eol;
    }
    ungetc(ch, fp);
    if (fscanf(fp, "%d", &thres) != 1) {
        return DEFAULT;
    }
    ch = getc(fp);
    if (ch == '#' || ch == EOF || ch == '\n' || isspace(ch)) {
        return thres;
    }
    goto eol;
}

int get_threshold()
{
    FILE *fp = fopen(SYSCONFIG_KVM, "r");
    int val = get_threshold_from_file(fp);

    fclose (fp);
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
