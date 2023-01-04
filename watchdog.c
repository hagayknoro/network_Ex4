#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/fcntl.h>

#define SERVER_PORT 3000
#define SERVER_IP_ADDRESS "127.0.0.1"

int main()
{
    printf("Hello im watchdog\n");

    //Creat connection
    int listeningSocket, clientSocket;
    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Could not create listening socket : %d\n", errno);
    }
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        printf("setsockopt() failed with error code : %d\n", errno);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    //Bind the socket
    if (bind(listeningSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("server is binding\n\n");

    if (listen(listeningSocket, 500) == -1) {
        printf("listen() failed with error code : %d\n", errno);
        close(listeningSocket);
        return -1;
    }
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

    //Accept and incoming connection
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);
    clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);

    if(clientSocket== -1)
    {
        printf("listen failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }

    int recvPing = 0;

    while (recvPing != 300)
    {
        int return_status = recv(clientSocket, &recvPing, sizeof(recvPing),MSG_DONTWAIT);
        if (return_status <= 0) {
            printf("Problem in receiving start\n");
        }
        if(recvPing == 300)
        {
            printf("start mesg recievde\n");
        }
    }
    recvPing = 0;

    fcntl(listeningSocket, F_SETFL, O_NONBLOCK);
    fcntl(clientSocket, F_SETFL, O_NONBLOCK);



    //Create time variables for timer
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);


    while(1)
    {
        sleep(3);
        int return_status = recv(clientSocket, &recvPing, sizeof(recvPing),0);
        if (return_status <= 0) {
            printf("Problem in receiving \n");
            break;
        }
        recvPing = 0;
    }

    kill(0,SIGINT); // kill all processes in the process group
    close(clientSocket);
    close(listeningSocket);

    return 0;
}
