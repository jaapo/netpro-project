<a id="files"></a>
Files
-----

### Configuration file format
All components use the same configuration file syntax. This file contains one option per line. Line syntax is `option_name value`. Option name is separated with any amount of space ('` `') or tab charactes (\t). Empty lines and lines starting with `#` are ignored.

### Client
Client has a configuration file `/etc/dfs_client.conf`. Alternative file name can be given as a command line parameter.

Configuration parameters are
	- `file_server` - IP-address or name of client's file server
	- `log_file` - path to the client log file

### File server
File server's state is mainly volatile. File server uses a configuration file, a list of file server's files and a directory in local file system to store the actual files. `syslog` is used for logging.

#### Configuration
File server uses a single configuration file `/etc/dfs_fileserv.conf`. Alternative file path may be given as a command line parameter.

Configuration parameters are
- `directory_server` - IP-address or name of the directory server
- `data_location` - path to server's data directory in local file system
- `maximum_space` - maximum total size of files to store, may use abbrevations (e.g. 10G, 500M), minimum size is 10M

### Data files
Data files are saved to a directory named `filedata` which is in the data directory. Hierarchy under the `filedata` directory is same as in the file system. That is if there is a file in path `/public/code/main.c` in the file system, the file can be found from `filedata/public/code/main.c` at the file server. File modification times are stored in memory, but saved to `timestamps` file when server process is closed. `timestamps` is a text file and it contains one path and timestamp per line, separated by single space. Timestamps are stored as integers.

### Directory server
Directory server uses a configuration file. File system's directory is stored in a file. `syslog` is used for logging.

#### Configuration
Directory server has a single configuration file `/etc/dfs_dirserv.conf`. Alternative file path may be given as a command line parameter.

Configuration parameters are
- `data_location` - path to directory to store server data

