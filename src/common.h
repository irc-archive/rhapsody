#define U_ALL 0xFFFF
#define U_MAIN_REFRESH 0x0001
#define U_MAIN_REDRAW 0x0002
#define U_USER_REFRESH 0x0004
#define U_USER_REDRAW 0x0008
#define U_BG_REFRESH 0x0010
#define U_BG_REDRAW 0x0010

#define U_ALL_REFRESH 0x0005
#define U_ALL_REDRAW 0x000A
#define U_NONE 0x0000

/* default screen color */
#define DEFAULT_WIN_COLOR_F C_WHITE
#define DEFAULT_WIN_COLOR_B C_BLACK

/* menus are reversed */
#define DEFAULT_MENU_COLOR_F C_BLUE
#define DEFAULT_MENU_COLOR_B C_WHITE

/* forms are reversed */
#define DEFAULT_FORM_COLOR_B C_WHITE
#define DEFAULT_FORM_COLOR_F C_BLUE

/* form components are not reversed */
#define DEFAULT_FORMBUTTON_COLOR_B C_WHITE
#define DEFAULT_FORMBUTTON_COLOR_F C_BLACK
#define DEFAULT_FORMFIELD_COLOR_B C_WHITE
#define DEFAULT_FORMFIELD_COLOR_F C_BLACK

#define DEFAULT_MESSAGE_COLOR C_CYAN
#define DEFAULT_ERROR_COLOR C_RED
#define DEFAULT_NOTICE_COLOR C_GREEN
#define DEFAULT_CTCP_COLOR C_MAGENTA
#define DEFAULT_DCC_COLOR C_CYAN

#define DEFAULT_JOIN_COLOR C_YELLOW
#define DEFAULT_RENAME_COLOR C_GREEN
#define DEFAULT_KICK_COLOR C_RED
#define DEFAULT_MODE_COLOR C_GREEN
#define DEFAULT_INVITE_COLOR C_GREEN

#define PROGRESS_COLOR_F C_BLACK
#define PROGRESS_COLOR_B C_YELLOW

#define MENU_COLOR make_color(configuration.menu_color_fg, configuration.menu_color_bg)

#define MESSAGE_COLOR make_color(configuration.message_color, configuration.win_color_bg)
#define ERROR_COLOR make_color(configuration.error_color, configuration.win_color_bg)
#define NOTICE_COLOR make_color(configuration.notice_color, configuration.win_color_bg)
#define CTCP_COLOR make_color(configuration.ctcp_color, configuration.win_color_bg)
#define DCC_COLOR make_color(configuration.dcc_color, configuration.win_color_bg)
#define JOIN_COLOR make_color(configuration.join_color, configuration.win_color_bg)
#define RENAME_COLOR make_color(configuration.rename_color, configuration.win_color_bg)
#define KICK_COLOR make_color(configuration.kick_color, configuration.win_color_bg)
#define MODE_COLOR make_color(configuration.mode_color, configuration.win_color_bg)
#define INVITE_COLOR make_color(configuration.invite_color, configuration.win_color_bg)


#define USERWINWIDTH 12

#define DCC_SEND 5
#define DCC_RECEIVE 1
#define DCC_CHAT 2
#define DCC_SOUND 3

#define SERVER 1
#define CHANNEL 2
#define CHAT 3
#define DCCCHAT 4
#define TRANSFER 5
#define LIST 6
#define HELP 7

#define MAXNICKLEN 64
#define MAXNICKDISPLEN 10
#define MAXHOSTLEN 64
#define MAXDOMLEN 256
#define MAXDESCLEN 256
#define MAXTOPICLEN 256
#define MAXMODELEN 16
#define MAXFILELEN 1024
#define MAXSERVERLEN MAXHOSTLEN
#define MAXCHANNELLEN 64
#define MAXPASSLEN 256

#define MEMALLOC_ERR 2

#define LIST_SORT_USERS 1
#define LIST_SORT_CHANNEL 2
#define LIST_SORT_DESCRIPTION 3

typedef struct screen_list_entry screen;
typedef struct server_info server;
typedef struct channel_info channel;
typedef struct nick_info user;
typedef struct chat_info chat;
typedef struct dcc_file_info dcc_file;
typedef struct dcc_chat_info dcc_chat;
typedef struct transfer_info transfer;
typedef struct channel_list_info list;
typedef struct help_info help;

typedef struct inputline_entry inputline_entry;
typedef struct inputline_info inputwin;
typedef struct menuline_info menuwin;
typedef struct statusline_info statuswin;
typedef struct config_data config;
typedef struct config_server_data config_server;
typedef struct config_channel_data config_channel;
typedef struct config_user_data config_user;
typedef struct channel_list_channel_info list_channel;

struct screen_list_entry{  
        void *screen;
        struct screen_list_entry *prev;
        struct screen_list_entry *next;
        int type;   
	int scrolling;
	int scrollpos;
	int update;
	int hidden;
};

struct server_info{
	WINDOW *message;
	int active;
	char server[MAXHOSTLEN];
	int port;
	char nick[MAXNICKLEN];
	char lastnick[MAXNICKLEN];
	char user[MAXNICKLEN];
	char host[MAXHOSTLEN];
	char domain[MAXDOMLEN];
	char name[MAXDESCLEN];
	char password[MAXPASSLEN];
	int serverfd;
	int update;
	int connect_status;
	int nickinuse;
};

struct channel_info{
	WINDOW *message;
	WINDOW *user;
	WINDOW *vline;
	int active;
	char channel[MAXCHANNELLEN];
	char topic[MAXTOPICLEN];
	user *userlist;
	user *selected;
	user *top;
	int userlist_done;
	int selecting;
	server *server;
	int update;	
};

struct nick_info{
	struct nick_info *prev;
	struct nick_info *next;
	char nick[MAXNICKLEN];
	int op;
	int voice;
};

struct chat_info{
        WINDOW *message;
        char nick[MAXNICKLEN];   
        server *server;
	int update;
};

struct dcc_chat_info{
	WINDOW *message;
	int active;
	int type;
	unsigned long hostip;
	unsigned int port;
	unsigned long localip;
	unsigned int localport;	
	char nick[MAXNICKLEN];
	char dest[MAXDOMLEN];
        server *server;
	int dccfd;
	int update;
	int serverstatus;
	int direction;
	int allowed;
};

struct dcc_file_info{
	int type;
	int active;
	char filename[MAXFILELEN];
	unsigned long size;
	unsigned long byte;
	unsigned long ackbyte;
	unsigned long hostip;
	unsigned int port;
	unsigned long localip;
	unsigned int localport;	
	char nick[64];
	int dccfd;
	int serverstatus;
	time_t starttime;
	time_t last_activity_at;
	time_t last_updated_at;
	FILE *dccfp;
	transfer *transfer;
	dcc_file *next;
	dcc_file *prev;
	int direction;
	int allowed;
};

struct transfer_info{
	WINDOW *message;
	char name[MAXDESCLEN];
	dcc_file *dcclist;
	dcc_file *dcclisttop;
	dcc_file *selectedfile;
	int update;
};

struct channel_list_info{
	WINDOW *message;
	int active;
	server *server;
	char servername[MAXDOMLEN];
	int update;
	int usinglist;
	list_channel *list;
	list_channel *alphalist;
	list_channel *userlist;
	list_channel *view;
	list_channel *top;
	list_channel *selected;
	
};

struct channel_list_channel_info{
	list_channel *alphaprev;
	list_channel *alphanext;
	list_channel *userprev;
	list_channel *usernext;
	list_channel *viewprev;
	list_channel *viewnext;
	char channel[MAXCHANNELLEN];
	char description[MAXDESCLEN];
	int users;
};

struct help_info{
	WINDOW *message;
	char name[MAXDESCLEN];
	char subname[MAXDESCLEN];
	int update;
};

struct inputline_entry{
        struct inputline_entry *prev;
        struct inputline_entry *next;
        char *buffer;
};

struct inputline_info{
	WINDOW *inputline;
	char inputbuffer[MAXDATASIZE];
	int cursorstart;
	int cursorpos;
	int update;
	inputline_entry *head;
	inputline_entry *current;
};

struct statusline_info{
	WINDOW *statusline;
	int update;
};

struct menuline_info{
	WINDOW *menuline;
	int update;
};

struct config_server_data{
        char name[MAXDOMLEN];
        unsigned int port;
	char password[MAXPASSLEN];
        config_server *next;
        config_server *prev;
};

struct config_channel_data{
        char name[MAXDOMLEN];
        config_channel *next;
        config_channel *prev;
};

struct config_user_data{
        char name[MAXDOMLEN];
        config_user *next;
        config_user *prev;
};

struct config_data{
        char nick[MAXNICKLEN];
        char alt_nick[MAXNICKLEN];   
        char user[MAXNICKLEN];
        char hostname[MAXHOSTLEN];
        char domain[MAXDOMLEN];
        char userdesc[MAXDESCLEN];
        char mode[MAXMODELEN];
	char dccdlpath[MAXFILELEN];
	char dcculpath[MAXFILELEN];
	char dcchostname[MAXHOSTLEN];
	char ctcpfinger[MAXDESCLEN];
	char ctcpuserinfo[MAXDESCLEN];
	int dccstartport;
	int dccendport;
	int dccblocksize;
	int dccaccept;
	int dccdup;
	int dccduplicates;
	int autosave;
	int connecttimeout;
	int ctcpreply;
	int ctcpthrottle;
        config_server *serverfavorite;           
        config_server *lastserverfavorite;           
        config_channel *channelfavorite;
        config_channel *lastchannelfavorite;
        config_user *userfavorite;           
        config_user *lastuserfavorite;           
        config_user *userignored;
        config_user *lastuserignored;
	int changed;

	/* application color definitions */
	int menu_color_bg;
	int menu_color_fg;
	int win_color_fg;
	int win_color_bg;
	int form_color_bg;
	int form_color_fg;
	int formbutton_color_fg;	
	int formbutton_color_bg;
	int formfield_color_fg;
	int formfield_color_bg;

	/* message color definitions */
	int message_color;
	int error_color;
	int notice_color;
	int ctcp_color;
	int dcc_color;
	int join_color;
	int rename_color;
	int kick_color;
	int mode_color;
	int invite_color;
};

inputline_entry *inputline_head;
inputline_entry *inputline_current;

screen *screenlist;
inputwin *inputline;
menuwin *menuline;
statuswin *statusline;   

screen *screenlist;
screen *currentscreen;

transfer *transferscreen;
config configuration;

char installpath[MAXFILELEN];
char homepath[MAXFILELEN];
char configfile[MAXFILELEN];
char loginuser[MAXNICKLEN];
char hostname[MAXHOSTLEN];
char domain[MAXDOMLEN];



