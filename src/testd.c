#include <sys/signal.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    signal(SIGTERM, SIG_IGN);
    while (1) sleep(20);
    exit(0);
}
