dcc_file *add_incoming_dcc_file(transfer *transfer, char *nick, char *filename, unsigned long hostip, unsigned int port, int size);
dcc_file *add_outgoing_dcc_file(transfer *transfer, char *nick, char *filename);

int start_incoming_dcc_chat(dcc_chat *D);
int start_outgoing_dcc_chat(dcc_chat *D);
int start_incoming_dcc_file(dcc_file *D);  	
int start_outgoing_dcc_file(dcc_file *D);  	

int get_dcc_file(dcc_file *D);
int get_dcc_file_blocking(dcc_file *D);

int dcc_file_exists(transfer *T, dcc_file *D);
void remove_dcc_file(dcc_file *D);
void remove_dcc_chat(dcc_chat *D);

int put_dcc_file(dcc_file *D);

void gen_dccack(unsigned long byte, unsigned char *ack);
int get_dccack(dcc_file *D);








