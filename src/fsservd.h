struct client_info {
	uint64_t id;
	int sd;
};

void read_args(int argc, char* argv[]);
void go_daemon();
void connect_dirserv();
void start_listen();
void do_fap_server();
void serve_client(struct client_info *info);
int register_client(int clisd, uint64_t cid);
uint64_t nextcid();
