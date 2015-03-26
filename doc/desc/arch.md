<a id="arch"></a>
Architecture
------------

### Components

#### Directory server
Directory server stores metadata of all files and file servers and file system directory hierarchy. Directory server also manages locks. It keeps track of all file servers, the files they store and their load.

#### File servers
File servers store file contents and related metadata. They serve client requests. File servers contact the directory to update it and to get metadata of requested files. Each server store only some files, usually files their clients use most. To access other files, it must ask directory which servers store the file and get the file from them.

#### Client
Clients can view, edit, create and remove files and directories in the file system. Clients communicate only with their file servers. When started, a client establishes a connection to the server it is configured to use. This connection is kept open and used for all communication until client is finished.
Simplified hierarchial architecture

#### Simplified architecture diagram

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

