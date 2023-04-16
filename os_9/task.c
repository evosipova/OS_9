#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MESSAGE_LIMIT 10

int main() {
  int pipefd[2];
  pid_t child_pid;
  sem_t *semaphore;
  int message_count = 0;

  // Создаем неименованный канал
  if (pipe(pipefd) == -1) {
    perror("Ошибка создания канала");
    exit(1);
  }

  // Создаем семафор
  semaphore = sem_open("/my_semaphore", O_CREAT, 0666, 0);
  if (semaphore == SEM_FAILED) {
    perror("Ошибка создания семафора");
    exit(1);
  }

  // Создаем дочерний процесс
  child_pid = fork();

  if (child_pid == -1) {
    perror("Ошибка создания дочернего процесса");
    exit(1);
  } else if (child_pid == 0) {
    // Дочерний процесс

    close(pipefd[1]); // Закрываем конец канала для записи

    while (message_count < MESSAGE_LIMIT) {
      char message[100];
      ssize_t bytes_read = read(pipefd[0], message, sizeof(message));
      if (bytes_read > 0) {
        printf("Дочерний процесс получил сообщение: %.*s", (int)bytes_read,
               message);
        sem_post(semaphore); // Увеличиваем счетчик семафора
        message_count++;
      }
    }

    sem_close(semaphore); // Закрываем семафор
    close(pipefd[0]); // Закрываем конец канала для чтения
    exit(0);
  } else {
    // Родительский процесс

    close(pipefd[0]); // Закрываем конец канала для чтения

    while (message_count < MESSAGE_LIMIT) {
      char message[100];
      printf("Введите сообщение для передачи: ");
      fgets(message, sizeof(message), stdin);
      ssize_t bytes_written = write(pipefd[1], message, strlen(message));
      if (bytes_written > 0) {
        sem_wait(semaphore); // Ожидаем счетчика семафора
        printf("Родительский процесс передал сообщение: %s", message);
        message_count++;
      }
    }

    sem_close(semaphore); // Закрываем семафор
    close(pipefd[1]); // Закрываем конец канала для записи

    // Ожидаем завершения дочернего процесса
    int status;
    waitpid(child_pid, &status, 0);

    if (WIFEXITED(status)) {
      printf("Дочерний процесс завершился с кодом: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("Дочерний процесс завершился с сигналом: %d\n", WTERMSIG(status));
    }

    sem_unlink("/my_semaphore"); // Удаляем семафор
    exit(0);
  }
}