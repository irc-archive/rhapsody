int parse_ctcp(char *bufferout, char *bufferin); 	
int execute_ctcp(server *server, char *command, char *cmdnick, char *cmduser, char *cmdhost, char *dest);  	
int translate_ctcp_message (char *command, char *cmdnick, char *cmduser, char *cmdhost, char *message);

char *create_ctcp_message(char *message, ...);
char *create_ctcp_command(char *command, char *parameters, ...);
