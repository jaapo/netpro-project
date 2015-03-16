# Netowork Programming Project

Distributed file server
=======================

A distributed file server consists of a directory server, multiple file servers and clients. Directory server stores file metadata. File servers store files. Clients read and write files on servers.

Components
----------

### Directory server
Directory server stores metadata of files and file servers and file system directory hierarchy. Directory server also manages locks. It keeps track of all file servers, the files they store and their load.

### File servers
File servers store file contents and related metadata. They serve client requests. File servers contact the directory to update it and to get metadata of requested files.

### Client
Clients can view, edit, create and remove files and directories in the file system. Clients communicate with their file servers.

Operations
----------

### Load balancing

File servers send information about their status and load to directory server periodically. Directory server may also explicitly request a status update if a file server hasn't contacted it for a while.

Directory server detects servers with heavy load and instructs them to instruct their clients to use other servers if possible.

### File access

Clients access files using file servers.

Commands include

- Directory creation
- Directory deletion
- Directory content listing
- File creation
- File editing (open file for editing)
- File deletion
- File copying
- File search

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

