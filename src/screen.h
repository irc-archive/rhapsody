#define O_WORD_WRAP 0x01
#define O_COLOR 0x02
#define O_BOLD 0x04
#define O_UNDERLINE 0x08
#define O_INDENT 0x10
#define O_REVERSE 0x20
#define O_NOMONOCHROME 0x1000
#define O_ALL 0xFFFF


screen *add_screen(void *screenptr, int type);
void remove_screen(screen *screenptr);

int print_screen(WINDOW *win, char *buffer);	
int print_screen_opt(WINDOW *win, char *buffer, int linesize, int indent, int attr_def, int options);
void print_all(char *buffer);
void print_all_attrib(char *buffer, int attrib);
void vprint_all_attrib(int attrib, char *template, ...);
void vprint_all(char *template, ...);

void scroll_message_screen(screen *screen, int lines);
void set_message_scrolling(screen *screen, int scroll);	
void redraw_screen(screen *current);
void refresh_screen(screen *current);

//void set_update_status(screen *S, int update){
//void unset_update_status(screen *S, int update){
//inline int update_status(screen *S){
void set_screen_update_status(screen *scr, int update);

screen *select_next_screen(screen *current);
screen *select_prev_screen(screen *current);
screen *select_screen(screen *new);

int process_screen_events(screen *screen, int key);
void hide_screen(screen *Current, int hide);

/* server *************************************************************************************************/

int create_server_screen(server *S);
int delete_server_screen(server *S);
int redraw_server_screen(server *S);
server *add_server(char *servername, int port, char *nick, char *user, char *host, char *domain, char *name);
void refresh_server_screen(server *S);

void print_server(server *S, char *buffer);
void print_server_attrib(server *S, char *buffer, int attrib);
void vprint_server_attrib(server *S, int attrib, char *template, ...);
void vprint_server(server *S, char *template, ...);

int server_update_status(server *S);
void set_server_update_status(server *S, int update);
void unset_server_update_status(server *S, int update);



/* channel ************************************************************************************************/

int create_channel_screen(channel *C);
int delete_channel_screen(channel *C);
int redraw_channel_screen(channel *C);
void refresh_channel_screen(channel *C);
channel *add_channel(char *channelname, server *server);

void refresh_user_list(channel *C);

void printmsg_channel(channel *C, char *nick, char *buffer);
void print_channel(channel *C, char *buffer);
void print_channel_attrib(channel *C, char *buffer, int attrib);
void printmymsg_channel(channel *C, char *buffer);
void vprint_channel_attrib(channel *C, int attrib, char *template, ...);
void vprint_channel(channel *C, char *template, ...);

void refresh_user_list(channel *C);
int user_win_offset(channel *C);

int add_user(channel *C, char *nick, int op, int voice);
int remove_user(channel *C, char *nick);
int change_user_status(channel *C, char *nick, char *mode);
void remove_all_users(channel *C);
void quit_user(char *nick);
char get_user_status(channel *C, char *nick, int *op, int *voice);

void select_next_user(channel *C);
void select_prev_user(channel *C);
void select_next_user_by_key(channel *C, int key);
char *selected_channel_nick(channel *C);

void end_channel(channel *C);

int channel_update_status(channel *S);
void set_channel_update_status(channel *S, int update);
void unset_channel_update_status(channel *S, int update);


/* chat ***************************************************************************************************/


int create_chat_screen(chat *C);
int delete_chat_screen(chat *C);
int redraw_chat_screen(chat *C);
void refresh_chat_screen(chat *C);
chat *add_chat(char *chatname, server *server);

void print_chat(chat *C, char *buffer);
void print_chat_attrib(chat *C, char *buffer, int attrib);
void printmsg_chat(chat *C, char *nick, char *buffer);
void printmymsg_chat(chat *C, char *buffer);
void vprint_chat_attrib(chat *C, int attrib, char *template, ...);
void vprint_chat(chat *C, char *template, ...);

void end_chat(chat *C);

int chat_update_status(chat *S);
void set_chat_update_status(chat *S, int update);
void unset_chat_update_status(chat *S, int update);


/* dcc chat ***********************************************************************************************/

int create_dccchat_screen(dcc_chat *D);
int delete_dccchat_screen(dcc_chat *D);
int redraw_dccchat_screen(dcc_chat *D);
void refresh_dccchat_screen(dcc_chat *C);

dcc_chat *add_incoming_dcc_chat(char *nick, char *dest, server *server, unsigned long hostip, unsigned short port);
dcc_chat *add_outgoing_dcc_chat(char *nick, char *dest, server *server);

void printmsg_dcc_chat(dcc_chat *C, char *nick, char *buffer);
void printmymsg_dcc_chat(dcc_chat *D, char *buffer);
void print_dcc_chat(dcc_chat *C, char *buffer);		
void print_dcc_chat_attrib(dcc_chat *C, char *buffer, int attrib);		

void vprint_dcc_chat_attrib(dcc_chat *C, int attrib, char *template, ...);
void vprint_dcc_chat(dcc_chat *C, char *template, ...);

void disconnect_dccchat(dcc_chat *D);
void end_dccchat(dcc_chat *D);

int dccchat_update_status(dcc_chat *S);
void set_dccchat_update_status(dcc_chat *S, int update);
void unset_dccchat_update_status(dcc_chat *S, int update);



/* transfer **********************************************************************************************/

int create_transfer_screen(transfer *T);
int redraw_transfer_screen(transfer *T);
void refresh_transfer_screen(transfer *T);

transfer *add_transfer();

int transfer_update_status(transfer *S);
void set_transfer_update_status(transfer *S, int update);
void unset_transfer_update_status(transfer *S, int update);


/* list **************************************************************************************************/

int create_list_screen(list *L);        
list *add_list(server *server);
int redraw_list_screen(list *L);
void refresh_list_screen(list *L);
void print_list(list *L, char *buffer);
int print_list_pos_attrib(list *L, char *buffer, int x, int y, int attrib, int options);

void apply_list_view(list *L, char *search, int minusers, int maxusers, int sorttype);
void select_prev_list_channel(list *L);
void select_next_list_channel(list *L);
void select_next_list_channel_by_key(list *L, int key);
void refresh_channel_list(list *L);
int add_list_channel(list *L, char *channel, int users, char *description, int sorttype);

void select_prev_list_channel_page(list *L);
void select_next_list_channel_page(list *L);
char *selected_list_channel(list *L);
int is_channel_in_list_view(list *L, list_channel *channel);

int list_update_status(list *L);
void set_list_update_status(list *L, int update);
void unset_list_update_status(list *L, int update);

list *list_by_server(server *server);
list *active_list_by_server(server *server);

/* help **************************************************************************************************/

int create_help_screen(help *H);
int delete_help_screen(help *H);

help *add_help(char *helpname, char *subname, char *filename);
int redraw_help_screen(help *H);
void refresh_help_screen(help *H);

int help_update_status(help *H);
void set_help_update_status(help *H, int update);
void unset_help_update_status(help *H, int update);

void print_help(help *H, char *buffer);
void print_help_attrib(help *H, char *buffer, int attrib);
int print_help_file(help *H, char *filename);


/* input and status **************************************************************************************/


inputwin *create_input_screen();
void redraw_input_screen(inputwin *I);

//inputline_entry *add_inputline_entry(inputline_entry *head_entry, char *str_buffer);
//inputline_entry *prev_inputline_entry(inputline_entry *current, char *str_buffer);
//inputline_entry *next_inputline_entry(inputline_entry *current, char *buffer);

int add_inputline_entry(inputwin *I, char *buffer);
int prev_inputline_entry(inputwin *I, char *buffer);
int next_inputline_entry(inputwin *I, char *buffer);

int print_inputline(inputwin *inputline);
int process_inputline_events(inputwin *inputline, int event);

menuwin *create_menu_screen();
void redraw_menu_screen(menuwin *M);

statuswin *create_status_screen();
void redraw_status_screen(statuswin *W);

void set_input_buffer(inputwin *I, char *buffer);
void backspace_input_buffer(inputwin *I);
void delete_input_buffer(inputwin *I);
void add_input_buffer(inputwin *I, int value);
void append_input_buffer(inputwin *I, char *string);
void move_input_cursor(inputwin *I, int spaces);

void set_inputline_update_status(inputwin *S, int update);
void unset_inputline_update_status(inputwin *S, int update);
inline int inputline_update_status(inputwin *S);

void set_menuline_update_status(menuwin *S, int update);
void unset_menuline_update_status(menuwin *S, int update);
inline int menuline_update_status(menuwin *S);

void set_statusline_update_status(statuswin *S, int update);
void unset_statusline_update_status(statuswin *S, int update);
int statusline_update_status(statuswin *S);


/* misc **************************************************************************************************/


void progress_bar(WINDOW *win, int posy, int posx, int size, int percent);

server *server_by_name(char *servername);
screen *server_screen_by_name(char *servername);

channel *channel_by_name(char *channelname);
screen *channel_screen_by_name(char *channelname);

chat *chat_by_name(char *chatname);
screen *chat_screen_by_name(char *chatname);

dcc_chat *dcc_chat_by_name(char *chatname);
screen *dcc_chat_screen_by_name(char *chatname);

list *list_by_name(char *listname); 
list *active_list_by_name(char *listname); 
screen *list_screen_by_name(char *listname);

transfer *transfer_by_name(char *name); 
screen *transfer_screen_by_name(char *name);

