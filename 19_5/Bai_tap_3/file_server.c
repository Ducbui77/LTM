#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define FOLDER "./files"

void send_file_list(int client)
{
    DIR *dir = opendir(FOLDER);

    if (dir == NULL)
    {
        send(client,
             "ERROR No files to download\r\n",
             strlen("ERROR No files to download\r\n"),
             0);

        close(client);
        exit(0);
    }

    struct dirent *entry;

    char list[4096] = "";

    int count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
        {
            count++;
        }
    }

    if (count == 0)
    {
        send(client,
             "ERROR No files to download\r\n",
             strlen("ERROR No files to download\r\n"),
             0);

        closedir(dir);

        close(client);

        exit(0);
    }

    rewinddir(dir);

    // Dong dau
    char first_line[100];

    sprintf(first_line,
            "OK %d\r\n",
            count);

    strcat(list, first_line);

    // Them ten file
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
        {
            strcat(list, entry->d_name);
            strcat(list, "\r\n");
        }
    }

    strcat(list, "\r\n");

    send(client,
         list,
         strlen(list),
         0);

    closedir(dir);
}

// Gui file cho client
void send_file(int client, char *filename)
{
    char path[512];

    snprintf(path,
             sizeof(path),
             "%s/%s",
             FOLDER,
             filename);

    FILE *f = fopen(path, "rb");

    if (f == NULL)
    {
        send(client,
             "ERROR File not found\r\n",
             strlen("ERROR File not found\r\n"),
             0);

        return;
    }

    fseek(f, 0, SEEK_END);

    long size = ftell(f);

    rewind(f);

    // Gui header
    char header[100];

    sprintf(header,
            "OK %ld\r\n",
            size);

    send(client,
         header,
         strlen(header),
         0);

    // Gui noi dung file
    char buffer[BUFFER_SIZE];

    int n;

    while ((n = fread(buffer,
                      1,
                      sizeof(buffer),
                      f)) > 0)
    {
        send(client,
             buffer,
             n,
             0);
    }

    fclose(f);
}

// Xu ly client
void handle_client(int client)
{
    char buf[BUFFER_SIZE];

    // Gui danh sach file
    send_file_list(client);

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
        send_file(client, buf);
    }

    close(client);

    printf("Client disconnected\n");

    exit(0);
}

// Server loop

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

    // Bind
    if (bind(listener,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    // Listen
    if (listen(listener, 5) < 0)
    {
        perror("listen");
        exit(1);
    }

    printf("File server running at port %d\n",
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