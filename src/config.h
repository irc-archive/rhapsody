#define MAXLINELEN 256
#define MAXSCOPELEN 64

#define CONFIG_FAVORITE_USER_LIST 0
#define CONFIG_IGNORED_USER_LIST 1

#define LIST_ORDER_FRONT 0
#define LIST_ORDER_BACK 1

FILE *open_config(char *config_file);
int read_config(char *config_file, config *C);
int writeconfig(char *config_file, config *C);

int add_config_server(config *C, char *name, unsigned int port, char *password, int order);
int remove_config_server(config *C, config_server *server);
int config_server_exists(config *C, char *name, unsigned int port);

int add_config_channel(config *C, char *name, int order);
int remove_config_channel(config *C, config_channel *channel);
int config_channel_exists(config *C, char *name);

int add_config_user(config *C, int listnum, char *name, int order);
int remove_config_user(config *C, int listnum, config_user *U);
int remove_config_user_by_name(config *C, int listnum, char *name);
int config_user_exists(config *C, int listnum, char *name);
int config_user_exists_exact(config *C, int listnum, char *name);
config_user *config_user_exact(config *C, int listnum, char *name);

int string_match(char *str, char *exp);

void print_config_info(config *C);

server *new_server_config(config *C);
