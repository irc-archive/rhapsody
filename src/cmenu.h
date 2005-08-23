#define M_NONE 0x0001
#define M_SELECTABLE 0x0001
#define M_DIVIDER 0x0010
#define M_ADD_FIRST 0x0100
#define M_AUTO -1

#define MENUNAMELEN 64

typedef struct MENUDATA menu;
typedef struct MENUITEMDATA menuitem;
typedef struct MENUSBARDATA menubar;

struct MENUSBARDATA{
	int entries;
	int textstart;
	int textspace;
	int changed;
	int update;
	int reconstruct;
	menu *menuhead;
	menu *menutail;
	menu *selected;
};

struct MENUDATA{
	WINDOW *window;
	int width;
	int height;
	int startx;
	int starty;
	int entries;
	char name[MENUNAMELEN];
	menuitem *itemhead;
	menuitem *itemtail;
	menuitem *selected;
	menuitem *chosen;
	int key;
	int id;
	menu *next;
	menu *prev;
	menubar *bar;
};

struct MENUITEMDATA{
	int option;
	char text[64];
	char description[256];
	int key;
	int id;
	menu *child;
	menuitem *next;
	menuitem *prev;
};

/*** menusline ************************************************************************/

menubar *init_menubar(int textstart, int textspace, int id);
void print_menubar(menuwin *menuline, menubar *menubar);
int process_menubar_events(menubar *line, int event);
int add_menubar_menu(menubar *menubarptr, menu *menuptr, int option);
void close_menubar(menubar *menubar);
menu *get_menubar_menu(menubar *menubarptr, int id);

int selected_menubar_item_id(menubar *line);
inline int menubar_update_status(menubar *menubar);
inline void set_menubar_update_status(menubar *menubar, int update);
void unset_menubar_update_status(menubar *menubar, int update);

void init_all_menus();

/*** menu ****************************************************************************/
        
menu *init_menu(int startx, int starty, char *name, int highlight, int id);
int delete_menu_items(menu *menuptr);
int delete_menu(menu *menuptr);
void set_menu_position(menu *menuprt, int startx, int starty);

menuitem *add_menu_item(menu *menuptr, char *text, char *desc, int key, int option, int id, menu *child);
void print_menu(menu *menuptr);
int process_menu_events(menu *menu, int event);

int select_next_item(menu *menuptr);
int select_prev_item(menu *menuptr);
inline int selected_menu_item_id(menu *menuptr);


extern menubar *servermenus;
extern menubar *channelmenus;
extern menubar *chatmenus;
extern menubar *dccchatmenus;
extern menubar *transfermenus;
extern menubar *listmenus;
extern menubar *helpmenus;
extern menubar *currentmenusline;

extern menu *UserMenu;
extern menu *UserListMenu;
extern menu *CtcpMenu;
extern menu *DCCMenu;
extern menu *ControlMenu;
