#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define WORKERS 4
#define BUFFER_SIZE 1024

void handle_client(int client)
{
    char buf[BUFFER_SIZE];

    int ret = recv(client, buf, sizeof(buf) - 1, 0);

    if (ret > 0)
    {
        buf[ret] = 0;

        printf("PID %d xu ly request:\n%s\n",
               getpid(), buf);

        char *msg =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body>"
            "<h1>Xin chao cac ban</h1>"
            "<p>HTTP Prefork Server</p>"
            "</body></html>";

        send(client, msg, strlen(msg), 0);
    }

    close(client);
}

int main()
{
    int listener;

    struct sockaddr_in server_addr;

    listener = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener,
         (struct sockaddr *)&server_addr,
         sizeof(server_addr));

    listen(listener, 10);

    printf("HTTP Prefork Server running at port %d\n", PORT);

    // PREFORK 
    for (int i = 0; i < WORKERS; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // Child process
            printf("Worker %d started (PID=%d)\n",
                   i, getpid());

            while (1)
            {
                int client =
                    accept(listener, NULL, NULL);

                if (client < 0)
                    continue;

                printf("PID %d accept client %d\n",
                       getpid(), client);

                handle_client(client);
            }

            exit(0);
        }
    }

    // Parent process
    while (1)
    {
        pause();
    }

    close(listener);

    return 0;
}