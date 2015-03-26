<a id="protocols"></a>
Protocols
---------

There are two protocols. One for client-server communication and other for communication between file servers and the directory server.

### File access protocol

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

#### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **handshake** message. Server registers the client and sends a response.

Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, a separate connection is opened for the purpose. Server's control message includes a request to open this new connection. Client should then accept servers connection to port xxxxx2 and receive data described by the control message.

When client exits, it closes the connection by sending a **quit** message to the server. Server acknowledges this.

#### Message format

All messages have a common message format. Messages have headers and sections. Message header contains *message_id*, *client id*, *server id*, *file system id*, *message type* and *next section* fields.

All identifiers are 64 bits long. Command number takes 16 bits. *next section* field value is 16 bits.

Every section ends with a the *next section* field. Rest of section format depends on the section type. A message may contain 0 to 127 sections.

    +----------------------------------------------------------------+
    |                      Message ID                                |
    +----------------------------------------------------------------+
    |                      Server ID                                 |
    +----------------------------------------------------------------+
    |                      Client ID                                 |
    +----------------------------------------------------------------+
    |                      File system ID                            |
    +----------------------------------------------------------------+
    | Message type   | Next section   | Section data ....            |
    +----------------------------------------------------------------+
    | ....                                           | Next section  |

##### Message type

Message type determines rest of the message contents. Possible message types are *handshake*, *status*, *quit*, *command* and *response*. Their codes are 1, 2, 3, 4 and 5 respectively.

##### Sections

Valid sections are message specific. Section types are *integer*, *string* and *binary*.

*integer* section is always 4-bytes long and it contains only one 4-byte signed integer. Integer is in big-endian byte order and the leftmost bit is sign bit.

*string* section has two fields: *string length* and *data*. *string length* is a 32-bit unsigned integer and *data* is an arbitrary length ASCII data field. *data* must be *string length* bytes long.

*binary* section is same as *string* but it may contain non-ASCII characters.

##### handshake

Two string sections: hostname, username.

##### command

Every command message contains a *command number* section. It is an integer section with command number.
Messages contain additional sections depending on command.

###### create
Command number is 1.
Message contains a string section which contains full path of file to create.

###### open
Command number is 2.
Message contains a string section with full path of file.

###### read
Command number is 3.
Message contains a string section with file path, an integer section with offset to start reading from and another integer section with byte count to read. If the bytecount is 0 whole file is read.

###### write
Command number is 4.
Message contains a string section with file path, an integer section with byte count of write and a binary section with data to write.

###### delete
Command number is
Message contains a string section 

###### copy
Command number is
Message contains a string section 

###### find
Command number is
Message contains a string section 



| command name | number | argument | description |
| ------------ | ------ | -------- | ----------- |
| **create**   | 1      | full path of file | creates an empty file to the file system with filename |
| **open**     | 2      | full path of file | opens a file in the file system |
| **read**     | 3      | full path of file | retrieves file contents |
| **write**    | 3      | full path of file | 

##### quit

Message contains no payload data.

##### status

Message contains no payload data.

##### response



### Directory control protocol

Directory control protocol DCP uses TCP port xxxxx3. File servers make requests to the Directory server using this protocol. It is similar with FAP, but has it's own messages.

### File content transfer protocol

Content transfer protocol FCTP uses TCP port xxxxx4 and is used in File server to File server communication. It is similar with FAP, but the messages are different.
