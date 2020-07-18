#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <dirent.h>

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define DIRECTORY "./info"
#define HOST "localhost"
#define MAX_CLI 30
#define BUF_SIZE 1024

DIR *dirptr;

int num_of_connections = 0;

char http_err[] = "HTTP/1.1 404 Not Found\r\n\r\n404 Page not found";
char http_ok[] = "HTTP/1.1 200 OK\r\n";

void *client_handle(void *);
char *getFileType(char *file);

int main(int argc, char const *argv[])
{
    int opt = TRUE;
    int sock_main, addrlen, new_socket;
    struct sockaddr_in address;

    if ((dirptr = opendir(DIRECTORY)) == NULL)
    {
        printf("Directory Not Found!\n");
        exit(1);
    }

    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error:");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock_main, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sock_main, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening ...\n");

    if (listen(sock_main, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE)
    {
        if ((new_socket = accept(sock_main, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        if (num_of_connections >= MAX_CLI)
        {
            printf("Max client connections limit reached\n");
            close(new_socket);
            continue;
        }
        pthread_t thread_id;
        int *socket = malloc(sizeof(int));
        *socket = new_socket;
        num_of_connections++;
        pthread_create(&thread_id, NULL, client_handle, (void *)socket);
    }

    return 0;
}

void *client_handle(void *_args)
{
    FILE *fileptr;
    time_t timenow;
    struct tm *timeinfo;
    time(&timenow);
    timeinfo = localtime(&timenow);
    char *header, *request, *path, *newpath;
    char get[5], http[12];
    char filepath[BUF_SIZE];
    char buffer[BUF_SIZE];
    char *contentType;
    int socketfd = *(int *)_args;
    char *root_page_path = "index.html";

    header = (char *)malloc(BUF_SIZE * sizeof(char));
    request = (char *)malloc(BUF_SIZE * sizeof(char));
    path = (char *)malloc(BUF_SIZE * sizeof(char));

    recv(socketfd, request, BUF_SIZE, 0);
    sscanf(request, "%s %s %s", get, path, http);
    newpath = path + 1;
    if (strlen(newpath) == 0)
        newpath = root_page_path;
    sprintf(filepath, "%s/%s", DIRECTORY, newpath);
    contentType = getFileType(newpath);
    printf("%s %s", get, filepath);
    if ((fileptr = fopen(filepath, "r")) == NULL)
    {
        printf(" -- 404 NOT FOUND\n");
        send(socketfd, http_err, strlen(http_err), 0);
    }
    else
    {
        printf(" -- 200 OK\n");
        sprintf(header, "%sDate: %sHost: %s:%d\r\nLocation: %s\nContent-Type: %s\r\n\r\n",
                http_ok, asctime(timeinfo), HOST, PORT, newpath, contentType);

        send(socketfd, header, strlen(header), 0);

        memset(&buffer, 0, sizeof(buffer));
        while (!feof(fileptr))
        {
            int n = fread(&buffer, 1, sizeof(buffer), fileptr);
            send(socketfd, buffer, n, 0);
            memset(&buffer, 0, sizeof(buffer));
        }
    }
    close(socketfd);
    free(header);
    free(request);
    free(path);
    num_of_connections--;
    return NULL;
}

char *getFileType(char *file)
{
    char *temp;
    if ((temp = strstr(file, ".html")) != NULL)
    {
        return "text/html";
    }
    else if ((temp = strstr(file, ".pdf")) != NULL)
    {
        return "application/pdf";
    }
    else if ((temp = strstr(file, ".txt")) != NULL)
    {
        return "text/html";
    }
    else if ((temp = strstr(file, ".js")) != NULL)
    {
        return "application/javascript";
    }
    else if ((temp = strstr(file, ".css")) != NULL)
    {
        return "text/css";
    }
    else if ((temp = strstr(file, ".jpeg")) != NULL)
    {
        return "image/jpeg";
    }
    return "application/octet-stream";
}