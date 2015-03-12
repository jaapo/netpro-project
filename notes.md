Distributed peer-to-peer filesystem
===================================

Components: directory server, clients

Directory server
- Maintains directory of file metadata
	- filenames and sizes
	- list of clients that have the file data
- Keeps track of client's load
- Responds to client requests
- Messages from clients:
	- advertise a file ("I now have a copy of file doc.txt")
	- notify about removal ("I don't have file doc.txt anymore")
	- go offline ("I wont be available"/"Delete me from all file's client lists")
	- get file info by name ("Get metadata of file doc.txt")
	- find file peers ("Get list of clients storing doc.txt")
	- search files by name ("Get list of files with '.h' in their name")
- Details
	- server orders requested peer lists to balance load
	- b-tree of files indexed by name
	- index of files by client addresses

Client
- Stores files
- Downloads files from peers
- Upload files to peers when requested

- Messages from clients:
	- file requests ("Send me file doc.txt")
	- partial file request ("Send me bytes 100-200 of file doc.txt")
	- upload file ("Please store this file")
	- get file list ("What files do you have?")

- Communication with clients:
	- send requested data
	- receive data to store

- Messages from server:
	- status query ("Are you up? How much load do you have?")
	- file list update query ("What files do you store?")
