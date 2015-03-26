<a id="ui"></a>
User Interface
--------------

### Client
Client's user interface processes user commands synchronously. Commands are entered in a command prompt. Most commands print some output to standard out. Error messages are printed to standard error.

User interface commands are

- `mkdir [-p] <directory>`

	Creates new directory to specified path. If option `-p` is used, `mkdir` creates the directory and all parents if they do not exist.

- `-...`

### File server
File server runs as a daemon and should be controlled using scripts.

### Directory server
Runs as a daemon and should be controlled using scripts.

