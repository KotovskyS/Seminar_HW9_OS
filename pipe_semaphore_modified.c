#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 32

volatile sig_atomic_t stop = 0;

void sigint_handler(int sig) {
    stop = 1;
}

int main() {
    int pipe_fd[2];
    pid_t pid;
    char message[BUFFER_SIZE];
    sem_t *sem_parent, *sem_child;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    sem_unlink("/sem_parent");
    sem_unlink("/sem_child");

    sem_parent = sem_open("/sem_parent", O_CREAT | O_EXCL, 0644, 1);
    sem_child = sem_open("/sem_child", O_CREAT | O_EXCL, 0644, 0);

    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, sigint_handler);

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Дочерний процесс
        close(pipe_fd[1]);

        while (!stop) {
            sem_wait(sem_child);
            if (stop) break;

            read(pipe_fd[0], message, BUFFER_SIZE);
            printf("Child received: %s\n", message);

            sem_post(sem_parent);
        }

        close(pipe_fd[0]);
        sem_close(sem_child);
        sem_close(sem_parent);
        sem_unlink("/sem_child");
        sem_unlink("/sem_parent");
    } else { // Родительский процесс
        close(pipe_fd[0]);

        int counter = 1;
        while (!stop) {
            sem_wait(sem_parent);
            if (stop) break;

            snprintf(message, BUFFER_SIZE, "Message %d", counter++);
            write(pipe_fd[1], message, BUFFER_SIZE);
            printf("Parent sent: %s\n", message);

            sem_post(sem_child);
        }

        close(pipe_fd[1]);
        wait(NULL);
        sem_close(sem_child);
        sem_close(sem_parent);
        sem_unlink("/sem_child");
        sem_unlink("/sem_parent");
    }

    return 0;
}

