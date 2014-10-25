#include "client.h"
#include <iostream>
using namespace std;

Client::Client(char *hName, char *servPort, char *URL)
{
    isConnected = false;
    //strcpy(this->userName, usrName);
    strcpy(this->hostName, hName);
    strcpy(this->serverPort, servPort);
    strcpy(this->URLToRetrieve, URL);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
}

int Client::connectToHost()
{
    struct addrinfo *serverInfo;
    char s[INET6_ADDRSTRLEN];
    int rVal = getaddrinfo(this->hostName, this->serverPort, &this->hints, &serverInfo);
    if (rVal != 0)
    {
        // Non-zero return value indicates an error
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rVal));
        return -1;
    }

    /*
     * Loop through results and connect to the first we can
     */
    struct addrinfo *temp;
    for (temp = serverInfo; temp != NULL; temp = temp->ai_next)
    {
        sockFd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
        if (sockFd == -1)
        {
            perror("client: Unable to create socket");
            continue;
        }

        if (connect(sockFd, temp->ai_addr, temp->ai_addrlen) == -1)
        {
            close(sockFd);
            perror("client: Error in connecting");
            continue;
        }

        break;
    }

    if (temp == NULL)
    {
        fprintf(stderr, "client: Failed to connect\n");
        return -2;
    }

    inet_ntop(temp->ai_family, get_in_addr((struct sockaddr*)temp->ai_addr), s, sizeof s);
    printf("Establishing connecting with the server at %s\n", s);
    isConnected = true;

    freeaddrinfo(serverInfo);
    return 0;
}


int Client::sendData(void *msg, size_t len, int flags)
{
    int retVal;
    retVal = send(sockFd, msg, len, flags);
    return retVal;
}

bool Client::getConnectionStatus()
{
    return isConnected;
}

int Client::getSocketFd()
{
    return sockFd;
}

char * Client::getUserName()
{
    return userName;
}

char * Client::getURL()
{
    return URLToRetrieve;
}


int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: client <proxy IP> <proxy PORT> <URL to retrieve>\n");
        exit(1);
    }
    Client *c = new Client(argv[1], argv[2], argv[3]);

    int retVal = c->connectToHost();
    if (retVal < 0)
    {
        fprintf(stderr, "Connection failed\n");
        exit(1);
    }
    char query[1024];
    if (c->getConnectionStatus())
    {
        /*
         * Send HTTP request message to the proxy server
         */
        cout << "Connection successful..\n";
        strcpy(query, createHTMLRequest(c->getURL()));

        c->sendData(query, strlen(query)+1);

        // Get file size from the proxy server
        int numBytes;
        int fileSize = 0;
        int bytesReadSoFar = 0;
        if ( (numBytes = recv(c->getSocketFd(), &fileSize, sizeof(int), 0)) < 0)
        {
            perror("Error receiving file size");
            exit(1);
        }

        cout << "Receving the file from the proxy server.." << endl;
        // Get actual file data
        string fileName = getFileName(string(query));
        char str[100];
        if (fileName == "/")
            fileName = "index.html";
        else
        {
            std::size_t len = fileName.copy(str, fileName.length());
            str[len] = '\0';

            /*
             * Replace / by -
             */
            /*
            for (int i = 0; i < len; i++)
            {
                if (str[i] == '/')
                    str[i] = '-';
            }
            */
            fileName = string(str);
            unsigned found = fileName.find_last_of("/\\");
            fileName =  fileName.substr(found+1);
        }

        char incomingBuf[512];
        memset(incomingBuf, 0, 512);

        std::size_t pos1 = string(query).find("Host");
        std::size_t pos2 = string(query).find("com");
        string hostName = string(query).substr(pos1 + 6, pos2 - pos1 - 3);
        hostName = formatName(hostName);

        //string fName = hostName + fileName;
        string fName = fileName;

        std::fstream fileOut(fName.c_str(), std::fstream::out | std::fstream::app | std::fstream::binary);

        int fdMax;
        fdMax = c->getSocketFd();
        fd_set read_fds;
        int fd = c->getSocketFd();

        while(bytesReadSoFar != fileSize)
        {
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);

            if(select (fdMax +1, &read_fds, NULL, NULL, NULL) == -1)
            {
                perror("Select");
                exit(4);
            }

            if (FD_ISSET(fd, &read_fds))
            {
                numBytes = recv(fd, incomingBuf, 512, 0);
                bytesReadSoFar += numBytes;
                fileOut.write(incomingBuf, numBytes);
                if (bytesReadSoFar > fileSize)
                    break;
            }

        }
        cout << "File: " << fName << " received" << endl;
        fileOut.close();
    }

    return 0;
}
