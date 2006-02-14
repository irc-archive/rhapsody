int process_sock4_server_message(server *S, unsigned char *message, int len);
int process_sock4_dccchat_message(dcc_chat *D, unsigned char *message, int len);
int process_sock4_dccfile_message(dcc_file *D, unsigned char *message, int len);

char *socks4_error_message(int code);
