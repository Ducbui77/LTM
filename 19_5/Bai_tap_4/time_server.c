#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8888
#define BUFFER_SIZE 1024

void *handle_client(void *arg)
{
    int client = *((int *)arg);

    free(arg);

    char buf[BUFFER_SIZE];

    send(client,
         "Nhap lenh: GET_TIME [format]\n",
         strlen("Nhap lenh: GET_TIME [format]\n"),
         0);

    while (1)
    {
        int n =
            recv(client,
                 buf,
                 sizeof(buf) - 1,
                 0);

        if (n <= 0)
            break;

        buf[n] = '\0';

        // Xoa ky tu xuong dong

        buf[strcspn(buf, "\r\n")] = 0;

        char command[50];
        char format[50];

        int ret =
            sscanf(buf,
                   "%49s %49s",
                   command,
                   format);

        // Kiem tra cu phap

        if (ret != 2 ||
            strcmp(command, "GET_TIME") != 0)
        {
            char *msg =
                "Lenh khong hop le\n";

            send(client,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        time_t now = time(NULL);

        struct tm *t =
            localtime(&now);

        char result[100];

        // dd/mm/yyyy

        if (strcmp(format, "dd/mm/yyyy") == 0)
        {
            strftime(result,
                     sizeof(result),
                     "%d/%m/%Y",
                     t);
        }

        // dd/mm/yy

        else if (strcmp(format, "dd/mm/yy") == 0)
        {
            strftime(result,
                     sizeof(result),
                     "%d/%m/%y",
                     t);
        }

        // mm/dd/yyyy

        else if (strcmp(format, "mm/dd/yyyy") == 0)
        {
            strftime(result,
                     sizeof(result),
                     "%m/%d/%Y",
                     t);
        }

        // mm/dd/yy

        else if (strcmp(format, "mm/dd/yy") == 0)
        {
            strftime(result,
                     sizeof(result),
                     "%m/%d/%y",
                     t);
        }

        else
        {
            char *msg =
                "Format khong hop le\n";

            send(client,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        strcat(result, "\n");

        send(client,
             result,
             strlen(result),
             0);
    }

    close(client);

    printf("Client disconnected\n");

    pthread_exit(NULL);
}

int main()
{
    int listener;

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

    printf("Time server running at port %d\n",
           PORT);

    while (1)
    {
        int client =
            accept(listener,
                   NULL,
                   NULL);

        if (client < 0)
            continue;

        printf("New client connected\n");

        pthread_t tid;

        int *pclient =
            malloc(sizeof(int));

        *pclient = client;

        pthread_create(&tid,
                       NULL,
                       handle_client,
                       pclient);

        pthread_detach(tid);
    }

    close(listener);

    return 0;
}