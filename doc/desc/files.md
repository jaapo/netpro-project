<a id="files"></a>
Files
-----

### Client
Client has a configuration file.

### File server
File server's state is mainly volatile. File server uses a configuration file, a list of file server's files and a directory in local file system to store the actual files. Also a log file exists.

#### Configuration
File server uses a single configuration file called `fileserv.conf`. This file contains one option per line. Line syntax is `option_name value`. Option name is separated with any amount of space ('` `') or tab charactes (\t). Empty lines and lines starting with `#` are ignored.

Valid options are
- `directory_server` - IP-address or name of the server
- `data_location` - path to server's data directory in local file system
- ...

#### Log

### Directory server files
Directory server has a single configuration file. File system's directory is stored in a file. A single log file is used.

