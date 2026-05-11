#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 8888
#define BUFFER_SIZE 1024

void send_time(int client, char *format)
{
    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    char result[100];

    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(result,
                 sizeof(result),
                 "%d/%m/%Y",
                 t);
    }

    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(result,
                 sizeof(result),
                 "%d/%m/%y",
                 t);
    }

    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(result,
                 sizeof(result),
                 "%m/%d/%Y",
                 t);
    }

    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(result,
                 sizeof(result),
                 "%m/%d/%y",
                 t);
    }

    else
    {
        char *msg = "Sai dinh dang thoi gian\n";

        send(client,
             msg,
             strlen(msg),
             0);

        return;
    }

    strcat(result, "\n");

    send(client,
         result,
         strlen(result),
         0);
}

void handle_client(int client)
{
    char buf[BUFFER_SIZE];

    char *welcome =
        "Nhap lenh: GET_TIME [format]\n";

    send(client,
         welcome,
         strlen(welcome),
         0);

    while (1)
    {
        int n = recv(client,
                     buf,
                     sizeof(buf) - 1,
                     0);

        if (n <= 0)
            break;

        buf[n] = '\0';

        buf[strcspn(buf, "\r\n")] = 0;

        char command[50];
        char format[50];

        if (sscanf(buf,
                   "%s %s",
                   command,
                   format) != 2)
        {
            char *msg =
                "Sai cu phap\n";

            send(client,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        if (strcmp(command,
                   "GET_TIME") != 0)
        {
            char *msg =
                "Lenh khong hop le\n";

            send(client,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        send_time(client, format);
    }

    close(client);

    printf("Client disconnected\n");

    exit(0);
}

int main()
{
    int listener;

    struct sockaddr_in server_addr;

    listener = socket(AF_INET,
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

    if (bind(listener,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(listener, 5) < 0)
    {
        perror("listen");
        exit(1);
    }

    printf("Time multiprocessing server running at port %d\n",
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

        pid_t pid = fork();

        if (pid == 0)
        {

            close(listener);

            handle_client(client);
        }
        else
        {

            close(client);

            while (waitpid(-1,
                           NULL,
                           WNOHANG) > 0);
        }
    }

    close(listener);

    return 0;
}