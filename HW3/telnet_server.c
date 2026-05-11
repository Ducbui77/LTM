#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int check_login(char *user, char *pass)
{
    FILE *f = fopen("accounts.txt", "r");

    if (f == NULL)
        return 0;

    char f_user[50], f_pass[50];

    while (fscanf(f, "%s %s", f_user, f_pass) != EOF)
    {
        if (strcmp(user, f_user) == 0 &&
            strcmp(pass, f_pass) == 0)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void send_file(int client, char *filename)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
    {
        send(client,
             "Khong mo duoc file\n",
             strlen("Khong mo duoc file\n"),
             0);
        return;
    }

    char buf[BUFFER_SIZE];

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        send(client, buf, strlen(buf), 0);
    }

    fclose(f);
}

void handle_client(int client)
{
    char buf[BUFFER_SIZE];

    char *login_msg = "Nhap user va pass:\n";

    send(client,
         login_msg,
         strlen(login_msg),
         0);

    int n = recv(client,
                 buf,
                 sizeof(buf) - 1,
                 0);

    if (n <= 0)
    {
        close(client);
        exit(0);
    }

    buf[n] = '\0';

    char user[50], pass[50];

    if (sscanf(buf, "%s %s", user, pass) != 2)
    {
        char *msg = "Sai dinh dang\n";

        send(client,
             msg,
             strlen(msg),
             0);

        close(client);
        exit(0);
    }

    if (!check_login(user, pass))
    {
        char *msg = "Dang nhap that bai\n";

        send(client,
             msg,
             strlen(msg),
             0);

        close(client);
        exit(0);
    }

    char *success = "Dang nhap thanh cong\n";

    send(client,
         success,
         strlen(success),
         0);

    while (1)
    {
        char *cmd_msg = "\nNhap lenh: ";

        send(client,
             cmd_msg,
             strlen(cmd_msg),
             0);

        n = recv(client,
                 buf,
                 sizeof(buf) - 1,
                 0);

        if (n <= 0)
            break;

        buf[n] = '\0';

        buf[strcspn(buf, "\r\n")] = 0;

        if (strcmp(buf, "exit") == 0)
            break;

        char cmd[BUFFER_SIZE + 50];

        snprintf(cmd,
                 sizeof(cmd),
                 "%s > out.txt 2>&1",
                 buf);

        system(cmd);

        send_file(client, "out.txt");
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

    printf("Telnet multiprocessing server running at port %d\n",
           PORT);


    while (1)
    {
        int client =
            accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf("New client connected\n");

        pid_t pid = fork();

        if (pid == 0)
        {
            // Child process

            close(listener);

            handle_client(client);
        }
        else
        {
            // Parent process
            close(client);
            while (waitpid(-1,NULL,WNOHANG) > 0);
        }
    }

    close(listener);

    return 0;
}