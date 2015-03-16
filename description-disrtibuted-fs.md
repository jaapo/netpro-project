## Netowork Programming Project 2015
## Aaro Lehikoinen

Distributed file server
=======================

A distributed file server consists of a directory server, multiple file servers and clients. Directory server stores file metadata. File servers store files. Clients read and write files on servers.

Components
----------

### Directory server
Directory server stores metadata of files and file servers and file system directory hierarchy. Directory server also manages locks. It keeps track of all file servers, the files they store and their load.

### File servers
File servers store file contents and related metadata. They serve client requests. File servers contact the directory to update it and to get metadata of requested files. Each server store only some files. To access other files, it must ask directory which servers store the file and get the file from them.

### Client
Clients can view, edit, create and remove files and directories in the file system. Clients communicate only with their file servers.

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
- File editing (open file for editing)
- File deletion (`rm`)
- File copying (`cp`)
- File search (`find`)

Command execution include following steps

1. Client issues the command to the file server it's connected to
2. File server checks the user session and interprets the command
3. File server acquires required locks from the directory server
4. File server executes the command
	- More communication with client if necessary (eg. data transfer)
	- Notify directory server about changes
5. File server releases locks
6. File server communicates the result to client
7. Done

