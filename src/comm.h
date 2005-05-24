int disconnect_from_server (server *S);
int connect_to_server (server *S);

void sendcmd_server(server *S, char *command, char *args, char *dest, char *nick);
void sendmsg_channel(channel *C, char *message);
void sendmsg_chat(chat *C, char *message);
void sendmsg_dcc_chat(dcc_chat *C, char *message);
void send_server(server *S, char *template, ...);

