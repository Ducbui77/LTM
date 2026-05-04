#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_CLIENTS 20
#define MAX_TOPICS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char topics[MAX_TOPICS][50];
    int topic_count;
} Client;

Client clients[MAX_CLIENTS];

// kiểm tra đã SUB chưa
int is_subscribed(Client *c, char *topic)
{
    for (int i = 0; i < c->topic_count; i++)
    {
        if (strcmp(c->topics[i], topic) == 0)
            return 1;
    }
    return 0;
}

// SUB
void subscribe(Client *c, char *topic)
{
    if (!is_subscribed(c, topic) && c->topic_count < MAX_TOPICS)
    {
        strcpy(c->topics[c->topic_count++], topic);
    }
}

// UNSUB
void unsubscribe(Client *c, char *topic)
{
    for (int i = 0; i < c->topic_count; i++)
    {
        if (strcmp(c->topics[i], topic) == 0)
        {
            for (int j = i; j < c->topic_count - 1; j++)
            {
                strcpy(c->topics[j], c->topics[j + 1]);
            }
            c->topic_count--;
            return;
        }
    }
}

// PUB
void publish(int sender_fd, char *topic, char *msg)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "[%s] %s\n", topic, msg);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1 && clients[i].fd != sender_fd)
        {
            if (is_subscribed(&clients[i], topic))
            {
                send(clients[i].fd, buffer, strlen(buffer), 0);
            }
        }
    }
}

//  MAIN 

int main()
{
    int listen_fd, max_fd;
    struct sockaddr_in server_addr;
    fd_set master_set, read_fds;

    // init clients
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_fd, 5);

    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    max_fd = listen_fd;

    printf("PubSub Server running on port %d...\n", PORT);

    while (1)
    {
        read_fds = master_set;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(1);
        }

        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (!FD_ISSET(fd, &read_fds))
                continue;

            //  NEW CLIENT 
            if (fd == listen_fd)
            {
                int new_fd = accept(listen_fd, NULL, NULL);

                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == -1)
                    {
                        clients[i].fd = new_fd;
                        clients[i].topic_count = 0;
                        added = 1;
                        break;
                    }
                }

                if (!added)
                {
                    close(new_fd);
                    continue;
                }

                FD_SET(new_fd, &master_set);
                if (new_fd > max_fd)
                    max_fd = new_fd;

                send(new_fd,
                     "Connected. Commands: SUB <topic>, UNSUB <topic>, PUB <topic> <msg>\n",
                     74, 0);
            }

            else
            {
                char buf[BUFFER_SIZE];
                int n = recv(fd, buf, sizeof(buf) - 1, 0);

                if (n <= 0)
                {
                    close(fd);
                    FD_CLR(fd, &master_set);

                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i].fd == fd)
                        {
                            clients[i].fd = -1;
                            clients[i].topic_count = 0;
                            break;
                        }
                    }
                    continue;
                }

                buf[n] = '\0';
                buf[strcspn(buf, "\r\n")] = 0;

                // tìm client
                Client *c = NULL;
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == fd)
                    {
                        c = &clients[i];
                        break;
                    }
                }

                if (c == NULL)
                    continue;

                // SUB 
                if (strncmp(buf, "SUB ", 4) == 0)
                {
                    char topic[50];
                    sscanf(buf + 4, "%s", topic);

                    subscribe(c, topic);

                    char msg[100];
                    snprintf(msg, sizeof(msg), "Subscribed %s\n", topic);
                    send(fd, msg, strlen(msg), 0);
                }

                //  UNSUB 
                else if (strncmp(buf, "UNSUB ", 6) == 0)
                {
                    char topic[50];
                    sscanf(buf + 6, "%s", topic);

                    unsubscribe(c, topic);

                    char msg[100];
                    snprintf(msg, sizeof(msg), "Unsubscribed %s\n", topic);
                    send(fd, msg, strlen(msg), 0);
                }

                // PUB 
                else if (strncmp(buf, "PUB ", 4) == 0)
                {
                    char topic[50], message[BUFFER_SIZE];

                    if (sscanf(buf + 4, "%s %[^\n]", topic, message) >= 1)
                    {
                        publish(fd, topic, message);
                    }
                    else
                    {
                        send(fd, "Sai cu phap PUB\n", 16, 0);
                    }
                }

                else
                {
                    send(fd, "Lenh khong hop le\n", 18, 0);
                }
            }
        }
    }

    return 0;
}