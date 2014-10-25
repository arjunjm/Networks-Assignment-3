#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <map>
#include <time.h>
#include <stdio.h>
#include <list>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <fcntl.h>

using namespace std;

#define MAXDATASIZE 100
#define STDIN 0
#define HTTP_PORT 80
#define MAX_CACHE_SIZE 10

struct sockaddr;
struct sockaddr_in;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

double getCurrentTime()
{
       time_t timer;
       struct tm reference;
       double seconds;
       
       reference.tm_hour = 0;
       reference.tm_min  = 0;
       reference.tm_sec  = 0;
       reference.tm_mon  = 0;
       reference.tm_mday = 1;
       reference.tm_year = 100;

       // Get current time
       time(&timer);

       // Calculate the difference in seconds
       seconds = difftime(timer, mktime(&reference));

       return seconds;
}

int getFileSize(const char* fileName)
{
    struct stat st;
    if (stat(fileName, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            fileName, strerror(errno));

    return -1;
}

void removeLRUEntry(std::map<string, double> reqTimeMap)
{
    std::map<string, double>::iterator it;
    std::map<string, double>::iterator leastRecentIter;
    leastRecentIter = reqTimeMap.begin();
    for (it = reqTimeMap.begin(); it != reqTimeMap.end(); it++)
    {
        if (it->second < leastRecentIter->second)
            leastRecentIter = it;
    }
    reqTimeMap.erase(leastRecentIter);
}

void deleteHeaderFromFile(string fileName)
{
    string tempFileName = fileName + "temp_file";
    std::fstream tempFile(tempFileName.c_str(), std::fstream::out | std::fstream::app | std::fstream::binary);
    std::fstream inFile(fileName.c_str());
    std::string line;
    /*
     * Ignore lines till the line that contains only carriage return
     */
    bool ignoreLines = true;
    bool firstLine = false;
    while (std::getline(inFile, line))
    {
        if (ignoreLines)
        {
            if (line == "\r")
            {
                firstLine = true;
                ignoreLines = false;
                continue;
            }

        }
        else
        {
            if (firstLine)
            {
                firstLine = false;
            }
            else
            {
                tempFile << endl;
            }
            tempFile << line;
        }
    }
    inFile.close();
    std::remove(fileName.c_str());
    std::rename(tempFileName.c_str(), fileName.c_str());
}

const char *createHTMLRequest(char *clientRequest)
{
    string cliReq(clientRequest);
    std::size_t pos1 = cliReq.find("www");

    bool slashesFound = false;
    if (pos1 == std::string::npos)
    {
        pos1 = cliReq.find("//");
        if (pos1 != std::string::npos)
            slashesFound = true;
        else
            pos1 = 0;
    }

    string subReq;
    if (!slashesFound)
    {
        subReq = cliReq.substr(pos1);
    }
    else
    {
        subReq = cliReq.substr(pos1 + 2);
    }
    std::size_t pos2 = subReq.find("/");
    string hostName = subReq.substr(0, pos2);
    string fileRequest;

    if(pos2 == std::string::npos)
    {
        fileRequest = "/";
    }
    else
    {
        fileRequest = subReq.substr(pos2);
    }

    string HTMLRequest = "GET " + fileRequest + " HTTP/1.0\r\n"
        "Host: " + hostName + "\r\n"
        "\r\n";

    return HTMLRequest.c_str();
}

string getFileName(string HTTPMessage)
{
    std::size_t pos = HTTPMessage.find("GET"); 
    const char *subReq = HTTPMessage.substr(pos + 4).c_str();
    
    int i = 0;
    while(subReq[i] != ' ')
    {
        i++;
    }
    string fileName = string(subReq).substr(0, i);
    return fileName;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char *getHostIP(const char *hostName)
{
    char *IP = new char[16];
    memset(IP, 0, 16);
    struct addrinfo hints;
    struct addrinfo *serverInfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port[10];
    sprintf(port, "%d", HTTP_PORT);

    int rVal = getaddrinfo(hostName, port, &hints, &serverInfo);
    if (rVal != 0)
    {
        //Non-zero return value indicates an error
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rVal));
        return NULL;
    }
    inet_ntop(serverInfo->ai_family, get_in_addr((struct sockaddr*)serverInfo->ai_addr), IP, 16);

    return IP;
}

string formatName(string name)
{
   for (std::size_t i = name.length() - 1; i > 0; i--)
   {
       if (!isalnum(name[i]) && name[i] != '.')
       {
           name.erase(i, 1);
       }
   }
   return name;
}


#endif /* __HELPER_H__ */
