#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <list>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>

using namespace std;

#define MAXDATASIZE 100
#define STDIN 0
#define HTTP_PORT 80

struct sockaddr;
struct sockaddr_in;

typedef enum UserStatus
{
    ONLINE,
    OFFLINE,
    IDLE
} UserStatusT;

typedef enum AttributeType
{
   ATTR_REASON,
   ATTR_USER,
   ATTR_CLI_CNT,
   ATTR_MSG
} AttributeTypeT;

typedef enum SBMPMessageType
{
    JOIN = 2,
    FWD  = 3,
    SEND = 4,
    ACK = 5,
    NACK = 6,
    ONLINE_INFO = 7,
    OFFLINE_INFO = 8
} SBMPMessageTypeT;

typedef union
{
    char username[16];
    char message[512];
    char reason[32];
    unsigned short clientCount;
}Payload;

typedef struct SBMPAttribute
{
    AttributeTypeT type;
    unsigned short length;
    Payload payload;
} __attribute__((packed)) SBMPAttributeT;

typedef struct SBMPHeader
{
    unsigned int version : 9;
    unsigned int type : 7;
    unsigned short length;
    SBMPAttributeT attributes[2];
} __attribute__((packed)) SBMPHeaderT;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
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
    struct hostent *he;
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
SBMPHeaderT* createMessagePacket(SBMPMessageTypeT msgType, const char *userName, const char *msg)
{
    SBMPAttributeT sbmpAttr;
    SBMPHeaderT *sbmpHeader;

    switch(msgType)
    {
        case JOIN:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_USER;
            strcpy(sbmpAttr.payload.username, userName);
            sbmpAttr.length = strlen(sbmpAttr.payload.username) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)JOIN;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;
        
        case FWD:

            /*
             * Fill in the SBMP Attribute struct
             */   

            SBMPAttributeT sbmpUserAttr;
            sbmpUserAttr.type = ATTR_USER;
            strcpy(sbmpUserAttr.payload.username, userName);
            sbmpUserAttr.length = strlen(sbmpUserAttr.payload.username) + 4;

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)FWD;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpUserAttr;
            sbmpHeader->attributes[1] = sbmpAttr;

            break;

        case SEND:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)SEND;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        case ACK:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)ACK;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        case NACK:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_REASON;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)NACK;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;


            break;

        case OFFLINE_INFO:
        case ONLINE_INFO:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)msgType;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        default:
            printf("Error!!!Did not match any known message types. Exiting");
            exit(1);
    }
    return sbmpHeader;
}


#endif /* __ELPER_H__ */
