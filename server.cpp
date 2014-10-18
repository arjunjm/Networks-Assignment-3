/*
 * Implementation of the Server Class
 */

#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

Server::Server(char *servIP, char *portNum, int maxConns)
{
    strcpy(this->serverIP, servIP);
    strcpy(this->portNum, portNum);
    this->maxConnections = maxConns;

    memset(&this->hints, 0, sizeof hints);
    this->hints.ai_family = AF_UNSPEC;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_flags = 0; 
}

int Server::createSocketAndBind()
{
    struct addrinfo *serverInfo, *temp;
    int yes = 1;
    int rv;
    if ((rv = getaddrinfo(this->serverIP, this->portNum, &this->hints, &serverInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    /*
     * Loop through the results and bind to the first we can
     */
    for(temp = serverInfo; temp != NULL; temp = temp->ai_next)
    {
        sockFd = socket(temp->ai_family, temp->ai_socktype,
                temp->ai_protocol);
        if (sockFd == -1)
        {
            perror("Unable to create socket. Attempting again..");
            continue;
        }

        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, 
                    sizeof(int)) == -1)
        {
            perror("setsockopt failed");
            exit(1);
        }

        if (bind(sockFd, temp->ai_addr, temp->ai_addrlen) == -1)
        {
            close(sockFd);
            perror("server: Unable to bind");
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if (temp == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    else
    {
        // success!
        return 0;
    }
}

int Server::listenForConnections()
{
    if(listen(sockFd, maxConnections) == -1)
    {
        perror("Error while listening");
        exit(1);
    }
    return 0;
}

int Server::sendData(int sockFD, void * buf, size_t len, int flags)
{
    int retVal;
    retVal = send(sockFD, buf, len, flags);
    return retVal;
}

/*
void printMap(std::map<int, string> myMap)
{
    std::map<int, string>::iterator it;
    for (it = myMap.begin(); it != myMap.end(); it++)
    {
        cout << it->first << " : " << it->second << endl;   
    }
}
*/
int Server::recvData(int sockFD, SBMPMessageType &msgType, char *message)
{
    int numBytes;
    SBMPHeaderT *recvHeader = new SBMPHeaderT();
    numBytes = recv(sockFD, recvHeader, sizeof(SBMPHeaderT)-1, 0);
    if (numBytes == -1)
    {
        perror("Error in receiving data from the client");
        exit(1);
    }

    msgType = (SBMPMessageTypeT) recvHeader->type;

    switch(msgType) 
    {
        case JOIN:
            {
                if (userStatusMap.size() >= maxConnections)
                {
                    std::string connFailReason("Maximum number of connections reached");
                    cout << connFailReason << endl;
                    SBMPHeaderT *sbmpHeader = createMessagePacket(NACK, NULL, connFailReason.c_str());
                    if (sendData(sockFD, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                    {
                        perror("Error while sending NACK");
                    }
                    return 0;
                }

                string userName(recvHeader->attributes[0].payload.username);
                if (userStatusMap.find(userName) == userStatusMap.end())
                {
                    cout << userName << " has joined the chat session" << endl;
                    userStatusMap[userName] = ONLINE;
                    fdUserMap[sockFD] = userName;
                }
                else
                {
                    std::string connFailReason("Username already in use");
                    cout <<  "The user has already connected\n";
                    SBMPHeaderT *sbmpHeader = createMessagePacket(NACK, NULL, connFailReason.c_str());
                    if (sendData(sockFD, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                    {
                        perror("Error while sending NACK");
                    }
                    return 0;
                }

                /*
                 * Send ACK with currently online user information
                 */
                std::string userInfo = getUserInfo();
                SBMPHeaderT *sbmpHeader = createMessagePacket(ACK, NULL, userInfo.c_str());
                if (sendData(sockFD, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                {
                    perror("Error while sending ACK");
                }
            }
            break;

        case FWD:
            break;

        case SEND:
            {
                strcpy(message, recvHeader->attributes[0].payload.message);
                cout << "Received message from " << fdUserMap[sockFD] << ": [" << message << "]. Forwarding message to other clients" << endl;
            }
            break;

        case ACK:
        case NACK:
        case ONLINE_INFO:
        case OFFLINE_INFO:
            break;

    }
    
    return numBytes;
}

int Server::acceptConnection()
{
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;
    char ipAddr[INET6_ADDRSTRLEN]; 
    fd_set master, read_fds;
    int fdMax;
    struct timeval tv;
    tv.tv_sec = 50;
    tv.tv_usec = 500000;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockFd, &master);
    fdMax = sockFd;

    while(1)
    {
       read_fds = master;
       if (select(fdMax + 1, &read_fds, NULL, NULL, &tv) == -1)
       {
           perror("Select");
           exit(4);
       }

       /*
        * Run through existing connections and check if a client is ready
        */
       for (int i = 0; i <= fdMax; i++)
       {
           if (FD_ISSET(i, &read_fds))
           {
               if (i == sockFd)
               /*
                * This is the listening socket. 
                * Check new incoming connections.
                */
               {
                   sin_size = sizeof clientAddr;
                   newConnFd = accept(i, (struct sockaddr *)&clientAddr, &sin_size);
                   if (newConnFd == -1)
                   {
                       perror("Error while accepting connection..");
                   }
                   else
                   {
                       FD_SET(newConnFd, &master);

                       if (newConnFd > fdMax)
                       {
                           fdMax = newConnFd;
                       }
                       inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);


                   }

               }
               else
               {
                   /*
                    * Handle data from client connection.
                    */
                   int readBytes;
                   //SBMPMessageType msgType;
                   char *message = new char[512];
                   if ((readBytes = recv(i, message, 512, 0)) <= 0)
                   {
                        /*
                         * This means either there was a error in receiving data
                         * or that the client has closed the connection.
                         */
                        if (readBytes == 0)
                        {
                            if (fdUserMap[i] != "")
                            {
                                string leftMsg = fdUserMap[i] + " has left the chat session.\n";
                                cout << leftMsg;
                                cout << "Connection closed\n";
                                
                            }
                            // Close the connection
                            FD_CLR(i, &master);
                            userStatusMap.erase(fdUserMap[i]);
                            fdUserMap.erase(i);
                            close(i);
                        }
                        else
                        {
                            perror("receive");
                        }
                   }
                   else
                   {
                       /*
                        * Client is sending actual data.
                        */
                       cout << message << endl; 
                       string msg(message);
                       std::size_t pos1 = msg.find("Host");
                       std::size_t pos2 = msg.find("com");
                       string hostName = msg.substr(pos1 + 6, pos2 - pos1 - 3);

                       char *hostIP = getHostIP(hostName.c_str());

                       /*
                        * Fill struct for sending to remote server
                        */
                       struct sockaddr_in *remoteServer = new sockaddr_in();
                       remoteServer->sin_family = AF_INET;
                       int result = inet_pton(remoteServer->sin_family, hostIP, (void *)(&(remoteServer->sin_addr.s_addr)));
                       if (result <= 0)
                       {
                           perror("IP conversion to binary failed");
                           continue;
                       }

                       remoteServer->sin_port = htons(HTTP_PORT);

                       /*
                        * Create socket for communicating with the remote HTTP server
                        */
                       int httpSock = socket(AF_INET, SOCK_STREAM, 0);
                       if (httpSock < 0)
                       {
                           perror("socket creation failed");
                           continue;
                       }

                       if (connect(httpSock, (struct sockaddr *)remoteServer, sizeof(struct sockaddr)) < 0)
                       {
                           perror("connect error");
                           continue;
                       }

                       /*
                        * Send HTTP request
                        */
                       int bytesSent = send(httpSock, message, strlen(message) + 1, 0);
                       if (bytesSent < 0)
                       {
                           perror("Sending failed");
                           continue;
                       }

                       char incomingBuf[512];
                       std::ofstream fileOut(hostName.c_str(), ios::out);
                       int numBytes = 0;
                       do
                       {
                           numBytes = recv(httpSock, incomingBuf, 511, 0);
                           fileOut << incomingBuf;
                       }while (numBytes > 0);
                       fileOut.close();

                       delete [] message;
                        
                   }
               }
           }
       }
    }
    return 0;
}

std::string Server::getUserInfo()
{
    int count = fdUserMap.size();

    std::stringstream countStr;
    countStr << count;
    string clientCountStr = countStr.str(); 

    std::string userInfo;
    userInfo.append(clientCountStr);
    
    std::map<int, string>::iterator it;
    
    for (it = fdUserMap.begin(); it != fdUserMap.end(); it++)
    {
        userInfo.append(" ");
        userInfo.append(it->second);
    }
    return userInfo;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
       fprintf(stderr, "Usage: server <server IP> <server PORT> <max clients>\n");
       exit(1);
    }

    Server *s = new Server(argv[1], argv[2], atoi(argv[3]));
    s->createSocketAndBind();
    s->listenForConnections();

    printf("Chat server is waiting for incoming connections...\n");
    s->acceptConnection();
    return 0;
}
