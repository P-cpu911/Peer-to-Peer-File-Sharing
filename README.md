### P2P File Transfer Introduction

There are 2 programs: peer.c and index_server.c. Assume you named the executable file as "peer" and "iserver", respectively. Upon running, the syntax should be:

```bash
./peer [Index server IP] [Index server's Port] [Peer server's Port]
```
and
```bash
./iserver
```

Well, Index server's IP and Port is self-explanatory, and for [Peer server's Port], just pick an unused port for this program to bind to.
Now, in the source code, index_server is set to run on port **8784**, but well, that can be changed by changing the macro in the source code file.

---

For peer.c, there are 4 functions:

`SEED [file name]`  
`SEARCH [file name]`  
`GET [file name]`  
`exit`  

*All of these commands are actually case-insensitive, actually* 

**SEED**: For registering the program to the index server  
**SEARCH**: For searching peers that is holding the file  
**GET**: For downloading the file  
**exit**: To exit the peer program    

For **GET** specifically, after running it, the program will query for IP address and port:  
"Enter Peer IP and Port from SEARCH result:"  
Enter IP and Port in the exact format as **SEARCH** returned, which is "[IP] [Port]".  

---

For index_server.c:
Just let it run there. And if you want to shut it off, just do the usual SIGKILL with Ctrl+C
       
