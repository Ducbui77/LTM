#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8888
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct
{
    int socket;
    char client_id[50];
    char client_name[50];
    int registered;

} Client;

Client clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex =
    PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message,
                       int sender_socket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != 0 &&
            clients[i].socket != sender_socket &&
            clients[i].registered)
        {
            send(clients[i].socket,
                 message,
                 strlen(message),
                 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
    int client_socket = *((int *)arg);

    free(arg);

    char buffer[BUFFER_SIZE];

    int client_index = -1;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == 0)
        {
            clients[i].socket = client_socket;
            clients[i].registered = 0;
            client_index = i;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    char *welcome =
        "Nhap theo cu phap: client_id: client_name\n";

    send(client_socket,
         welcome,
         strlen(welcome),
         0);

    while (1)
    {
        int n =
            recv(client_socket,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

        if (n <= 0)
        {
            close(client_socket);

            pthread_mutex_lock(&clients_mutex);

            clients[client_index].socket = 0;

            pthread_mutex_unlock(&clients_mutex);

            pthread_exit(NULL);
        }

        buffer[n] = '\0';

        buffer[strcspn(buffer, "\r\n")] = 0;

        char id[50];
        char name[50];

        if (sscanf(buffer,
                   "%49[^:]: %49s",
                   id,
                   name) == 2 ||

            sscanf(buffer,
                   "%49[^:]:%49s",
                   id,
                   name) == 2)
        {
            strncpy(clients[client_index].client_id,
                    id,
                    sizeof(clients[client_index].client_id) - 1);

            strncpy(clients[client_index].client_name,
                    name,
                    sizeof(clients[client_index].client_name) - 1);

            clients[client_index]
                .client_id[
                    sizeof(clients[client_index].client_id) - 1] = '\0';

            clients[client_index]
                .client_name[
                    sizeof(clients[client_index].client_name) - 1] = '\0';

            clients[client_index].registered = 1;

            char *ok =
                "Dang ky thanh cong!\n";

            send(client_socket,
                 ok,
                 strlen(ok),
                 0);

            printf("Client registered: %s - %s\n",
                   clients[client_index].client_id,
                   clients[client_index].client_name);

            break;
        }
        else
        {
            char *error =
                "Sai cu phap. Nhap lai: client_id: client_name\n";

            send(client_socket,
                 error,
                 strlen(error),
                 0);
        }
    }

    while (1)
    {
        int n =
            recv(client_socket,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

        if (n <= 0)
            break;

        buffer[n] = '\0';

        buffer[strcspn(buffer, "\r\n")] = 0;

        // Lay thoi gian

        time_t now = time(NULL);

        struct tm *t =
            localtime(&now);

        char time_str[100];

        strftime(time_str,
                 sizeof(time_str),
                 "%Y/%m/%d %I:%M:%S%p",
                 t);

        // Tao message

        char final_msg[1500];

        snprintf(final_msg,
                 sizeof(final_msg),
                 "%s %s:%s %s\n",
                 time_str,
                 clients[client_index].client_id,
                 clients[client_index].client_name,
                 buffer);

        // Broadcast

        broadcast_message(final_msg,
                          client_socket);
    }

    close(client_socket);

    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket = 0;

    clients[client_index].registered = 0;

    pthread_mutex_unlock(&clients_mutex);

    printf("Client disconnected\n");

    pthread_exit(NULL);
}

int main()
{
    int server_socket;

    struct sockaddr_in server_addr;

    // Khoi tao danh sach client

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket = 0;
        clients[i].registered = 0;
    }

    // Tao socket

    server_socket =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    int opt = 1;

    setsockopt(server_socket,
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

    bind(server_socket,
         (struct sockaddr *)&server_addr,
         sizeof(server_addr));

    // Listen

    listen(server_socket, 10);

    printf("Chat server running at port %d\n",
           PORT);

    while (1)
    {
        int client_socket =
            accept(server_socket,
                   NULL,
                   NULL);

        if (client_socket < 0)
            continue;

        printf("New client connected\n");

        pthread_t tid;

        int *pclient =
            malloc(sizeof(int));

        *pclient = client_socket;

        pthread_create(&tid,
                       NULL,
                       handle_client,
                       pclient);

        pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}