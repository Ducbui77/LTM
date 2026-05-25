#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int check_login(char *user, char *pass)
{
    FILE *f = fopen("accounts.txt", "r");

    if (f == NULL)
        return 0;

    char f_user[50];
    char f_pass[50];

    while (fscanf(f,
                  "%49s %49s",
                  f_user,
                  f_pass) != EOF)
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

void send_file(int client,
               char *filename)
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

    while (fgets(buf,
                 sizeof(buf),
                 f) != NULL)
    {
        send(client,
             buf,
             strlen(buf),
             0);
    }

    fclose(f);
}

void *handle_client(void *arg)
{
    int client =
        *((int *)arg);

    free(arg);

    char buf[BUFFER_SIZE];

    char user[50];
    char pass[50];

    while (1)
    {
        send(client,
             "Nhap user va pass:\n",
             strlen("Nhap user va pass:\n"),
             0);

        int n =
            recv(client,
                 buf,
                 sizeof(buf) - 1,
                 0);

        if (n <= 0)
        {
            close(client);
            pthread_exit(NULL);
        }

        buf[n] = '\0';

        // Xoa xuong dong

        buf[strcspn(buf, "\r\n")] = 0;

        // Doc user pass

        if (sscanf(buf,
                   "%49s %49s",
                   user,
                   pass) != 2)
        {
            send(client,
                 "Sai dinh dang\n",
                 strlen("Sai dinh dang\n"),
                 0);

            continue;
        }

        // Kiem tra login

        if (check_login(user, pass))
        {
            send(client,
                 "Dang nhap thanh cong\n",
                 strlen("Dang nhap thanh cong\n"),
                 0);

            break;
        }

        // Login sai

        send(client,
             "Dang nhap that bai. Thu lai\n",
             strlen("Dang nhap that bai. Thu lai\n"),
             0);
    }

    while (1)
    {
        send(client,
             "\nNhap lenh: ",
             strlen("\nNhap lenh: "),
             0);

        int n =
            recv(client,
                 buf,
                 sizeof(buf) - 1,
                 0);

        if (n <= 0)
            break;

        buf[n] = '\0';

        // Xoa xuong dong

        buf[strcspn(buf, "\r\n")] = 0;

        // Thoat

        if (strcmp(buf, "exit") == 0)
            break;

        // Tao file tam rieng cho thread

        char outfile[100];

        snprintf(outfile,
                 sizeof(outfile),
                 "out_%ld.txt",
                 pthread_self());

        // Tao command

        char cmd[BUFFER_SIZE + 200];

        snprintf(cmd,
                 sizeof(cmd),
                 "%s > %s 2>&1",
                 buf,
                 outfile);

        // Thuc thi lenh

        system(cmd);

        // Gui ket qua

        send_file(client,
                  outfile);

        // Xoa file tam

        remove(outfile);
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

    printf("Telnet multithread server running at port %d\n",
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