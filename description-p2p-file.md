##Network Programming Project
##Task Description

Peer-to-Peer file-sharing system
================================

System stores files in distrubuted manner and lets clients upload and download them from each other. One or more centralized servers are used to store file metadata.

Components:
	- directory servers (one or many)
	- file clients

### Metadata servers

Servers main task is to keep directory of files and clients. They store metadata of files and keep track where files are stored. Server knows client's current status, but doesn't store information of inactive/offline clients. Servers mainly reply to clients' queries, but also tries to take care of some load balancing between clients. Connections to clients are not kept open for long.

All stored data is volatile. If a server is stopped, it must reacquire all data from clients. Data structures to access file data and client data fast are needed. Clients are indexed by host names. Files are indexed by names.

File metadata
- name (used as identifier)
- size
- type
- checksum

Server operations
- add file
- remove file
- refresh client status information
- retrieve metadata
- find peers storing a file
- find files

Communication with clients
- queries to clients
	- status and load query
	- file list update query
- messages from clients
	- file advertisement
	- removal notification
	- file metadata query
	- file client list query
	- file search query
	- 

### Clients

Clients can download files, store them, add them to directories and upload them to peers. Only files and statistics are stored in persistent storage. Clients may be configured to contact multiple directory servers. Client informs server about its files and current load. Files can be uploaded to peers to balance load. User running a client can search and download files using commandline. Other operations are automatic.

Clients P2P communication
- messages
	- file requests
	- partial file request
	- upload requests
	- file list requests
- responses to messages
	- stream file content data
	- send requested data


