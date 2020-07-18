#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <dirent.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define DIRECTORY "./info"
#define HOST "localhost"
#define MAX_CLI 30
#define BUF_SIZE 1024

DIR *dirptr;

char http_err[] = "HTTP/1.1 404 Not Found\r\n\r\n404 Page not found";
char http_ok[] = "HTTP/1.1 200 OK\r\n";

char *getFileType(char *file);
void handleClient(int[], int, int);

int main(int argc, char *argv[])
{
    int opt = TRUE;
    int sock_main, addrlen, new_socket;
    int clients[MAX_CLI];
    int activity, i, sd;
    int max_sd;
    struct sockaddr_in address;

    fd_set readfds;

    if ((dirptr = opendir(DIRECTORY)) == NULL)
    {
        printf("Directory Not Found!\n");
        exit(1);
    }

    for (i = 0; i < MAX_CLI; i++)
    {
        clients[i] = 0;
    }

    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
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

    if (listen(sock_main, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    while (TRUE)
    {
        FD_ZERO(&readfds);

        FD_SET(sock_main, &readfds);
        max_sd = sock_main;

        for (i = 0; i < MAX_CLI; i++)
        {
            sd = clients[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(sock_main, &readfds))
        {
            if ((new_socket = accept(sock_main, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            for (i = 0; i < MAX_CLI; i++)
            {
                if (clients[i] == 0)
                {
                    clients[i] = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < MAX_CLI; i++)
        {
            sd = clients[i];

            if (FD_ISSET(sd, &readfds))
            {
                handleClient(clients, i, sd);
            }
        }
    }

    return 0;
}

void handleClient(int clients[], int index, int socketfd)
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
        sprintf(header, "%sDate: %sHost: %s:%d\r\nLocation: %s\nContent-Type: %s\r\n\r\n", http_ok, asctime(timeinfo), HOST, PORT, newpath, contentType);

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
    clients[index] = 0;
    free(header);
    free(request);
    free(path);
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
