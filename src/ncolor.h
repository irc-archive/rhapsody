int HAS_COLORS;
int CHANGE_COLORS;

/* map 16 color rhapsody palette to 6 color curses palette */

#define C_BLACK		COLOR_BLACK
#define C_RED		COLOR_RED
#define C_GREEN		COLOR_GREEN
#define C_YELLOW	COLOR_YELLOW
#define C_BLUE		COLOR_BLUE
#define C_MAGENTA	COLOR_MAGENTA
#define C_CYAN		COLOR_CYAN
#define C_WHITE		COLOR_WHITE

#define C_BROWN		8
#define C_PURPLE	9
#define C_ORANGE	10
#define C_LRED		11
#define C_LGREEN	12
#define C_LBLUE		13
#define C_GREY		14
#define C_LGREY		15

#define DEFAULT_COLOR_B COLOR_BLACK
#define DEFAULT_COLOR_F COLOR_WHITE

int begin_color(void);
void init_color_palette(void);

int mirc_palette(int fg, int bg);
int make_color_pair(int fg, int bg);
int make_color(int fg, int bg);
