#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define TITLE_LEN 256
#define TAG_LEN 4096
#define LIST_DEPTH 4
#define LIST_LEN 256

#define M_PARAGRAPH 0x0000
#define M_TAG 0x0001
#define M_TABLE 0x0002
#define M_TITLE 0x0004
#define M_LISTITEM 0x0008

#define L_UNORDERED 1
#define L_ORDERED 2
#define L_DEFINITION 3

#define TAG_ROOT 0x0000
#define TAG_ORDERED_LIST 0x0001
#define TAG_UNORDERED_LIST 0x0002
#define TAG_DEFINITION_LIST 0x0003
#define TAG_TABLE 0x0004
#define TAG_LINK 0x0005
#define TAG_BOLD 0x1000
#define TAG_ITALIC 0x1001
#define TAG_UNDERLINE 0x1002

#define TT_IMMEDIATE 0x0000
#define TT_LINE 0x0001
#define TT_SCOPE 0x0002
#define TT_PERSISTENT 0x0003

typedef struct tag_stack_entry Tag;
typedef struct tag_info_data TagInfo;

struct tag_stack_entry{
	int type;
	int persistence;
	Tag *parent;
	Tag *child;
	int startx;
	int starty;
	int endx;
	int endy;
	char *buffer;
};

struct tag_info_data{
	char *tag;
	int type;
	// int *function(char *);
	void *ptr;
};

/*
struct list_data{
	int type;
	listitem *item;
	listitem *desc;
};

struct list_item{
};
*/

/* globals */

int tag = 0;
int tagcount;
char currenttag[TAG_LEN];


static TagInfo *taginfo = {
	{"br", TT_IMMEDIATE, NULL},
	{"title", TT_LINE, NULL}
};
	
int main (){
	draw_html("k.html", 1, 1);
}

int pushtag(char *tagname){
	Tag *new;

	new = malloc(sizeof(Tag));
	new->parent = NULL;
	
	/* resize here */

	new->type = TT_SCOPE;
	new->child = Stack;
	Stack->parent = new;
	Stack = new;
}

int poptag(char *tagname){
	Tag *new;

	new = malloc(sizeof(Tag));
	new->parent = NULL;
	
	/* resize here */

	new->type = TT_SCOPE;
	new->child = Stack;
	Stack->parent = new;
	Stack = new;
}

char *draw_html(char *filename, int columns, int lines){
	int ch;
	int mode;

	int tagcount;
	// char tag[TAG_LEN];

	int titlecount;
	char title[TITLE_LEN];

	int listlevel;
	int list[LIST_DEPTH];

	int listitemcount;
	char listitem[LIST_LEN];

	FILE *fp;

	mode = M_PARAGRAPH;
	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("cannot open files\n");
		return NULL;
	}

	// tag[0] = 0;
	title[0] = 0;

	while (1){

	        /* 
		if ( sscanf(message, ":%s %s %[^\n]", prefix, command, params)==3){
                sscanf(prefix, "%[^!]!%[^@]@%s", nick, user, host);
        	*/

		ch = fgetc(fp);
		if (feof(fp) || ferror(fp)) break;

		if (processtags(ch)){
			printf ("[%s]", currenttag);
		}			
		if (!tag){
			if (ch != '\n' && ch != ' ') printf("%c", ch);
		}

	}
	printf("title = %s\n", title);
	fclose (fp);
	return(NULL);
}		

int processtags(char ch){
	static int tagon = 0;
	
	if (ch == '<' && !tag){
		tag = 1;
		tagon = 1;
		tagcount = 0;
		currenttag[0] = 0;
		return(0);
	}	
	
	else if (ch == '>' && tag){
		currenttag[tagcount] = 0;
		tagon = 0;
		return(0);
	}

	else if (tag && !tagon){
		tag = 0;
		return(1);
	}
	
	else if (tagon){
		currenttag[tagcount] = ch;
		tagcount++;
		return(0);
	}
	return(0);
}

void printlistitem(int listlevel, int listtype, char *listitem){
	char *listsign;
	int i;

	if (listtype == L_UNORDERED) listsign = "*";
	else if (listtype == L_ORDERED) listsign = "1.";

	for (i = 0; i<listlevel; i++){
		printf("	");
	}
	printf("%s %s\n", listsign, listitem);
}	

char *draw_html2(char *filename, int columns, int lines){
	int ch;
	int mode;

	int tagcount;
	char tag[TAG_LEN];

	int titlecount;
	char title[TITLE_LEN];

	int listlevel;
	int list[LIST_DEPTH];

	int listitemcount;
	char listitem[LIST_LEN];

	FILE *fp;

	mode = M_PARAGRAPH;
	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("cannot open files\n");
		return NULL;
	}

	tag[0] = 0;
	title[0] = 0;

	while (1){
	        /* 
		if ( sscanf(message, ":%s %s %[^\n]", prefix, command, params)==3){
                sscanf(prefix, "%[^!]!%[^@]@%s", nick, user, host);
        	*/
		ch = fgetc(fp);
		if (feof(fp) || ferror(fp)) break;

		if (ch == '<' && !(mode & M_TAG)){
			mode = (mode | M_TAG);
			tagcount = 0;
			tag[0] = 0;
		}
		
		else if (ch == '>' && (mode & M_TAG)){
			mode = (mode ^ M_TAG);
			tag[tagcount] = 0;
			/* printf ("****%s****", tag); */
			
			if (strcasecmp(tag, "title") == 0){
				mode = (mode | M_TITLE);
				titlecount = 0;
			}
			else if (strcasecmp(tag, "/title") == 0){ 
				if (mode & M_TITLE) mode = (mode ^ M_TITLE);
			}
			/* unordered list */
			else if (strcasecmp(tag, "ul") == 0){
				if (listitemcount > 0){
					printlistitem(listlevel, list[listlevel-1], listitem);
				}
				listitemcount = 0;
				if (listlevel < LIST_DEPTH) listlevel++;
				list[listlevel-1] = L_UNORDERED;
				printf("\n");
			}
			else if (strcasecmp(tag, "/ul") == 0){
				if (listlevel > 0  && list[listlevel-1] == L_UNORDERED){
					listlevel--;
					list[listlevel] = 0;
					if (listitemcount > 0){
						printlistitem(listlevel, list[listlevel-1], listitem);
					}
					listitemcount = 0;
				}
			}

			/* ordered list */
			else if (strcasecmp(tag, "ol") == 0){
				if (listitemcount > 0){
					printlistitem(listlevel, list[listlevel-1], listitem);
				}
				listitemcount = 0;
				if (listlevel < LIST_DEPTH) listlevel++;
				list[listlevel-1] = L_ORDERED;
				printf("\n");
			}
			else if (strcasecmp(tag, "/ol") == 0){
				if (listlevel > 0  && list[listlevel-1] == L_ORDERED){
					listlevel--;
					list[listlevel] = 0;
					if (listitemcount > 0){
						printlistitem(listlevel, list[listlevel-1], listitem);
					}
					listitemcount = 0;
				}
			}
			if (strcasecmp(tag, "li") == 0){
				if (listlevel > 0){
					mode = (mode | M_LISTITEM);
					if (listitemcount > 0){
						printlistitem(listlevel, list[listlevel-1], listitem);
					}
					listitemcount = 0;
				}
			}
			if (strcasecmp(tag, "/li") == 0){
				if (listlevel > 0){
					if (mode & M_LISTITEM) mode = (mode ^ M_LISTITEM);
					if (listitemcount > 0){
						printlistitem(listlevel, list[listlevel-1], listitem);
					}
					listitemcount = 0;
				}
			}
			else if (strcasecmp(tag, "br") == 0){
				printf("\n");
			}

		}	
		else {
			if (mode & M_TAG){
				tag[tagcount] = ch;
				tagcount++;
			}

			else if (mode & M_TITLE){
				title[titlecount] = ch;
				titlecount++;
				title[titlecount] = 0; 
			}

			else if (mode & M_LISTITEM){
				listitem[listitemcount] = ch;
				listitemcount++;
				listitem[listitemcount] = 0; 
			}

			else if (!mode){
				if (ch != '\n' && ch != ' ') printf("%c", ch);
			}
		}

	}
	printf("title = %s\n", title);
	fclose (fp);
	return(NULL);
}		

void printlistitem2(int listlevel, int listtype, char *listitem){
	char *listsign;
	int i;

	if (listtype == L_UNORDERED) listsign = "*";
	else if (listtype == L_ORDERED) listsign = "1.";

	for (i = 0; i<listlevel; i++){
		printf("	");
	}
	printf("%s %s\n", listsign, listitem);
}	




