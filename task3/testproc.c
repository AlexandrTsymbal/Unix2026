#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    while (1) {
        printf("%s alive\n", argv[1]);
        fflush(stdout);
        sleep(5);
    }

    return 0;
}