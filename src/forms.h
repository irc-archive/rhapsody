#define IN_ALPHA 0x0001
#define IN_ALPHANUM 0x0002
#define IN_LETTERS 0x0003
#define IN_NUM 0x0004
#define IN_HEX 0x0008
#define IN_LOWER 0x0010
#define IN_UPPER 0x0020
#define IN_PUNCT 0x0040
#define IN_CONTROL 0x0080
#define IN_BLANK 0x0100
#define IN_PRINTABLE 0x0200

#define F_BLACK 0
#define F_WHITE 1
#define F_BLUE 2
#define F_GREEN 3
#define F_RED 4
#define F_LGREEN 6
#define F_YELLOW 7
#define F_PINK 13

#define STYLE_TITLE 0x0001
#define STYLE_CHECKBOX_ROUND 0x0010
#define STYLE_CHECKBOX_TRIANGLE 0x0020
#define STYLE_CHECKBOX_STAR 0x0040
#define STYLE_NO_HIGHLIGHT 0x0100
#define STYLE_MASK_TEXT 0x0200
#define STYLE_LEFT_JUSTIFY 0x1000
#define STYLE_RIGHT_JUSTIFY 0x2000
#define STYLE_CENTER_JUSTIFY 0x4000



#define F_TEXTLINE 1
#define F_LIST 2
#define F_BUTTON 3
#define F_CHECKBOX 4
#define F_CHECKBOX_ARRAY 5

#define FORMLIST_SORTED 1
#define FORMLIST_FIRST 2
#define FORMLIST_LAST 3

typedef struct form_list_info form_list;
typedef struct form_entry_info form_list_entry;

typedef struct form_info form;
typedef struct form_component_info Fcomponent;
typedef struct form_text_line Ftextline;
typedef struct form_list Flist;
typedef struct form_list_line Flistline;
typedef struct form_button Fbutton;
typedef struct form_checkbox Fcheckbox;
typedef struct form_checkbox_array Fcheckbox_array;
typedef struct form_checkbox_array_box Fcheckbox_array_box;
typedef struct form_textarea Ftextarea;

/*** form *****************************************************************************/
 
form *add_form (char *name, int id, int x, int y, int width, int height, int attr, int style);
Fcomponent *place_form_component (form *form, void *component, int type);
Fcomponent *add_form_component (form *form, void *component, int id, int type);
Fcomponent *active_form_component (form *form);
Fcomponent *form_component_by_id (form *form, int id);

int add_form_textarea (form *form, Ftextarea *textarea);

int process_form_events(form *form, int event);
int active_form_component_id (form *form);
form *remove_form(form *form);
int remove_all_forms(void);
int remove_form_component (form *form, Fcomponent *component);
void print_form(form *form);

/*** text area ***********************************************************************/

Ftextarea *add_Ftextarea(char *name, int x, int y, int width, int height, int style, int attr, char *template, ...);
int remove_Ftextarea(form *form, Ftextarea *area);
void print_Ftextarea(form *form, Ftextarea *area);
void set_Ftextarea_buffer(Ftextarea *F, char *buffer);
char *Ftextarea_buffer_contents(Ftextarea *F);

/*** textline ************************************************************************/


Ftextline *add_Ftextline(char *name, int tx, int ty, int x, int y, int size, int width, int attr, int style, int inputallow);
int remove_Ftextline(Ftextline *textline);
int process_Ftextline_events(Ftextline *T, int event);
void set_Ftextline_buffer(Ftextline *F, char *buffer);
void backspace_Ftextline_buffer(Ftextline *F);
void delete_Ftextline_buffer(Ftextline *F);
void add_Ftextline_buffer(Ftextline *F, int value);
void move_Ftextline_cursor(Ftextline *F, int spaces);

char *Ftextline_buffer_contents(Ftextline *F);
void print_Ftextline(form *form, Ftextline *line, int active);


/*** list *****************************************************************************/

Flist *add_Flist(char *name, int tx, int ty, int x, int y, int width, int attrib, int fgcolor, int style);
int remove_Flist(Flist *list);
void print_Flist(form *form, Flist *list, int active);
int process_Flist_events(Flist *F, int event);
Flistline *add_Flistline(Flist *list, int id, char *string, void *ptrid, int type);

Flistline *active_list_line(Flist *F);
char *active_list_line_text(Flist *F);
int active_list_line_id(Flist *F);
void *active_list_line_ptrid(Flist *F);
int set_active_list_line_by_id(Flist *F, int id);
int set_active_list_line_by_ptrid(Flist *F, void *ptrid);

/*** button ***************************************************************************/

Fbutton *add_Fbutton(char *name, int x, int y, int width, int attr, int eventid, int style);
int remove_Fbutton(Fbutton *button);
void print_Fbutton(form *form, Fbutton *button, int active);
int process_Fbutton_events(Fbutton *F, int event);

/*** checkbox *************************************************************************/

Fcheckbox *add_Fcheckbox(char *name, int x, int y, int textx, int texty, int attrib, int style);
int remove_Fcheckbox(Fcheckbox *checkbox);
void print_Fcheckbox(form *form, Fcheckbox *checkbox, int active);
int process_Fcheckbox_events(Fcheckbox *B, int event);
int Fcheckbox_value(Fcheckbox *B);
void set_Fcheckbox_value(Fcheckbox *B, int value);

/*** checkbox_array *******************************************************************/

Fcheckbox_array *add_Fcheckbox_array(char *name, int textx, int texty, int attrib, int style);
int remove_Fcheckbox_array(Fcheckbox_array *array);
void print_Fcheckbox_array(form *form, Fcheckbox_array *checkboxes, int active);
int process_Fcheckbox_array_events(Fcheckbox_array *B, int event);
int Fcheckbox_array_active_num(Fcheckbox_array *B);

Fcheckbox_array_box *add_Fcheckbox_to_array(Fcheckbox_array *array, int id, Fcheckbox *box);
int Fcheckbox_array_selected_id(Fcheckbox_array *B);
int Fcheckbox_array_select(Fcheckbox_array *B, Fcheckbox_array_box *selected);
int Fcheckbox_array_select_by_id(Fcheckbox_array *B, int id);


/* list of opened forms */
struct form_list_info{
	form_list_entry *list;
	form_list_entry *last;
};

struct form_entry_info{
	form *form;
	form_list_entry *next;
	form_list_entry *prev;
};

struct form_info{
	char name[256];
	int id;
	int requestedx, requestedy;	// requested position of form on screen
	int x,y;			// actual position
	int width, height;		
	int attrib; 			// form color attribute
	int style;			// rendering style
	Fcomponent *list;		// component list start
	Fcomponent *endlist;		// component list end
	Fcomponent *active;		// currently selected component
	Ftextarea *textlist;		// list of text areas
	WINDOW *window;			// form's window
};

struct form_component_info{
	void *component;
	int type;
	int id;
	Fcomponent *next;
	Fcomponent *prev;
};

struct form_text_line{
	char name[256];
	char *buffer;
	int buffersize;
	int x,y;
	int namex, namey;
	int start;
	int width;
	int cursorpos;
	int attrib;
	int style;
	int inputallow;
};


struct form_list{
	char name[256];
	int x,y;
	int namex, namey;
	int height, width;
	Flistline *top;
	Flistline *list;
	Flistline *last;
	Flistline *selected;
	int style;
	int attrib;
};

struct form_list_line{
	int id;
	char *text;
	Flistline *prev;
	Flistline *next;
	void *ptrid;
};

struct form_button{
	char *text;
	int x,y;
	int height, width;
	int style;
	int attrib;
	int eventid;
};

struct form_checkbox{
	char *text;
	int x,y;
	int textx,texty;
	int style;
	int attrib;
	int selected;
};

struct form_checkbox_array{
	char *text;
	int textx,texty;
	int style;
	int attrib;
	Fcheckbox_array_box *selected;
	Fcheckbox_array_box *list;
	Fcheckbox_array_box *last;	
};

struct form_checkbox_array_box{
	int id;
	Fcheckbox *checkbox;
	Fcheckbox_array_box *next;
	Fcheckbox_array_box *prev;
};


struct form_textarea{
	char name[256];
	int x,y;
	int height, width;
	int style;
	int attrib;
	char *buffer;
	Ftextarea *next;
	Ftextarea *prev;
};

