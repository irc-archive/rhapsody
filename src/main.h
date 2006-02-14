/* define active form ids */

#define CF_NEW_SERVER_CONNECT 1
#define CF_FAVORITE_SERVER_CONNECT 2
#define CF_EDIT_SERVER_FAVORITES 3

#define CF_IDENTITY 10   
#define CF_DCC_OPTIONS 11
#define CF_DCC_SEND_OPTIONS 12
#define CF_CLIENT_OPTIONS 13
#define CF_SCRIPT_OPTIONS 14
#define CF_CTCP_OPTIONS 15
#define CF_COLOR_OPTIONS 16
#define CF_TERM_INFO 17
#define CF_NETWORK_OPTIONS 18

#define CF_EDIT_CHANNEL_FAVORITES 20
#define CF_JOIN 21
#define CF_JOIN_FAVORITE 22

#define CF_LIST_OPTIONS 30
#define CF_CHANNEL_SELECT 32

#define CF_EDIT_USER_FAVORITES 40
#define CF_EDIT_USER_IGNORED 41
#define CF_NEW_CHAT 42
#define CF_FAVORITE_CHAT 43
#define CF_NEW_DCC_CHAT 44
#define CF_FAVORITE_DCC_CHAT 45
#define CF_NEW_DCC_SEND 46
#define CF_FAVORITE_DCC_SEND 47
#define CF_DCC_SEND_FILE 48

#define CF_HELP_ABOUT 50

#define CF_FILE_TRANSFER 60
#define CF_FILE_TRANSFER_INFO 61
#define CF_FILE_ALLOW 62

#define CF_EXIT 250
#define CF_EXIT_LASTSERVER 251
#define CF_CLOSE_SERVER 252

int ctrl_c_occured;
int alarm_occured;
int resize_occured;

/* prototypes *******************************************/

/* main finctions */
void parse_message(server *currentserver, char *buffer);
int parse_input(server *currentserver, char *buffer);
int end_run();
// menu *build_window_menu(int startx, int starty);

/* handler functions */

int ctrl_c_handler(void);
int alarm_handler(void);
int resize_handler(void);
int inst_ctrlc_handler(void);
int inst_alarm_handler(void);
int inst_resize_handler(void);
int remove_resize_handler(void);

/* data structure maintnance functions */

void update_transfer_info(transfer *T);
void update_current_screen();

/* communication functions */

int connect_to_server (server *S);
void sendcmd_server(server *S, char *command, char *args, char *dest, char *nick);
void sendmsg_channel(channel *C, char *message);
void sendmsg_chat(chat *C, char *message);
void sendmsg_dcc_chat(dcc_chat *C, char *message);
// void send_current_server(char *message);
void send_server(server *S, char *template, ...);

int get_serverfd(screen *screen);

server *get_server(screen *screen);
 




