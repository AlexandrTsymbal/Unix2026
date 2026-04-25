#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define LOCK_SUFFIX ".lck"

static volatile int running = 1;
static int success_count = 0;
static char stat_file[] = "stat.txt";

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

void write_stat() {
    FILE *f = fopen(stat_file, "a");
    if (!f) {
        perror("fopen stat");
        return;
    }
    fprintf(f, "PID %d: %d locks\n", getpid(), success_count);
    fclose(f);
}

int main(int argc, char *argv[]) {
    int opt;
    char *filename = NULL;

    signal(SIGINT, handle_sigint);

    while ((opt = getopt(argc, argv, "f:")) != -1) {
        if (opt == 'f') {
            filename = optarg;
        } else {
            fprintf(stderr, "Usage: %s -f filename\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!filename) {
        fprintf(stderr, "Filename required\n");
        exit(EXIT_FAILURE);
    }

    srand(getpid());
    
    char lockfile[256];
    snprintf(lockfile, sizeof(lockfile), "%s%s", filename, LOCK_SUFFIX);

    while (running) {

        usleep(100000 + rand() % 200000);

        int fd;

        while ((fd = open(lockfile, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
            if (errno == EEXIST) {
                usleep(100000);
            } else {
                perror("open lockfile");
                exit(EXIT_FAILURE);
            }
        }

        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d\n", getpid());

        ssize_t w = write(fd, buf, len);
        if (w == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }

        close(fd);

        sleep(1);

        FILE *f = fopen(lockfile, "r");
        if (!f) {
            perror("fopen lockfile");
            exit(EXIT_FAILURE);
        }

        int pid_in_file;
        if (fscanf(f, "%d", &pid_in_file) != 1) {
            fprintf(stderr, "Failed to read PID\n");
            fclose(f);
            exit(EXIT_FAILURE);
        }

        fclose(f);

        if (pid_in_file != getpid()) {
            fprintf(stderr, "Lock stolen!\n");
            exit(EXIT_FAILURE);
        }

        if (unlink(lockfile) == -1) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

        success_count++;

        usleep(100000 + rand() % 200000);
    }

    write_stat();
    return 0;
}