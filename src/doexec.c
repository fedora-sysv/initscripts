#include <unistd.h>

int main(int argc, char ** argv) {
    if (argc<2) return 1;
    execvp(argv[1], argv + 2);
    return 1;
}
