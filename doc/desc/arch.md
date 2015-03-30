<a id="arch"></a>
Architecture
------------

### Components

#### Directory server
Directory server is a centralized metadata server. It stores metadata of all files and file servers, and file system directory hierarchy. Directory server also manages locks.
File servers connect to directory server to query file information, acquire/and free locks, or notify about a file changes.

##### Data structures
Directory server's most important data structure is the file hierarchy. Files are identified by UNIX style paths and the server maintains a tree structure of the files. From this structure it can fast lookup file information by path.

An index stores all the file servers currently connected to the directory server. Data related to each file server can be found from this structure. This data contains locks and load balancing data.

#### File servers
File servers store file contents and related metadata. They serve client requests. File servers contact the directory to update it and to get metadata of requested files. Each server store only some files, usually files their clients use most. To access other files, it must ask directory which servers store the file and get the file from them. Downloaded files are stored until they are removed, updated by some other server's user, or server runs out of space.

##### Data structures
Files are stored in server's local file system. File index for fast access and metadata storing is kept in memory. List of connected clients and their connections are kept in memory.

#### Client
Clients can view, edit, create and remove files and directories in the file system. Clients communicate only with their file servers. When started, a client establishes a connection to the server it is configured to use. This connection is kept open and used for all communication until client is finished.

Client doesn't store any major data structures, most of the state is stored by its file server.

#### Simplified hierarchial architecture diagram

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
                                                   
Every client uses only one file server to access files at a time. All file servers use the centralized directory. File servers communicate with each other to exchange files.

#### Architecture diagram with interfaces
<center><img alt="System architecture diagram" src="arch.svg" width="75%"></center>

