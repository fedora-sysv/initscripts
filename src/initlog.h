
#define _GNU_SOURCE	1

#ifndef INITLOG_H
#define INITLOG_H

struct logInfo {
    char *cmd;
    char *line;
    int fac;
    int pri;
};

char *getLine(char **data);
int logString(char *cmd, char *string);
void processArgs(int argc, char **argv);

#endif
