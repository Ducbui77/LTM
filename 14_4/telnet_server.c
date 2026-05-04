#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int auth;
} Client;

int check_login(char *u, char *p) {
    FILE *f = fopen("accounts.txt", "r");
    if (!f) return 0;

    char fu[50], fp[50];
    while (fscanf(f, "%s %s", fu, fp) != EOF) {
        if (!strcmp(u, fu) && !strcmp(p, fp)) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void send_file(int fd, char *file) {
    FILE *f = fopen(file, "r");
    if (!f) return;

    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), f)) {
        send(fd, buf, strlen(buf), 0);
    }
    fclose(f);
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 5);

    struct pollfd fds[MAX_CLIENTS + 1];
    Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        fds[i+1].fd = -1;
    }

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    printf("Telnet poll server...\n");

    while (1) {
        poll(fds, MAX_CLIENTS + 1, -1);

        if (fds[0].revents & POLLIN) {
            int new_fd = accept(listen_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_fd;
                    clients[i].auth = 0;
                    fds[i+1].fd = new_fd;
                    fds[i+1].events = POLLIN;
                    send(new_fd, "Login (user pass):\n", 20, 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == -1) continue;

            if (fds[i+1].revents & POLLIN) {
                char buf[BUFFER_SIZE];
                int n = recv(clients[i].fd, buf, sizeof(buf)-1, 0);

                if (n <= 0) {
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    fds[i+1].fd = -1;
                    continue;
                }

                buf[n] = '\0';
                buf[strcspn(buf, "\r\n")] = 0;

                if (!clients[i].auth) {
                    char u[50], p[50];
                    if (sscanf(buf, "%s %s", u, p) == 2 && check_login(u,p)) {
                        clients[i].auth = 1;
                        send(clients[i].fd, "OK\n", 3, 0);
                    } else {
                        send(clients[i].fd, "Fail\n", 5, 0);
                    }
                } else {
                    char out[20], cmd[BUFFER_SIZE+50];
                    snprintf(out, sizeof(out), "out%d.txt", clients[i].fd);
                    snprintf(cmd, sizeof(cmd), "%s > %s 2>&1", buf, out);

                    system(cmd);
                    send_file(clients[i].fd, out);
                    unlink(out);
                    send(clients[i].fd, "\n", 1, 0);
                }
            }
        }
    }
}