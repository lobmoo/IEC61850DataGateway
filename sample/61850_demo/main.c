#include "common.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if(argc < 2) {
        printf("Usage: %s [svr] [cli]\n", argv[0]);
        return -1;
    }
    if (strcmp(argv[1], "svr") == 0) {
        return service(102);
    } else if (strcmp(argv[1], "cli") == 0) {
        return client("localhost", 102);
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return -1;
    }
    return 0;
}