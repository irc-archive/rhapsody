void progress_bar(WINDOW *win, int posy, int posx, int size, int percent);

server *server_by_name(char *servername);
screen *server_screen_by_name(char *servername);
int get_server_count();
int get_server_screen_count(server *S);

channel *channel_by_name(char *channelname, server *S);
screen *channel_screen_by_name(char *channelname, server *S);

chat *chat_by_name(char *chatname, server *S);
screen *chat_screen_by_name(char *chatname, server *S);

dcc_chat *dcc_chat_by_name(char *chatname, server *S);
screen *dcc_chat_screen_by_name(char *chatname, server *S);

list *list_by_name(char *listname); 
list *active_list_by_name(char *listname); 
screen *list_screen_by_name(char *listname);

transfer *transfer_by_name(char *name); 
screen *transfer_screen_by_name(char *name);

int close_screen_and_children(screen *scr);
int get_child_count(screen *scr);
