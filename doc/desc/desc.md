<a id="desc"></a>
Task description
----------------

Distributed file system stores files and enables access to them. Files are organized in hierarchial directory structure and they are identified by Unix style paths.

User can create, read, edit and remove files using an user interface. Files are saved on file servers in distributed manner, but are distributed mainly on-demand - that is servers store only files that their clients use. A centralized directory server keeps track of the files and servers. Clients communicate their requests to file server.

### Operations

#### File access

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

#### Load balancing

File servers send information about their status and load to directory server periodically. Directory server may also explicitly request a status update if a file server hasn't contacted it for a while.

Directory server detects servers with heavy load and instructs them to instruct their clients to use other servers if possible.

### Communication

#### File server --- Client

- **handshake**
	- on client start
	- client registers with its hostname and username
	- server introduces itself
	- server initializes all necessary data structures
- **client commands**
	- client sends using the same connection
	- same connection also for response
<!--- **change server**
	- server tells client to use other server
	- directory server's load balancing decision-->
- **quit**
<!--- **status query**
	- both can send
	- keepalive messages
	- only when connection is otherwise idle-->

#### File server *A* --- File server *B*

- **request file**
	- *A* needs to serve file to a client
	- downloads a copy from *B*

#### File server --- Directory server

- **handshake**
- **acquire a lock**
	- file server's client opens a file
	- file server needs a lock
	- directory may S-lock or X-lock the file
- **CRUD** operations (create, read, update, delete)
<!--- **status information**
	- file server sends information about it's load
- **client redirect command**
	- directory server load balance instruction
	- tells file server to tell it's clients to use another server-->

