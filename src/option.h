#define FORM_ID_OK 1
#define FORM_ID_CANCEL 27
#define FORM_ID_YES 1
#define FORM_ID_NO 0
#define FORM_ID_RESET 3
#define FORM_ID_DEFAULT 3

#define FORM_ID_SERVER 10
#define FORM_ID_PORT 11
#define FORM_ID_SERVER_ADD 12
#define FORM_ID_SERVER_DELETE 13
#define FORM_ID_SERVER_EDIT 14
#define FORM_ID_PASSWORD 15

#define FORM_ID_NICK 32 
#define FORM_ID_ALTNICK 33 
#define FORM_ID_NAME 34 
#define FORM_ID_USER 35
#define FORM_ID_HOST 36
#define FORM_ID_DOMAIN 37
#define FORM_ID_MODE_I 38
#define FORM_ID_MODE_W 39
#define FORM_ID_MODE_S 40

#define FORM_ID_CONNECT_TIMEOUT 51 
#define FORM_ID_AUTOSAVE 52
#define FORM_ID_OTHER 255
#define FORM_ID_SERVERLIST_START 1024

#define FORM_ID_CHANNEL 50
#define FORM_ID_CHANNEL_ADD 51
#define FORM_ID_CHANNEL_DELETE 52
#define FORM_ID_CHANNEL_EDIT 53
#define FORM_ID_CHANNELLIST_START 2048

#define FORM_ID_SEARCH 60
#define FORM_ID_USERSMORE 61
#define FORM_ID_USERSLESS 62
#define FORM_ID_SORT 63

#define FORM_ID_SORTCHANNEL LIST_SORT_CHANNEL
#define FORM_ID_SORTDESC LIST_SORT_DESCRIPTION
#define FORM_ID_SORTUSERS LIST_SORT_USERS

#define FORM_ID_USERS 70
#define FORM_ID_USER_ADD 71
#define FORM_ID_USER_DELETE 72
#define FORM_ID_USER_EDIT 73
#define FORM_ID_USERLIST_START 2048

#define FORM_ID_FILE 80
#define FORM_ID_FILELIST_START 2048
#define FORM_ID_BLOCKSIZE 90
#define FORM_ID_DLPATH 91
#define FORM_ID_ULPATH 92
#define FORM_ID_DLACCEPT 93
#define FORM_ID_DLDUPLICATES 94

#define FORM_ID_DCCHOST 95
#define FORM_ID_DCCPORTSTART 96
#define FORM_ID_DCCPORTEND 97

#define FORM_ID_TRANSFERSTOP 98
#define FORM_ID_TRANSFERINFO 99
#define FORM_ID_TRANSFERALLOW 100

#define FORM_ID_CTCP_ON 110
#define FORM_ID_CTCP_THROTTLE 111
#define FORM_ID_CTCP_FINGER 112
#define FORM_ID_CTCP_USERINFO 113

#define FORM_ID_WINCOLOR_F 120
#define FORM_ID_WINCOLOR_B 121
#define FORM_ID_MENUCOLOR_B 122
#define FORM_ID_MENUCOLOR_F 123
#define FORM_ID_FORMCOLOR_F 124
#define FORM_ID_FORMCOLOR_B 125
#define FORM_ID_FIELDCOLOR_F 126
#define FORM_ID_FIELDCOLOR_B 127
#define FORM_ID_BUTTONCOLOR_F 128
#define FORM_ID_BUTTONCOLOR_B 129

#define FORM_ID_MESSAGECOLOR 140
#define FORM_ID_ERRORCOLOR 141
#define FORM_ID_NOTICECOLOR 142
#define FORM_ID_CTCPCOLOR 143 
#define FORM_ID_DCCCOLOR 144
#define FORM_ID_JOINCOLOR 145 
#define FORM_ID_RENAMECOLOR 146
#define FORM_ID_KICKCOLOR 147
#define FORM_ID_MODECOLOR 148
#define FORM_ID_INVITECOLOR 149


/** form event handlers *********************************************************************/

int close_all_forms();

/** new server connect **********************************************************************/

int get_new_connect_info(int key, char *server, int *port, char *password);
form *create_new_server_connect_form(char *server, int port, char *password);


/** favorite server connect *****************************************************************/

int get_favorite_connect_info(int key, char *server, int *port);
form *create_favorite_server_connect_form();


/** favorite server edit ********************************************************************/

int edit_favorite_servers(int key);
form *create_new_favorite_server_form();
form *create_edit_favorite_server_form();
form *create_change_favorite_server_form(char *server, int port, char *password);


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

/** color options ***************************************************************************/

int get_color_info(int key);
form *create_color_options_form();
void create_color_list(Flist *comp, int curcolor);

int get_term_info(int key);


/** help options ****************************************************************************/

int view_about(int key);
int view_screen_size_warning(int key);


int get_transfer_select_options(int key);
int get_transfer_info(int key, dcc_file *file);

int allow_transfer(int key, dcc_file *F);
