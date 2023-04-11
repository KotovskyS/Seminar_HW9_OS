#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define MSG_COUNT 10
#define BUFFER_SIZE 32

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

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Дочерний процесс
        close(pipe_fd[1]);

        for (int i = 0; i < MSG_COUNT; i++) {
            sem_wait(sem_child);
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

        for (int i = 0; i < MSG_COUNT; i++) {
            sem_wait(sem_parent);
            snprintf(message, BUFFER_SIZE, "Message %d", i + 1);
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
