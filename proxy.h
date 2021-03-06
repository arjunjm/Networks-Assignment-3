#ifndef __SERVER__H__
#define __SERVER__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
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
#include <set>
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
        std::set<int> serverSockets;
        std::map<int, int> servSockCliSockMap;
        std::map<int, string> sockFileMap;

        /*
         * This map is used for LRU implmenetation.
         * The map contains the HTTP request strings as
         * the key and the corresponding timestamp in seconds
         * (since year 2000) as the value.
         */
        std::map<string, double> requestTimeStampMap;

    public:
        Server(char *serverIP, char *portNum);
        int createSocketAndBind();
        int listenForConnections();
        int acceptConnection();
};

#endif /* __SERVER__H__ */
