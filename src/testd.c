#include <sys/signal.h>
#include <unistd.h>

void main() {
    signal(SIGTERM, SIG_IGN);
    while (1) sleep(20);
}
