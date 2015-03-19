Protocols
=========

There are two protocols. One for client-server communication and other for communication between file servers and the directory server.

File access protocol
--------------------

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **handshake** message. Server registers the client and sends a response.

Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, a separate connection is opened for the purpose. Server's control message includes a request to open this new connection. Client should then accept servers connection to port xxxxx2 and receive data described by the control message.

When client exits, it closes the connection by sending a control message to the server. Server acknowledges this.

### Message format

All messages have a common message format. Messages have headers and sections. Message header contains *message_id*, *client id*, *server id*, *file system id*, *message type* and *next section* fields.

All identifiers are 64 bits long. Command number takes 16 bits. *next section* field value is 16 bits.

Every section ends with a the *next section* field. Rest of section format depends on the section type. A message may contain 1 to 127 sections.

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

#### Message type

Message type determines rest of the message contents. Possible message types are *handshake*, *status*, *quit*, *command* and *response*. Their codes are 1, 2, 3, 4 and 5 respectively.

#### Sections

Valid section types are message specific.

##### status

##### handshake


##### quit
##### command

Section types are *command number* and *arguments*.

###### command number

*command number* section is an 8-bit section with command number.

###### argument

This section contains two fields: *argument length* and *argument data*.

*argument length* tells how many bytes of data does the argument contain. *argument length* may be 0. *argument data* is the variable length payload.

##### response

Directory control protocol
--------------------------
