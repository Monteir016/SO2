#include "api.h"

#include "src/common/constants.h"
#include "src/common/protocol.h"
#include <fcntl.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

char const *reqPipePath;
char const *respPipePath;
char const *notifPipePath;

void cleanup_pipes() {
  unlink(reqPipePath);
  unlink(respPipePath);
  unlink(notifPipePath);
}

int kvs_connect(char const *req_pipe_path, char const *resp_pipe_path,
                char const *server_pipe_path, char const *notif_pipe_path,
                int *notif_pipe) {
  // create pipes and connect
  reqPipePath = req_pipe_path;
  respPipePath = resp_pipe_path;
  notifPipePath = notif_pipe_path;


  if (mkfifo(req_pipe_path, 0666) == -1 || mkfifo(resp_pipe_path, 0666) == -1 || mkfifo(notif_pipe_path, 0666) == -1) {
    perror("Failed to create one of the pipes");
    cleanup_pipes();
    return 1;
  }

  int server_fd = open(server_pipe_path, O_WRONLY);
  if (server_fd == -1) {
    perror("Failed to open server pipe");
    cleanup_pipes();
    return 1;
  }

  char message[3 * MAX_STRING_SIZE + 2];
  snprintf(message, sizeof(message), "%c|%s|%s|%s", OP_CODE_CONNECT, req_pipe_path, resp_pipe_path, notif_pipe_path);
  if (write(server_fd, message, sizeof(message)) == -1) {
    perror("Failed to write to server pipe");
    close(server_fd);
    cleanup_pipes();
    return 1;
  }

  close(server_fd);
  *notif_pipe = open(notif_pipe_path, O_RDONLY | O_NONBLOCK);
  if (*notif_pipe == -1) {
    perror("Failed to open notification pipe");
    cleanup_pipes();
    return 1;
  }
  return 0;

}

int kvs_disconnect(void) {
  // close pipes and unlink pipe files
  if (unlink(reqPipePath) == -1) {
    perror("Failed to unlink request pipe");
    return 1;
  }
  if (unlink(respPipePath) == -1) {
    perror("Failed to unlink response pipe");
    return 1;
  }
  if (unlink(notifPipePath) == -1) {
    perror("Failed to unlink notification pipe");
    return 1;
  }
  return 0;
}

int kvs_subscribe(const char *key) {
  // send subscribe message to request pipe and wait for response in response
  // pipe
  int req_fd = open(reqPipePath, O_WRONLY);
  if (req_fd == -1) {
    perror("Failed to open request pipe");
    return 1;
  }

  char message[MAX_STRING_SIZE + 2];
  snprintf(message, sizeof(message), "%c%-40s", OP_CODE_SUBSCRIBE, key);

  if (write(req_fd, message, sizeof(message)) == -1) {
    perror("Failed to write to request pipe");
    close(req_fd);
    return 1;
  }

  close(req_fd);

  int resp_fd = open(respPipePath, O_RDONLY);
  if (resp_fd == -1) {
    perror("Failed to open response pipe");
    return 1;
  }

  char response;
  if (read(resp_fd, &response, sizeof(response)) == -1) {
    perror("Failed to read from response pipe");
    close(resp_fd);
    return 1;
  }

  close(resp_fd);

  printf("Server returned %d for operation: subscribe\n", response);
  return response == 0 ? 0 : 1;
}

int kvs_unsubscribe(const char *key) {
  // send unsubscribe message to request pipe and wait for response in response
  // pipe
  int req_fd = open(reqPipePath, O_WRONLY);
  if (req_fd == -1) {
    perror("Failed to open request pipe");
    return 1;
  }

  char message[MAX_STRING_SIZE + 2];
  snprintf(message, sizeof(message), "%c%-40s", OP_CODE_UNSUBSCRIBE, key); // FIXME

  if (write(req_fd, message, sizeof(message)) == -1) {
    perror("Failed to write to request pipe");
    close(req_fd);
    return 1;
  }

  close(req_fd);

  int resp_fd = open(respPipePath, O_RDONLY);
  if (resp_fd == -1) {
    perror("Failed to open response pipe");
    return 1;
  }

  char response;
  if (read(resp_fd, &response, sizeof(response)) == -1) {
    perror("Failed to read from response pipe");
    close(resp_fd);
    return 1;
  }

  close(resp_fd);

  printf("Server returned %d for operation: unsubscribe\n", response);
  return response == 0 ? 0 : 1;
}
