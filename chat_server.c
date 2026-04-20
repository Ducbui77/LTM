#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int registered;
    char id[50];
} Client;

void get_time_str(char *buf) {
    time_t now = time(NULL);
    strftime(buf, 64, "%Y/%m/%d %H:%M:%S", localtime(&now));
}

void broadcast(Client clients[], int sender, char *msg) {
    char time_str[64];
    get_time_str(time_str);

    char final[BUFFER_SIZE + 100];
    snprintf(final, sizeof(final), "%s %s: %s\n",
             time_str, clients[sender].id, msg);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && i != sender && clients[i].registered) {
            send(clients[i].fd, final, strlen(final), 0);
        }
    }

    printf("%s", final);
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
        clients[i].registered = 0;
        fds[i+1].fd = -1;
    }

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    printf("Chat server poll running...\n");

    while (1) {
        poll(fds, MAX_CLIENTS + 1, -1);

        // New connection
        if (fds[0].revents & POLLIN) {
            int new_fd = accept(listen_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_fd;
                    fds[i+1].fd = new_fd;
                    fds[i+1].events = POLLIN;
                    send(new_fd, "Nhap: id: name\n", 17, 0);
                    break;
                }
            }
        }

        // Client data
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

                if (!clients[i].registered) {
                    char id[50], name[50];
                    if (sscanf(buf, "%49[^:]: %49s", id, name) == 2) {
                        strcpy(clients[i].id, id);
                        clients[i].registered = 1;
                        send(clients[i].fd, "OK\n", 3, 0);
                    } else {
                        send(clients[i].fd, "Sai format\n", 11, 0);
                    }
                } else {
                    broadcast(clients, i, buf);
                }
            }
        }
    }
}