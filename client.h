#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "helper.h"

#define PORT "3490" // the port client will be connecting to 

class Client
{
    private:
        int sockFd;
        char userName[15];
        char hostName[15];
        char serverPort[15];
        struct addrinfo hints;
        bool isConnected;


    public:
        Client(char *userName, char *hostName, char *serverPort);
        int connectToHost();
        int sendData(void * buf, size_t len, int flags = 0);
        int recvData();
        bool getConnectionStatus();
        int  getSocketFd();
        char* getUserName();
};

#endif /* __CLIENT_H__ */
