#include "dcp.h"
#include "protomsg.h"
#include <stdint.h>

void go_daemon();
void do_dcp_server();
void serve_fileserver(struct fileserv_info *info);

int register_fsrv(int fsrvsd, uint64_t sid);
uint64_t nextsid();

void read_dir(struct fileserv_info *info, int recurse, char *path);
void create_file(struct fileserv_info *info, struct fileinfo_sect *fi);

//directory structures
struct node {
	char *name;
	char *user;
	struct node *children;
	struct node *next;
};

struct node *dir_init();
struct node *add_node(const char *path);
void add_child(struct node *p, struct node *c);
struct node *dir_find_node(const char *pathc, int exact);
