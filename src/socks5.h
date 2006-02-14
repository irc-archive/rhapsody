int process_sock5_server_message(server *S, unsigned char *message, int len);
int process_sock5_dccchat_message(dcc_chat *D, unsigned char *message, int len);
int process_sock5_dccfile_message(dcc_file *D, unsigned char *message, int len);

char *socks5_error_message(int code);
int get_socks5_host_and_port(unsigned char *message, char *hostname, int *port);
