#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define PORT 8888
#define BUFFER_SIZE 1024

typedef struct
{
    int client1;
    int client2;
} ChatPair;

void *chat_thread(void *arg)
{
    ChatPair *pair = (ChatPair *)arg;

    int c1 = pair->client1;
    int c2 = pair->client2;

    char buf[BUFFER_SIZE];
    char *msg = "Da ghep cap thanh cong!\n";
    send(c1,
         msg,
         strlen(msg),
         0);

    send(c2,
         msg,
         strlen(msg),
         0);

    fd_set readfds;

    int maxfd = (c1 > c2) ? c1 : c2;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(c1, &readfds);
        FD_SET(c2, &readfds);

        int activity =
            select(maxfd + 1,
                   &readfds,
                   NULL,
                   NULL,
                   NULL);

        if (activity < 0)
            break;

        if (FD_ISSET(c1, &readfds))
        {
            int n =
                recv(c1,
                     buf,
                     sizeof(buf) - 1,
                     0);

            if (n <= 0)
            {
                send(c2,
                     "Client kia da ngat ket noi\n",
                     strlen("Client kia da ngat ket noi\n"),
                     0);

                break;
            }

            buf[n] = '\0';

            send(c2,
                 buf,
                 strlen(buf),
                 0);
        }

        if (FD_ISSET(c2, &readfds))
        {
            int n =
                recv(c2,
                     buf,
                     sizeof(buf) - 1,
                     0);

            if (n <= 0)
            {
                send(c1,
                     "Client kia da ngat ket noi\n",
                     strlen("Client kia da ngat ket noi\n"),
                     0);

                break;
            }

            buf[n] = '\0';

            send(c1,
                 buf,
                 strlen(buf),
                 0);
        }
    }

    close(c1);
    close(c2);
    free(pair);
    printf("Chat pair disconnected\n");
    pthread_exit(NULL);
}

int main()
{
    int listener;

    struct sockaddr_in server_addr;

    listener =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    int opt = 1;

    setsockopt(listener,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr.sin_port = htons(PORT);

    bind(listener,
         (struct sockaddr *)&server_addr,
         sizeof(server_addr));

    listen(listener, 10);

    printf("Chat server running at port %d\n",
           PORT);
    int waiting_client = -1;

    while (1)
    {
        int client =
            accept(listener,
                   NULL,
                   NULL);

        if (client < 0)
            continue;

        printf("New client connected: %d\n",
               client);
        if (waiting_client == -1)
        {
            waiting_client = client;

            send(client,
                 "Dang cho client khac...\n",
                 strlen("Dang cho client khac...\n"),
                 0);
        }

        else
        {
            ChatPair *pair =
                malloc(sizeof(ChatPair));

            pair->client1 = waiting_client;

            pair->client2 = client;

            pthread_t tid;

            pthread_create(&tid,
                           NULL,
                           chat_thread,
                           pair);

            pthread_detach(tid);

            waiting_client = -1;
        }
    }
    close(listener);
    return 0;
}