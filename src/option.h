/** form event handlers *********************************************************************/

int close_all_forms();

/** new server connect **********************************************************************/

int get_new_connect_info(int key, char *server, int *port);
form *create_new_server_connect_form(char *server, int port);


/** favorite server connect *****************************************************************/

int get_favorite_connect_info(int key, char *server, int *port);
form *create_favorite_server_connect_form();


/** favorite server edit ********************************************************************/

int edit_favorite_servers(int key);
form *create_new_favorite_server_form();
form *create_edit_favorite_server_form();
form *create_change_favorite_server_form(char *server, int port);


/** new channel join ************************************************************************/

int get_new_join_info(int key, char *channel);
form *create_new_channel_join_form();

/** favourite channel join  *****************************************************************/
        
int get_favorite_join_info(int key, char *channel);
form *create_favorite_channel_join_form();

/** favorite channel edit *******************************************************************/

int edit_favorite_channels(int key);
form *create_edit_favorite_channel_form();
form *create_new_favorite_channel_form();
form *create_change_favorite_channel_form(char *channel);

/**  channel select options  ****************************************************************/
        
int get_channel_select_options(int key);
form *create_channel_select_form();




/** new chat *******************************************************************************/

int get_new_chat_info(int key, char *user);
form *create_new_chat_form();

/** favourite chat *************************************************************************/
        
int get_favorite_user_chat_info(int key, char *user);
form *create_favorite_chat_form();

/** new dcc chat ***************************************************************************/

int get_new_dccchat_info(int key, char *user);
form *create_new_dccchat_form();

/** favourite dcc chat *********************************************************************/
        
int get_favorite_user_dccchat_info(int key, char *user);
form *create_favorite_dccchat_form();

/** new dcc send ***************************************************************************/

int get_new_dccsend_info(int key, char *user);
form *create_new_dccsend_form();

int select_file(int key, char *path, int options);
form *create_directory_listing_form(char *path);

/** favourite dcc chat *********************************************************************/
        
int get_favorite_user_dccsend_info(int key, char *user);
form *create_favorite_dccsend_form();

/** fav/ban user edit **********************************************************************/

int edit_users(int key, int listnum);
form *create_edit_user_form();
form *create_new_user_form(char *title);
form *create_change_user_form(char *title, char *user);



/** client identity *************************************************************************/

int get_identity_info(int key);
form *create_identity_form(char *nick, char *altnick, char *name, char *user, char *host, char *domain, char *mode);

/** ctcp options ****************************************************************************/

int get_ctcp_info(int key);
form *create_ctcp_form(int status, int throttle, char *finger, char *userinfo);

/** dcc options *****************************************************************************/

int get_dcc_info(int key);
form *create_dcc_form(char *hostname, int startport, int endport);
int get_dcc_send_info(int key);
form *create_dcc_send_form(char *dlpath, char *ulpath, int blocksize, int accept, int duplicates);

/** client options **************************************************************************/

int get_client_options(int key);
form *create_client_options_form(int timeout, int autosave);

/** list options ****************************************************************************/

int get_list_view_options(int key, char *searchstr, int *minuser, int *maxuser, int *searchtype);
form *create_list_view_form();



/** help options ****************************************************************************/

int view_about(int key);
int view_screen_size_warning(int key);


int get_transfer_select_options(int key);
int get_transfer_info(int key, dcc_file *file);

int allow_transfer(int key, dcc_file *F);
