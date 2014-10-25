ECEN602 HW1 Programming Assignment
----------------------------------

Team Number: 25
Member 1 # Moothedath, Arjun Jayaraj(UIN: 722008073)
Member 2 # Wilkins, Janessa (UIN: 724000155)
---------------------------------------

Description/Comments:
--------------------

* Testing was done on both localhost and the Zachry machines.

We have implemented the assignment in C++. There are separate classes for the server and the client, each with 
its own routines and private data members. There is a separate helper header file which contains the utility 
routines used by both the server and the client. More information on the same follow.

1. The Server class contains the proxy server routines and server-specific data members. The data members include the listening socket FD, 
   the proxy server IP, the proxy server port number,  a map that has a mapping from the socket file descriptor to the client concerned, a set 
   of server sockets to the remote web server, a map from the client socket to the file requested and another map from the HTTP request 
   to the timestamp at which the request was made. This final map is used for implementing the LRU cache.
   
   The Server constructor initializes the proxy server parameters which are passed from the command line (proxy server IP, proxy server port number). 
   Most of these routines are self-explanatory from their names. The multiplexing of the socket FDs (for listening for new connections, accepting 
   data from the remote web server) is done in the acceptConnection() routine which runs in an infinite loop. 

   LRU Implementation
   ==================
   The LRU cache has a maximum size of 10. (this variable can be set in the helper.h header file). When a HTTP request from a client 
   arrives, the proxy server checks if the same request is present in the map (requestTimeStampMap). If present, the proxy server knows that the file is present 
   in its cache and sends the file to the client without querying the remote web server. In the case when the HTTP request is new, the 
   proxy server has to query the remote web server and fetch the data. The proxy server then checks if the maximum cache size has been reached 
   or not. If yes, then the server removes the oldest entry from the cache by checking the timestamps of all the cache entities and then adds 
   the entry for the new request. If otherwise, the proxy server simply adds the request and the timestamp into the map.

   Notes regarding the LRU:
   =======================
   * The LRU map is not persistent across different proxy server runs, ie if you close the proxy server and run it again, the map is lost 
   from the memory and hence it has to be refilled even though the files may be present in the folder

   * We realize that the LRU implementation is not the most robust and efficient one out there. But we had to limit to an inefficient 
   implementation due to the lack of time and the number of entries being relatively low (hence computationally not intensive)

2. The Client class contains client data members such as the proxy server IP to which the client has to connect to, the server port number, the 
   socket file descriptor through which the client communicates with the server etc. The class also contains the associated routines such as connectToHost, 
   sendData etc whose functionalities can be deducted from their names. 


Unix command for starting server:
------------------------------------------
./server SERVER_IP SERVER_PORT S

For example, if you want to create a proxy server on the local host, the usage would be : 

                        ./server 127.0.0.1 3490

Unix command for starting client:
------------------------------------------
./client SERVER_IP SERVER_PORT <file to fetch>

For example, if you're running the server on the local host, the usage would be :

                        ./client 127.0.0.1 3490 www.google.com
