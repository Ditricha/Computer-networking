#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define SOCK_ADDR "localhost"
#define SOCK_PORT 23232
#define MSG_LEN 500
#define BUF_LEN 500

using namespace std;

string GotGet(string filename)
{
    const string version = "HTTP/1.1";
    return "GET /" + filename + " " + version + "\r\n" + "User: console-pid-" + to_string(getpid());
}

void perror_and_exit(char* s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    srand(time(NULL));

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        perror_and_exit("[-] Error in socket");

    struct hostent* host = gethostbyname(SOCK_ADDR);
    if (!host)
        perror_and_exit("[-] Error in gethostbyname");

    struct sockaddr_in server_addr;
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    server_addr.sin_addr = *((struct in_addr*)host->h_addr_list[0]);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        perror_and_exit("[-] Error in connect");

    string request = GotGet("index.html");
    if (send(sock, request.c_str(), request.length(), 0) < 0)
        perror_and_exit("[-] Error in send");

    printf("[+] Client has send a get request, waiting for response...\n");

    char buf[BUF_LEN];
    if (recv(sock, buf, BUF_LEN, 0) < 0)
        perror_and_exit("[-] Error in receive");

    printf("[+] Client has recieved an answer:\n\n%s", buf);
    close(sock);
    return 0;
}

