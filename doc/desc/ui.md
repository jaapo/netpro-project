<a id="ui"></a>
User Interface
--------------

### Client
Client's user interface processes user commands synchronously. Commands are entered in a command prompt. Most commands print some output to standard out. Error messages are printed to standard error. Paths are UNIX style. Relative paths are allowed.

User interface commands are

- `cd <direcotry path>`

	Change working directory

- `pwd`

	Print current working directory

- `ls [-l] <path>...`

	List file details or directory contents. Use `-l` to print more details.

- `mkdir [-p] <directory>`

	Creates new directory to specified path. If option `-p` is used, `mkdir` creates the directory and all parents if they do not exist.

- `cat <file path>`

	Print file contents

- `edit <file path>`

	Open text editor to edit a file. File will be saved when editor is closed.

- `rm [-r] <path>...` 

	Remove a files. `-r` to remove directory and its contents recursively.

- `rmdir <path>`

	Remove an empty directory

- `mkdir <path>`

	Create a directory.

- `cp [-r] <file>... <destination>`

	Copies file(s) to destination. Use `-r` to copy directory and contents recursively.

- `find [<directory>] <search term>`

	Search `<directory>` or working directory for file matching search term. Search term may contain `*` as wildcard.

- `quit`, Ctrl+D
	
	Close client.

### File server
File server runs as a daemon and should be controlled using scripts and signals.

Command line arguments:
- `-conf <configuration file>` - path to configuration file, optional

### Directory server
Runs as a daemon and should be controlled using scripts and signals.

Command line arguments:
- `-conf <configuration file>` - path to configuration file, optional
