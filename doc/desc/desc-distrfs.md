### Netowork Programming Project 2015
### Aaro Lehikoinen

Distributed file server
=======================

A distributed file server consists of a directory server, multiple file servers and clients. Directory server stores file metadata. File servers store files. Clients read and write files on servers.

Components
----------

### Directory server
Directory server stores metadata of all files and file servers and file system directory hierarchy. Directory server also manages locks. It keeps track of all file servers, the files they store and their load.

### File servers
File servers store file contents and related metadata. They serve client requests. File servers contact the directory to update it and to get metadata of requested files. Each server store only some files, usually files their clients use most. To access other files, it must ask directory which servers store the file and get the file from them.

### Client
Clients can view, edit, create and remove files and directories in the file system. Clients communicate only with their file servers. When started, a client establishes a connection to the server it is configured to use. This connection is kept open and used for all communication until client is finished.

Architecture
------------

Simplified hierarchial architecture
                                                    
							 +-----------+                          
							 | Directory |                          
							 | Server D  |                         
							 +-----------+                          
							 /            \                           
							/              \                          
						 /                \                         
		+---------------+          +---------------+                       
		| File Server A | <------> | File Server B |                     
		+---------------+          +---------------+ 
					/        \                 /       \
		+----------+ +----------+  +----------+ +----------+                   
		| Client 1 | | Client 2 |  | Client 3 | | Client 4 |
		+----------+ +----------+  +----------+ +----------+                           
                                                   
Every client uses only one file server to access files at a time. All file servers use the centralized directory. File servers communicate with each other to excange files.

<center><img alt="System architecture diagram" src="arch.svg" width="75%"></center>

Operations
----------

### Load balancing

File servers send information about their status and load to directory server periodically. Directory server may also explicitly request a status update if a file server hasn't contacted it for a while.

Directory server detects servers with heavy load and instructs them to instruct their clients to use other servers if possible.

### File access

Clients access files using file servers. Access is transparent. If client's file server doesn't have the file, server will find it and the client will be able to use it.

Commands include

- Directory creation (`mkdir`)
- Directory deletion (`rmdir`, `rm -r`)
- Directory content listing (`ls`)
- File creation (using editor, `touch`)
- File editing (open file for editing, close and save when finished)
- File deletion (`rm`)
- File copying (`cp`)
- File search (`find`)

Command execution steps:

1. Client issues the command to the file server it's connected to
2. File server checks the user session and interprets the command
3. File server acquires required locks from the directory server
4. File server executes the command
	- More communication with client if necessary (eg. data transfer)
	- Notify directory server about changes
5. File server releases locks
6. File server communicates the result to client
7. Done

Communication
-------------

### File server --- Client

- **handshake**
	- on client start
	- client registers with its hostname and username
	- server introduces itself
	- server initializes all necessary data structures
- **client commands**
	- client sends using the same connection
	- same connection also for response
- **change server**
	- server tells client to use other server
	- directory server's load balancing decision
- **quit**
	- client quits
	- server releases all locks and data structures
- **status query**
	- both can send
	- keepalive messages
	- only when connection is otherwise idle

### File server *A* --- File server *B*

- **request file**
	- *A* needs to serve file to a client
	- downloads a copy from *B*

### File server --- Directory server

- **handshake**
- **acquire a lock**
	- file server's client opens a file
	- file server needs a lock
	- directory may S-lock or X-lock the file
- **status information**
	- file server sends information about it's load
- **client redirect command**
	- directory server load balance instruction
	- tells file server to tell it's clients to use another server

