#ifndef __SERVER__H__
#define __SERVER__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <map>
#include "helper.h"
#include <string>

#define PORT "3490"
#define BACKLOG 10

using namespace std;

class Server
{
    private:
        struct addrinfo hints;
        int sockFd;
        int newConnFd;
        char serverIP[20];
        char portNum[10];
        int maxConnections;
        std::map<int, string> fdUserMap;
        std::map<string, UserStatusT> userStatusMap;

    public:
        Server(char *serverIP, char *portNum);
        int createSocketAndBind();
        int listenForConnections();
        int acceptConnection();
        /*
         * This function returns the client count and the 
         * users connected in the form of a string
         */
        std::string getUserInfo();
};

#endif /* __SERVER__H__ */
