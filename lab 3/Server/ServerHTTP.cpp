#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Threads.hpp"

#define SOCK_ADDR "localhost"
#define SOCK_PORT 23232
#define MSG_LEN 500

#define PATH_STAT "../stat/test.txt"
#define PATH_ROOT "../root/"

#define MAX_CLIENTS 5
int clients[MAX_CLIENTS] = { 0 };

using namespace std;


void perror_and_exit(char* s)
{
    perror(s);
    exit(1);
}

string GetExtention(string filename)
{
    int pos = filename.rfind('.');
    if (pos > 0)
        return filename.substr(pos + 1);
    return "";
}

void Log(string name, string extention)
{
    ofstream fout(PATH_STAT, ios::app);
    fout << "\nName: " + name + ";\nExtention: " + extention + "\n\n";
}

int HandleRequest(char* message, int client_id)
{
    string debug(message);
    char* method = strtok(message, " ");
    char* filename = strtok(NULL, " /");
    char* version = strtok(NULL, " \r\n");
    char* tag = strtok(NULL, "\r\n:");
    char user[50];

    if (tag && !strcmp(tag, "User"))
        strcpy(user, strtok(NULL, " \r\n"));
    else
        strcpy(user, "Unknown");

    if (strcmp(method, "GET"))
        return -1;

    int rc = 0;
    string status, status_code;
    string body = "";
    string content_type = "text/html";

    string response(version);
    ifstream fin(PATH_ROOT + string(filename));
    if (fin.is_open()) {
        string extention = GetExtention(filename);
        Log(user, extention);

        if (extention == "ico")
            content_type = "image/x-icon";

        string buffer;
        while (getline(fin, buffer))
            body += buffer + "\n";
        status_code = "200";
        status = "OK";
    }
    else {
        rc = -2;
        status_code = "404";
        status = "Not Found";
        body = "<html>\n\r<body>\n\r<h1>404 Not Found</h1>\n\r</body>\n\r</html>";
    }
    response.append(" " + status_code + " " + status + "\r\n");
    response.append("Content-Length: " + to_string(body.length()) + "\r\n");
    response.append("Connection: closed\r\n");
    response.append("Content-Type: " + content_type + "; charset=UTF-8\r\n");
    response.append("\r\n");
    response.append(body);

    send(clients[client_id], response.c_str(), response.size(), 0);

    if (rc)
        printf("Client #%d has sent an invalid message.\n", client_id);
    else
        printf("Successfully handled a request from client #%d\n", client_id);
    return rc;
}

void HandleNewConnection(unsigned int socketfd)
{
    struct sockaddr_in client_addr;
    int addrSize = sizeof(client_addr);
    int clientfd = accept(socketfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrSize);
    if (clientfd < 0)
        perror_and_exit("[-] Error while accepting connection");

    printf("\nNew connection: \nClient socket fd = %d\nip = %s:%d\n",
        clientfd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    int i = 0;
    while (i < MAX_CLIENTS && clients[i])
        i++;
    if (i < MAX_CLIENTS) {
        clients[i] = clientfd;
        printf("Connected client %d\n\n", i);
    }
}

void RemoveClient(int fd, int id)
{
    struct sockaddr_in client_addr;
    int addrSize = sizeof(client_addr);

    getpeername(fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrSize);
    printf("Client %d has disconnected (ip %s:%d).\n", id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    close(fd);
    clients[id] = 0;
}

int main(void)
{
    Threads threads;
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Error in sock: ");
        return listener;
    }

    struct sockaddr_in client_addr;
    client_addr.sin_family = PF_INET;
    client_addr.sin_port = htons(SOCK_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error in bind");
        return -1;
    }
    printf("Server is listening on the %d port...\n", SOCK_PORT);

    if (listen(listener, MAX_CLIENTS) < 0) {
        perror("Error in listen");
        return -1;
    }
    printf("Waiting for the connections...\n");
    while (1) {
        fd_set readfds;
        int max_fd;
        int active_clients_count;

        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i];
            if (fd > 0)
                FD_SET(fd, &readfds);
            if (fd > max_fd)
                max_fd = fd;
        }
        active_clients_count = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (active_clients_count < 0 && (errno != EINTR)) {
            perror("Error in select");
            return active_clients_count;
        }

        if (FD_ISSET(listener, &readfds))
            HandleNewConnection(listener);

        for (int i = 0; i < MAX_CLIENTS; i++)
            if (clients[i] > 0 && FD_ISSET(clients[i], &readfds)) {
                char message[MSG_LEN];
                int message_size = recv(clients[i], message, MSG_LEN, 0);
                if (message_size == -1)
                    perror_and_exit("Error while recieving message");
                else if (message_size == 0)
                    RemoveClient(clients[i], i);
                else
                    threads.add(HandleRequest, message, i);
            }
    }
    return 0;
}