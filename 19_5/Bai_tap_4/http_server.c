#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define THREADS 4
#define BUFFER_SIZE 1024

int listener;

void handle_client(int client)
{
    char buf[BUFFER_SIZE];

    int ret =
        recv(client,
             buf,
             sizeof(buf) - 1,
             0);

    if (ret > 0)
    {
        buf[ret] = 0;

        printf("Thread %ld xu ly request:\n%s\n",
               pthread_self(),
               buf);

        char *msg =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body>"
            "<h1>Xin chao cac ban</h1>"
            "<p>HTTP Prethread Server</p>"
            "</body></html>";

        send(client,
             msg,
             strlen(msg),
             0);
    }

    close(client);
}

void *worker_thread(void *arg)
{
    while (1)
    {
        int client =
            accept(listener,
                   NULL,
                   NULL);

        if (client < 0)
            continue;

        printf("Thread %ld accept client %d\n",
               pthread_self(),
               client);

        handle_client(client);
    }

    pthread_exit(NULL);
}

int main()
{
    struct sockaddr_in server_addr;

    // Tao socket

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

    // Cau hinh server

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);

    // Bind

    bind(listener,
         (struct sockaddr *)&server_addr,
         sizeof(server_addr));

    // Listen

    listen(listener, 10);

    printf("HTTP Prethread Server running at port %d\n",
           PORT);

    // Tao thread worker

    pthread_t tids[THREADS];

    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&tids[i],
                       NULL,
                       worker_thread,
                       NULL);

        printf("Thread worker %d started\n",
               i);
    }

    // Cho thread chay

    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(tids[i],
                     NULL);
    }

    close(listener);

    return 0;
}