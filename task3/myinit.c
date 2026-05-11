#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define MAX_PROCS 100
#define MAX_ARGS 32
#define MAX_LINE 1024

typedef struct {
    char *exec_path;
    char *arg1;
    char *stdin_file;
    char *stdout_file;
    pid_t pid;
} process_t;

process_t processes[MAX_PROCS];

int process_count = 0;

char config_path[512];

volatile sig_atomic_t hup_flag = 0;

int log_fd = -1;

void log_message(const char *msg)
{
    if (log_fd >= 0) {
        write(log_fd, msg, strlen(msg));
        write(log_fd, "\n", 1);
    }
}

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void hup_handler(int sig)
{
    (void)sig;
    hup_flag = 1;
}

void daemonize()
{
    pid_t pid;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    pid = fork();

    if (pid < 0)
        die("fork");

    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        die("setsid");

    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0)
        die("fork");

    if (pid > 0)
        exit(EXIT_SUCCESS);

    struct rlimit rl;

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        die("getrlimit");

    for (int fd = 0; fd < (int)rl.rlim_max; fd++)
        close(fd);

    if (chdir("/") < 0)
        die("chdir");

    int fd0 = open("/dev/null", O_RDWR);

    if (fd0 < 0)
        die("open /dev/null");

    dup2(fd0, STDIN_FILENO);
    dup2(fd0, STDOUT_FILENO);
    dup2(fd0, STDERR_FILENO);

    log_fd = open(
        "/tmp/myinit.log",
        O_CREAT | O_WRONLY | O_APPEND,
        0644
    );

    if (log_fd < 0)
        exit(EXIT_FAILURE);
}

void free_config()
{
    for (int i = 0; i < process_count; i++) {
        free(processes[i].exec_path);
        free(processes[i].arg1);
        free(processes[i].stdin_file);
        free(processes[i].stdout_file);
    }

    process_count = 0;
}

void read_config()
{
    FILE *f = fopen(config_path, "r");

    if (!f)
        die("fopen config");

    free_config();

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0)
            continue;

        char *exec_path = strtok(line, " ");
        char *arg1 = strtok(NULL, " ");
        char *stdin_file = strtok(NULL, " ");
        char *stdout_file = strtok(NULL, " ");

        if (!exec_path || !arg1 || !stdin_file || !stdout_file) {
            log_message("Invalid config line");
            continue;
        }

        if (exec_path[0] != '/' ||
            stdin_file[0] != '/' ||
            stdout_file[0] != '/') {

            log_message("Non-absolute path detected");
            continue;
        }

        processes[process_count].exec_path = strdup(exec_path);
        processes[process_count].arg1 = strdup(arg1);
        processes[process_count].stdin_file = strdup(stdin_file);
        processes[process_count].stdout_file = strdup(stdout_file);
        processes[process_count].pid = -1;

        process_count++;
    }

    fclose(f);
}

void start_process(int index)
{
    pid_t pid = fork();

    if (pid < 0)
        die("fork");

    if (pid == 0) {

        int in_fd = open(processes[index].stdin_file, O_RDONLY);

        if (in_fd < 0)
            exit(EXIT_FAILURE);

        int out_fd = open(
            processes[index].stdout_file,
            O_CREAT | O_WRONLY | O_APPEND,
            0644
        );

        if (out_fd < 0)
            exit(EXIT_FAILURE);

        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);

        close(in_fd);
        close(out_fd);

        char *args[MAX_ARGS];

        args[0] = processes[index].exec_path;
        args[1] = processes[index].arg1;
        args[2] = NULL;

        execv(args[0], args);

        exit(EXIT_FAILURE);
    }

    processes[index].pid = pid;

    char buffer[256];

    snprintf(
        buffer,
        sizeof(buffer),
        "Started process pid=%d cmd=%s %s",
        pid,
        processes[index].exec_path,
        processes[index].arg1
    );

    log_message(buffer);
}

void start_all_processes()
{
    for (int i = 0; i < process_count; i++)
        start_process(i);
}

void stop_all_processes()
{
    for (int i = 0; i < process_count; i++) {

        if (processes[i].pid > 0) {

            kill(processes[i].pid, SIGTERM);

            char buffer[256];

            snprintf(
                buffer,
                sizeof(buffer),
                "Stopped process pid=%d",
                processes[i].pid
            );

            log_message(buffer);
        }
    }

    while (waitpid(-1, NULL, 0) > 0)
        ;
}

void reload_config()
{
    log_message("Reloading configuration");

    stop_all_processes();

    read_config();

    start_all_processes();
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "c:")) != -1) {

        switch (opt) {

            case 'c':
                strncpy(config_path, optarg, sizeof(config_path) - 1);
                break;

            default:
                fprintf(stderr, "Usage: %s -c config\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (strlen(config_path) == 0) {
        fprintf(stderr, "Config not specified\n");
        exit(EXIT_FAILURE);
    }

    if (config_path[0] != '/') {
        fprintf(stderr, "Config path must be absolute\n");
        exit(EXIT_FAILURE);
    }

    daemonize();

    signal(SIGHUP, hup_handler);

    log_message("myinit started");

    read_config();

    start_all_processes();

    while (1) {

        if (hup_flag) {

            reload_config();

            hup_flag = 0;
        }

        int status;

        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {

            for (int i = 0; i < process_count; i++) {

                if (processes[i].pid == pid) {

                    char buffer[256];

                    snprintf(
                        buffer,
                        sizeof(buffer),
                        "Child pid=%d terminated. Restarting.",
                        pid
                    );

                    log_message(buffer);

                    start_process(i);
                }
            }
        }

        sleep(1);
    }

    return 0;
}