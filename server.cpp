/*
 * Implementation of the Server Class
 */

#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include<fcntl.h>

using namespace std;

Server::Server(char *servIP, char *portNum)
{
    strcpy(this->serverIP, servIP);
    strcpy(this->portNum, portNum);

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
               else if (serverSockets.find(i) != serverSockets.end())
               {
                   /* 
                    * Receive data from a server socket
                    */
                   char incomingBuf[512];
                   memset(incomingBuf, 0, 512);
                   int readBytes = recv(i, incomingBuf, 512, 0);
                   std::fstream fileOut(sockFileMap[i].c_str(), std::fstream::out | std::fstream::app | std::fstream::binary);
                   if (readBytes == 0)
                   {
                       /*
                        * Server has finished sending data. Terminate.
                        * Also, add code to send data to client here.
                        */
                       serverSockets.erase(i);
                       FD_CLR(i, &master);
                       deleteHeaderFromFile(sockFileMap[i]);
                       fileOut.close();
                       sockFileMap.erase(i);
                       close(i);
                   }
                   else
                   {
                       //fileOut << incomingBuf;
                       fileOut.write(incomingBuf, readBytes);
                       fileOut.close();
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
                       string msg(message);
                       cout << message << endl; 
                       string fileName = getFileName(msg);
                       char str[100];
                       if (fileName == "/")
                           fileName = "-index.html";
                       else
                       {
                           std::size_t len = fileName.copy(str, fileName.length());
                           str[len] = '\0';

                           /*
                            * Replace / by -
                            */
                           for (int i = 0; i < len; i++)
                           {
                               if (str[i] == '/')
                                   str[i] = '-';
                           }
                           fileName = string(str);
                       }

                       std::size_t pos1 = msg.find("Host");
                       std::size_t pos2 = msg.find("com");
                       string hostName = msg.substr(pos1 + 6, pos2 - pos1 - 3);
                       hostName = formatName(hostName);

                       string fName = hostName + fileName;
                       char *hostIP = getHostIP(hostName.c_str());
                       if (hostIP == NULL)
                           continue;

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

                       serverSockets.insert(httpSock);
                       FD_SET(httpSock, &master);
                       if (httpSock > fdMax)
                       {
                           fdMax = httpSock;
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
                       std::fstream fileOut(fName.c_str(), ios::out);
                       sockFileMap[httpSock] = fName;
                       sockReadyForActualData[httpSock] = false;
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
    if (argc != 3)
    {
       fprintf(stderr, "Usage: server <IP to bind> <PORT to bind>\n");
       exit(1);
    }

    Server *s = new Server(argv[1], argv[2]);
    s->createSocketAndBind();
    s->listenForConnections();

    printf("Proxy server is waiting for incoming connections...\n");
    s->acceptConnection();
    return 0;
}
