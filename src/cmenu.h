#define M_NONE 0x0001
#define M_SELECTABLE 0x0001
#define M_DIVIDER 0x0010

#define MENUNAMELEN 64

typedef struct MENUDATA menu;
typedef struct MENUITEMDATA menuitem;
typedef struct MENUSBARDATA menubar;

struct MENUSBARDATA{
	int entries;
	int current;
	int textstart;
	int textspace;
	int changed;
	int update;
	int reconstruct;
	menu **menu;
};

struct MENUDATA{
	WINDOW *window;
	int width, height, startx, starty, entries;
	char name[MENUNAMELEN];
	menuitem *item;
	menuitem *selected;
	menuitem *chosen;
	int key;
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

/*** menu ****************************************************************************/
        
menu *init_menu(int startx, int starty, char *name, int highlight);
int delete_menu(menu *menuptr);
void move_menu(menu *menuprt, int startx, int starty);

menuitem *add_menu_item(menu *menuptr, char *text, char *desc, int key, int option, int id, menu *child);
void print_menu(menu *menuptr);
int process_menu_events(menu *menu, int event);

int select_next_item(menu *menuptr);
int select_prev_item(menu *menuptr);
inline int selected_menu_item_id(menu *menuptr);

/*** menusline ************************************************************************/

menubar *init_menubar(int entries, int textstart, int textspace, menu **menupptr);
void print_menubar(menuwin *menuline, menubar *menubar);
int process_menubar_events(menubar *line, int event);
void close_menubar(menubar *menubar);

int selected_menubar_item_id(menubar *line);
inline int menubar_update_status(menubar *menubar);
inline void set_menubar_update_status(menubar *menubar, int update);
void unset_menubar_update_status(menubar *menubar, int update);

void init_all_menus();


