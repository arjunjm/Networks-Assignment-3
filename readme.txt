ECEN602 HW1 Programming Assignment
----------------------------------

Team Number: 25
Member 1 # Moothedath, Arjun Jayaraj(UIN: 722008073)
Member 2 # Wilkins, Janessa (UIN: 724000155)
---------------------------------------

Description/Comments:
--------------------

Notes:
======

* We have implemented the bonus part feature-1 (ACK, NACK, ONLINE, OFFLINE).

* The testing was done only with our own server and client executables. But we did test across multiple systems 
  and not just on the locahost.

We have implemented the assignment in C++. The initial phase involved porting the existing C code into C++. 
There are separate classes for the server and the client, each with its own routines and private data members. 
There is a separate helper header file which contains the structure definitions and some common routines for 
packet creation. More information on the same follow.

1. The helper header file contains all of the structure definitions used through out the code, the most important 
   of which being the SBMPHeader structure. This structure takes the following form, 

    typedef struct SBMPHeader
    {
        unsigned int version : 9;
        unsigned int type : 7;
        unsigned short length;
        SBMPAttributeT attributes[2];
    } __attribute__((packed)) SBMPHeaderT;

    The SBMPAttribute structure contains the actual payload and the type of the message that the packet carries. It 
    is shown below.

    typedef struct SBMPAttribute
    {
        AttributeTypeT type;
        unsigned short length;
        Payload payload;
    } __attribute__((packed)) SBMPAttributeT;

    The AttributeType here informs us about the nature of content of the attribute, which could vary from actual message 
    that the client could be sending, the client user name or a failure message from the server. The Payload is a union 
    which could be one of the message types (username, message, failure reason etc)

    The structures were packed so as to avoid padding bytes. This was based on an answer in stackoverflow which seemed to 
    imply that this is the way to go when sending structs using sockets. (Not sure if this is entirely necessary though!)

    The helper function also contains the createMessagePacket function.

        SBMPHeaderT* createMessagePacket(SBMPMessageTypeT msgType, const char *userName, const char *msg)

        The function returns an SBMP header pointer of suitable type. The user name and message attributes are not required 
        for all header types.

2. The Server class contains the routines and server-specific data members. The data members include the listening socket FD, 
   the server IP, the server port number, the max clients that the server can accommodate, a map that has a mapping from the 
   socket file descriptor to the client concerned etc. The routines perform the normal socket operations such as socket creation 
   and binding, listen for connections, accepting incoming connections etc. 
   
   The Server constructor initializes the server parameters which are passed from the command line (server IP, server port number, 
   max number of clients). Most of these routines are self-explanatory from their names. The multiplexing of the socket FDs (for 
   listening for new connections and accepting data from existing connections) is done in the acceptConnection() routine which runs 
   in an infinite loop. In order to restrict the max number of clients, we check if the fdUserMap size has exceeded the maxClients parameter. 
   This map is also used to retrieve the currently online users. Another map called userStatusMap is used to verify that no 2 clients 
   have the same user name.

   The sequence of operations is as per the requirement and we feel that there isn't much to add here.

3. The Client class contains client data members such as the server IP to which the client has to connect to, the client user name, the 
   server port number, the socket file descriptor through which the client communicates with the server etc. The class also contains the 
   associated routines such as connectToHost, sendData, recvData etc whose functionalities can be deducted from their names. The only 
   tricky part in the client's implementation was to multiplex the server messages and the input prompt from STDIN. This was implemented 
   similar to how the server listens to multiple sockets using select() system call. This is in the client's main().


Unix command for starting server:
------------------------------------------
./server SERVER_IP SERVER_PORT MAX_CLIENTS

For example, if you want to create a server on the local host, the usage would be : 

                        ./server 127.0.0.1 3490 5

Unix command for starting client:
------------------------------------------
./client USERNAME SERVER_IP SERVER_PORT

For example, if you're running the server on the local host, the usage would be :

                        ./client Arjun 127.0.0.1 3490
