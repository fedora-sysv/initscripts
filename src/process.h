#ifndef PROCESS_H
#define PROCESS_H


#define CMD_FD	21

int runCommand(char *cmd, int reexec, int quiet, int debug);

#endif
