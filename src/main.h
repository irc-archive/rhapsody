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
void send_current_server(char *message);
void send_server(server *S, char *template, ...);

int get_serverfd(screen *screen);

server *get_server(screen *screen);
 




