/*****************************************************************************/
/*                                                                           */
/*  Copyright (C) 2004 Adrian Gonera                                         */
/*                                                                           */
/*  This file is part of Rhapsody.                                           */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  */
/*                                                                           */
/*****************************************************************************/

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
#include <curses.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "log.h"
#include "defines.h"
#include "events.h"
#include "forms.h"
#include "common.h"

static form_list *formlist = NULL;

/**** form *********************************************************************************/


form *add_form (char *name, int id, int x, int y, int width, int height, int attr, int style){
	form *new;
	form_list_entry *newentry;

	/* initalize form list */
	if (formlist == NULL){
		formlist = calloc(1, sizeof(form_list));	
        	if (formlist == NULL){
			plog("Cannot allocate form list memory in add_form(:1)\n");   
			exit(MEMALLOC_ERR);   
		}
		formlist->list = NULL;
		formlist->last = NULL;
	}

	/* now initialize form */
	new = malloc(sizeof(form));	
        if (new == NULL){
		plog("Cannot allocate screen memory in add_form(:2)\n");   
		exit(MEMALLOC_ERR);   
	}

	strncpy(new->name, name, 255);
	new->id = id;
	new->list = NULL;
	new->textlist = NULL;
	new->active = NULL;
        new->endlist = NULL;
	new->requestedx = x;
	new->requestedy = y;
	new->width = width;
	new->height = height;
	new->attr = attr;
	new->style = style;
	new->window = NULL;

	/* insert form into formlist */
	newentry = calloc(1, sizeof(form_list_entry));	
	if (newentry == NULL){
		plog("Cannot allocate form list entry memory in add_form(:3)\n");   
		exit(MEMALLOC_ERR);   
	}
	newentry->form = new;
	newentry->next = formlist->list;
	newentry->prev = NULL;
	formlist->list = newentry;
	if (formlist->last == NULL) formlist->last = newentry;

	return(new);	
}

form *remove_form(form *form){
	form_list_entry *entry;
	Fcomponent *current, *next;
	Ftextarea *currenta, *nexta;

	if (form == NULL) return(NULL);

	/* remove form from the form list */
	entry = formlist->list;
	while (entry != NULL){
		if (entry->form == form){
			if (entry->prev != NULL) (entry->prev)->next = entry->next;
			if (entry->next != NULL) (entry->next)->prev = entry->prev;
			if (entry == formlist->list) formlist->list = entry->next;
			if (entry == formlist->last) formlist->last = entry->prev;
			free(entry);
			break;
		}
		entry = entry->next;
	}

	/* now free up the momory used up by the form and close its window */
	if (form->window != NULL) delwin(form->window);

	current = form->list;
	while (current != NULL){
		next = current->next;
		remove_form_component(form, current); 	
		current = next;
	}

	currenta = form->textlist;
	while (currenta != NULL){
		nexta = currenta->next;
		remove_Ftextarea(form, currenta); 	
		currenta = nexta;
	}
        free (form);
	return(NULL);
}

int remove_all_forms(){
	int i;
	form_list_entry *current, *next;

	if (formlist == NULL) return(0);

	i = 0;
	current = formlist->list;
	while (current != NULL){
		next = current->next;
		remove_form(current->form);
		current = next;
		i++;
	}
	return(i);
}
	
void print_form(form *form){
	Fcomponent *current;
	Ftextarea *textarea;
	int active;

	if (form == NULL) return;
	curs_set(0);
	if (form->window == NULL){
		if (form->requestedx == -1) form->x = COLS/2 - (form->width)/2;
		else if (form->requestedx > COLS - (form->width)) form->x = COLS - (form->width);

		if (form->requestedy == -1) form->y = LINES/2 - (form->height)/2;
		else if (form->requestedy > LINES - (form->height)) form->y = COLS - (form->height);
		
		(form->window) = newwin(form->height, form->width, form->y, form->x);
		if (form->window == NULL){
			plog("Cannot create form window in print_form (:1)\n");   
			return;   
		}
		keypad(form->window, TRUE);
		intrflush(form->window, FALSE);
		meta(form->window, TRUE);
		wbkgdset(form->window, ' ' | form->attr);		
		werase(form->window);
		box(form->window, 0, 0);
	}
		
	if ((form->style)&(STYLE_TITLE)){
		wattrset(form->window, form->attr);
		mvwhline(form->window, 2, 1, 0, form->width - 2);
		mvwaddstr(form->window, 1, 2, form->name);
	}

	/* draw all the active components */
	current = form->list;
	while (current != NULL){
		if (current == form->active) active = 1;
		else active = 0; 
		if (current->type == F_TEXTLINE) print_Ftextline(form, (Ftextline *)(current->component), active); 	
		else if (current->type == F_LIST) print_Flist(form, (Flist *)(current->component), active); 	
		else if (current->type == F_BUTTON) print_Fbutton(form, (Fbutton *)(current->component), active); 	
		else if (current->type == F_CHECKBOX) print_Fcheckbox(form, (Fcheckbox *)(current->component), active); 	
		else if (current->type == F_CHECKBOX_ARRAY) print_Fcheckbox_array(form, 
			(Fcheckbox_array *)(current->component), active); 

		current = current->next;
	}

	/* draw passive text areas */
	textarea = form->textlist;
	while (textarea != NULL){
		print_Ftextarea(form, textarea); 	
		textarea = textarea->next;
	}

	touchwin(form->window);
	wrefresh(form->window);	
}

void redraw_form(form *form){
	Fcomponent *current;
	Ftextarea *textarea;
	int active;

	delwin(form->window);
	form->window = newwin(form->height, form->width, form->y, form->x);
	if (form->window == NULL){
		plog("Cannot create form window in print_form (:1)\n");   
		exit(MEMALLOC_ERR);   
	}

	wbkgdset(form->window, ' ' | form->attr);		
	werase(form->window);
	box(form->window, 0, 0);
	touchwin(form->window);
	wrefresh(form->window);

	current = form->list;
	while (current != NULL){
		if (current == form->active) active = 1;
		else active = 0; 
		if (current->type == F_TEXTLINE) print_Ftextline(form, (Ftextline *)(current->component), active); 	
		else if (current->type == F_LIST) print_Flist(form, (Flist *)(current->component), active); 	
		else if (current->type == F_BUTTON) print_Fbutton(form, (Fbutton *)(current->component), active); 	
		else if (current->type == F_CHECKBOX) print_Fcheckbox(form, (Fcheckbox *)(current->component), active); 	
		else if (current->type == F_CHECKBOX_ARRAY) print_Fcheckbox_array(form, (Fcheckbox_array *)(current->component), active); 
		current = current->next;
	}

	/* draw passive text areas */
	textarea = form->textlist;
	while (textarea != NULL){
		print_Ftextarea(form, textarea); 	
		textarea = textarea->next;
	}

	touchwin(form->window);
	wrefresh(form->window);	
}

int process_form_events(form *F, int event){
	int type;
	int revent;
	Fcomponent *active;

	if (F == NULL){
		return (event);   
	}
	if (event == E_NEXT ){
		if (F->active == NULL) F->active = F->list;
		else{ 
			F->active = (F->active)->next;
			if (F->active == NULL) F->active = F->list;
		}
		touchwin(F->window);
		return(E_NONE);
	}
	else if (event == 27) return(27);
	else {
		active = F->active;
		type = active->type;	
		if (type == F_TEXTLINE) revent = process_Ftextline_events((Ftextline *)(active->component), event); 	
		else if (type == F_LIST) revent = process_Flist_events((Flist *)(active->component), event); 	
		else if (type == F_BUTTON) revent = process_Fbutton_events((Fbutton *)(active->component), event); 	
		else if (type == F_CHECKBOX) revent = process_Fcheckbox_events((Fcheckbox *)(active->component), event); 	
		else if (type == F_CHECKBOX_ARRAY) revent = process_Fcheckbox_array_events((Fcheckbox_array *)(active->component), event); 	
	
		if (revent == E_NEXT){
			F->active = (F->active)->next;
                	if (F->active == NULL) F->active = F->list;
			touchwin(F->window);
			return(E_NONE);
		}
		else if (revent == E_PREV){
			F->active = (F->active)->prev;
                	if (F->active == NULL) F->active = F->endlist;
			touchwin(F->window);
			return(E_NONE);
		}
		/* special case, return the id of the component link, not the actual component */
		else if (revent == E_COMPONENT_ID){
			return(active->id);
		}
		else return(revent);
	}	
	return(event);
}


Fcomponent *add_form_component (form *form, void *component, int id, int type){
	Fcomponent *new;

	new = malloc(sizeof(Fcomponent));	
        if (new == NULL){
		plog("Cannot allocate form component memory in add_Fcomponent(:1)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->type = type;
	new->component = component;
	if (form->endlist != NULL) (form->endlist)->next = new;
	new->prev = form->endlist;
	new->next = NULL;
	new->id = id;
	
	if (form->list == NULL) form->list = new;
	form->endlist = new;
	if (form->active == NULL) form->active = new;
	return(new);
}

int remove_form_component (form *form, Fcomponent *current){
	if (current == NULL) return(0);

	if (current->type == F_TEXTLINE) remove_Ftextline((Ftextline *)(current->component)); 	
	else if (current->type == F_LIST) remove_Flist((Flist *)(current->component)); 	
	else if (current->type == F_BUTTON) remove_Fbutton((Fbutton *)(current->component)); 	
	else if (current->type == F_CHECKBOX) remove_Fcheckbox((Fcheckbox *)(current->component)); 	
	else if (current->type == F_CHECKBOX_ARRAY) remove_Fcheckbox_array((Fcheckbox_array *)(current->component)); 

	if (current->prev != NULL) (current->prev)->next = current->next;
	if (current->next != NULL) (current->next)->prev = current->prev;
	free(current);
	return(1);
}

int add_form_textarea (form *form, Ftextarea *textarea){
	textarea->next = form->textlist;
	textarea->prev = NULL;
	form->textlist = textarea;
	return(1);
}

Fcomponent *active_form_component (form *form){
	return(form->active);
}

Fcomponent *form_component_by_id (form *form, int id){
	Fcomponent *current;

	current = form->list;
	while (current != NULL){
		if (current->id == id) return(current);
		current = current->next;
	}
	return(NULL);
}

int active_form_component_id (form *form){
	return((form->active)->id);
}


/**** text area ***************************************************************************/

/*
struct form_textarea{
        char name[256];
        int x,y;
        int height, width;
        int style;
        int bgcolor, fgcolor;
	char *buffer;
        Ftext *next;
        Ftext *prev;
};
*/

Ftextarea *add_Ftextarea(char *name, int x, int y, int width, int height, int style, int attr, char *text){
	Ftextarea *new;

	new = calloc(1, sizeof(Ftextarea));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Ftextarea(:1)\n");   
		exit(MEMALLOC_ERR);   
	}

	new->buffer = malloc(strlen(text) + 1);	
        if (new->buffer == NULL){
		plog("Cannot allocate memory in add_Ftextarea(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->x = x;
	new->y = y;
	strcpy(new->name, name);
	new->width = width;
	new->height = height;
	new->style = style;	
	new->attr = attr;
	strcpy(new->buffer, text);
	return(new);	
}

int remove_Ftextarea(form *form, Ftextarea *current){
	if (current == NULL) return(0);

	if (current->prev != NULL) (current->prev)->next = current->next;
	if (current->next != NULL) (current->next)->prev = current->prev;
	if (current->buffer != NULL) free(current->buffer);
	free(current);
	return(1);
}

void print_Ftextarea(form *form, Ftextarea *area){
	int i, j, k, l, t, width;
	int lenstr;

	lenstr = strlen(area->buffer);

	/* if attribute is -1, area picks up form's attributes */
	if (area->attr == -1) wattrset(form->window, form->attr);
	else wattrset(form->window, area->attr);

	t = 0;
	for (i = area->y, k = 0; k < area->height; i++, k++){
		for (j = area->x, l = 0; l < area->width; j++, l++){
			if (t < lenstr){
				if (area->buffer[t] == '\n'){
					l = area->width;
				}
				else{
					mvwaddch(form->window, i, j, area->buffer[t]);
				}
			}
			// else mvwaddch(form->window, i, j, ' ');
			t++;
		}
	}

	//wattroff(form->window, A_BOLD);
	//wattron(form->window, A_REVERSE);
	//wattrset(form->window, COLOR_PAIR(FORM_CURSOR_COLOR_B*16+FORM_CURSOR_COLOR_F));
	touchwin(form->window);
}

void set_Ftextarea_buffer(Ftextarea *F, char *buffer){
	free(F->buffer);
	F->buffer = malloc(strlen(buffer) + 1);	
        if (F->buffer == NULL){
		plog("Cannot allocate memory in set_Ftextarea_buffer(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	strcpy(F->buffer, buffer);
}

char *Ftextarea_buffer_contents(Ftextarea *F){
	return(F->buffer);
}
	
/**** text line ***************************************************************************/

Ftextline *add_Ftextline(char *name, int tx, int ty, int nameattr, int x, int y, int size, 
	int width, int inattr, int style, int inputallow){
	Ftextline *new;

	new = malloc(sizeof(Ftextline));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Ftextline(:1)\n");   
		exit(MEMALLOC_ERR);   
	}

	new->buffer = malloc(size + 1);	
        if (new->buffer == NULL){
		plog("Cannot allocate memory in add_Ftextline(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->x = x;
	new->y = y;
	strcpy(new->name, name);
	new->namex = tx;
	new->namey = ty;
	new->buffersize = size;
	new->width = width;
	new->nameattr = nameattr;
	new->inattr = inattr;
	new->start = 0;
	new->cursorpos = 0;
	new->buffer[0] = 0;
	new->inputallow = inputallow;
	new->style = style;	
	return(new);	
}

int remove_Ftextline(Ftextline *textline){
	if (textline == NULL) return(0);
	if (textline->buffer != NULL) free(textline->buffer);
	free (textline);
	return(1);
}

void print_Ftextline(form *form, Ftextline *line, int active){
	int i, j, width;
	int strstart, linelen, lenstr;
	
	width = line->width;
	if (width > form->width - line->x) width = form->width - line->x - 1;

	strstart = line->start;
	linelen = width + line->x;
	//lenstr = strlen(line->buffer + line->start);
	lenstr = strlen(line->buffer);

	wattrset(form->window, line->nameattr);
	if (active){
		wattron(form->window, A_BOLD);
		mvwaddstr(form->window, line->namey, line->namex, line->name);
		wattroff(form->window, A_BOLD);
	}
	else mvwaddstr(form->window, line->namey, line->namex, line->name);

	wattrset(form->window, line->inattr);
	wattron(form->window, A_REVERSE);

	for (i = line->x, j = strstart; i < linelen; i++, j++){
		if ((j == line->cursorpos) && active){
			if (j < lenstr) mvwaddch(form->window, line->y, i, line->buffer[j]);
			if (line->style & STYLE_CURSOR_DARK){
				wattrset(form->window, COLOR_PAIR(F_CURSOR_COLOR_DARK_B*16+F_CURSOR_COLOR_DARK_F));
			}
			else if (line->style & STYLE_CURSOR_LIGHT){
				wattrset(form->window, COLOR_PAIR(F_CURSOR_COLOR_LIGHT_B*16+F_CURSOR_COLOR_LIGHT_F));
			}
			if (j < lenstr) mvwaddch(form->window, line->y, i, line->buffer[j]);
			else mvwaddch(form->window, line->y, i, ' ');
			wattrset(form->window, line->inattr);
			wattron(form->window, A_REVERSE);
		}
		else {
			if (j >= lenstr) mvwaddch(form->window, line->y, i, ' ');
			else mvwaddch(form->window, line->y, i, line->buffer[j]);
		}
	}

	/*
	mvwaddstr(form->window, line->y, line->x, line->buffer + line->start);
	if (active){
		if (line->cursorpos > strlen(line->buffer + line->start)) cursorchar = ' ';
		else cursorchar = line->buffer[line->start + line->cursorpos];
		wattrset(form->window, COLOR_PAIR(FORM_CURSOR_COLOR_B*16+FORM_CURSOR_COLOR_F));
		mvwaddch (form->window, line->y, line->x + line->cursorpos - line->start, cursorchar);
	}	
	*/
	touchwin(form->window);
}

void set_Ftextline_buffer(Ftextline *F, char *buffer){
	int width;

	strncpy(F->buffer, buffer, F->buffersize);
	F->buffer[F->buffersize] = 0;
	F->cursorpos = strlen(buffer);

	if (F->width < F->cursorpos) F->start = F->cursorpos - F->width; 
	
}

void backspace_Ftextline_buffer(Ftextline *F){
	if (F->cursorpos > 0){  
		F->cursorpos--;
                strcpy(&(F->buffer)[F->cursorpos], &(F->buffer)[F->cursorpos+1]);
        }
	if (F->start > F->cursorpos) F->start--; 
}

void delete_Ftextline_buffer(Ftextline *F){
	if (strlen(&(F->buffer)[F->cursorpos]) > 0){
		strcpy(&(F->buffer)[F->cursorpos], &(F->buffer)[F->cursorpos+1]);
	}
}

void add_Ftextline_buffer(Ftextline *F, int value){
	char scratch[MAXDATASIZE];

	if (strlen(F->buffer) < F->buffersize){
		strcpy(scratch, &(F->buffer)[F->cursorpos]);
	        (F->buffer)[F->cursorpos] = value;
	        strcpy(&(F->buffer)[F->cursorpos+1], scratch);
	        F->cursorpos++;
		if (F->cursorpos >= F->start + F->width) F->start++; 
	}
}

char *Ftextline_buffer_contents(Ftextline *F){
	return(F->buffer);
}
	
void move_Ftextline_cursor(Ftextline *F, int spaces){
        F->cursorpos+=spaces;
        if (F->cursorpos >= MAXDATASIZE) F->cursorpos = MAXDATASIZE-1;
       	else if (F->cursorpos < 0) F->cursorpos = 0;
        else if (F->cursorpos >= strlen(F->buffer)) F->cursorpos = strlen(F->buffer);
	if (F->cursorpos >= F->start + F->width) F->start++; 
	else if (F->start > F->cursorpos) F->start--; 
}

int process_Ftextline_events(Ftextline *T, int event){
	int in;

	if (event == KEY_LEFT){
		move_Ftextline_cursor(T, -1);
		return(E_NONE);
	}
	else if (event == KEY_RIGHT){
		move_Ftextline_cursor(T, 1);
		return(E_NONE);
	}
	else if (event == KEY_BACKSPACE || event == 0x7f || event == 8){
		backspace_Ftextline_buffer(T);
		return(E_NONE);
	}
	else if (event == KEY_DC){
		delete_Ftextline_buffer(T);
		return(E_NONE);
	}
	else if (event == KEY_UP) return(E_PREV);
	else if (event == KEY_DOWN || event == 10 || event == KEY_ENTER) return(E_NEXT);
	
	/* scan to see if this is an allowed key */ 	
	in = T->inputallow;
	
	if ((in & IN_ALPHA) && isalpha(event)) add_Ftextline_buffer(T, event);
	else if ((in & IN_NUM) && isdigit(event)) add_Ftextline_buffer(T, event);
	else if ((in & IN_ALPHANUM) && isalnum(event)) add_Ftextline_buffer(T, event);
	else if ((in & IN_PUNCT) && ispunct(event)) add_Ftextline_buffer(T, event);
	else if ((in & IN_BLANK) && isspace(event)) add_Ftextline_buffer(T, event);
	return(E_NONE);
}

/**** list ******************************************************************************/

Flist *add_Flist(char *name, int tx, int ty, int x, int y, int width, int height, int bgcolor, int fgcolor, int style){
	Flist *new;

	new = malloc(sizeof(Flist));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Flist(:1)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->x = x;
	new->y = y;
	strcpy(new->name, name);
	new->namex = tx;
	new->namey = ty;
	new->width = width;
	new->height = height;
	new->bgcolor = bgcolor;
	new->fgcolor = fgcolor;
	new->top = NULL;
	new->list = NULL;
	new->last = NULL;
	new->selected = NULL;
	return(new);	
}

int remove_Flist(Flist *list){
	Flistline *current, *next;
	if (list == NULL) return(0);
	
	current = list->list;
	while(current != NULL){
		next = current->next;
		if (current->text != NULL) free(current->text);
		free(current);
		current = next;
	}
	free(list);
	return(1);
}

void print_Flist(form *form, Flist *list, int active){
	int i, j, k, width, height;
	Flistline *current;

	width = list->width;
	if (width > form->width - list->x) width = form->width - list->x - 1;
	height = list->height;
	if (height > form->height - list->y) height = form->height - list->y - 1;

	wattroff(form->window, A_REVERSE);
	// wattrset(form->window, list->nameattr);
	if (active){
		wattron(form->window, A_BOLD);
		mvwaddstr(form->window, list->namey, list->namex, list->name);
		wattroff(form->window, A_BOLD);
	}
	else mvwaddstr(form->window, list->namey, list->namex, list->name);

	wattrset(form->window, COLOR_PAIR(list->bgcolor*16+list->fgcolor));
	

	/* clear the list area */
	for (j=list->y; (j < height + list->y); j++){
		for (i=list->x, k=0; k < width; i++, k++){
			mvwaddch(form->window, j, i, ' ');
		}
	}

	/* print all the text lines */	
	current=list->top;
	for (j=list->y; (j < height + list->y) && current != NULL; j++){
		if (current == list->selected){
			wattron(form->window, A_REVERSE);
			for (i=list->x, k=0; k < width; i++, k++){
				if (k < strlen(current->text)){
					mvwaddch(form->window, j, i, (current->text)[k]);
				}
				else mvwaddch(form->window, j, i, ' ');
			}
			for (i = strlen(current->text) + 2; i < width + 2; i++){
				mvwaddch(form->window, j, i, ' ');
			}
			wattrset(form->window, COLOR_PAIR(list->bgcolor*16+list->fgcolor));
		}

		else{
			for (i=list->x, k=0; k < width; i++, k++){
				if (k < strlen(current->text)){ 
					mvwaddch(form->window, j, i, (current->text)[k]);
				}
				else mvwaddch(form->window, j, i, ' ');
			}
		}

		current=current->next;
	}
	
}

Flistline *add_Flistline(Flist *list, int id, char *string, void *ptrid, int type){
	Flistline *new;
	char *newtext;

	new = malloc(sizeof(Flistline));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Flistline(:1)\n");   
		exit(MEMALLOC_ERR);   
	}

	newtext = malloc(strlen(string)+1);	
        if (newtext == NULL){
		plog("Cannot allocate memory in add_Flistline(:2)\n");   
		exit(MEMALLOC_ERR);   
	}

	strcpy(newtext, string);
	new->text = newtext;
	new->id = id;
	new->ptrid = ptrid;

	if (type == FORMLIST_FIRST){
		new->next = list->list;
		new->prev = NULL;
		if (list->list != NULL) list->list->prev = new;
		list->list = new;
		if (list->last == NULL) list->last = new;
		list->top = new;
		list->selected = new;
	}
	else if (type == FORMLIST_LAST){
		new->prev = list->last;
		new->next = NULL;
		if (list->last != NULL) list->last->next = new;
		list->last = new;
		if (list->list == NULL) list->list = new;
		list->top = list->list;
		list->selected = list->list;
	}
	// if (list->top == NULL) list->top = list->list;
	// if (list->selected == NULL) list->selected = new;

	return(new);	
}

int process_Flist_events(Flist *F, int event){
	int i, offset;
	Flistline *current, *topentry;
        int topnum, found;

	if (event == KEY_UP && F->selected != NULL){
	
		if (F->top == F->selected) F->top = F->top->prev;
		F->selected = F->selected->prev;

		if (F->selected == NULL) F->selected = F->list;
		if (F->top == NULL) F->top = F->list;

		return(E_NONE);
	}

	else if (event == KEY_DOWN && F->selected != NULL){

		F->selected = F->selected->next;
		if (F->selected == NULL) F->selected = F->last;

		/* count number of lines from top of list to currently selected line */
		current = F->top;
		offset = 0;
		while(current != NULL){
			if (current == F->selected) break;
			current = current->next;
			offset++;		
		}

		if (offset >= F->height) F->top = F->top->next;

		/* top is height lines away from last line */
		if (F->top == NULL){
			current = F->last;
			i = 0;
			while(current != NULL){
				if (i == F->height) break;
				current = current->prev;
				i++;		
			}
			if (current == NULL) F->top = F->list;
			else F->top = current;
		}

		current = F->list;
		while(current != NULL){
			if (current == F->top) break;
			else if (current == F->selected){
				// selected is below the last visible line, scroll down
				F->top = F->top->next;
				break;
			}
			current = current->next;
		}

		return(E_NONE);
	}

	/* if a key is pressed other than up or down, jump to next entry starting with key */
	else if (isprint(event) && F->selected != NULL){
		topnum = -1;
		i = 0;

		/* select the next channel name that starts with key */
		if (F->selected->next != NULL) current = F->selected->next;
		else current = F->list;
        
		found = 0;
		while (current != NULL){        
			if (event == current->text[0]){
				found = 1;
				F->selected = current;
				break;
			}
			current = current->next;
		}
                 
        	/* if the key is not found in the first scan, scan once more from top */
        	if (!found){    
			current = F->list;
			while (current != NULL){
				if (event == current->text[0]){
					found = 1;
					F->selected = current;
					break;
				}
				current = current->next;
			}
		}

		/* make sure that the window dimensions still display the selected entry */
		current = F->list;
		topentry = F->top;
        	if (topentry == NULL) topentry = F->list;

		while(current != NULL){
			if (current == F->top) topnum = i;
			if (topnum != -1 && i >= topnum + F->height){
				if (F->top->next != NULL) F->top = F->top->next;
				topnum++;
			}
	                if (current == F->selected){
	                        if (topnum == -1) F->top = F->selected;
	                        break;
			}
	                current = current->next;
        	        i++;
		}
		return(E_NONE);
	}
	return(event);
}

int active_list_line_id(Flist *F){
	if (F != NULL){
		if (F->selected != NULL){
			return(F->selected->id);
		}
	}
	return(-1);
}

void *active_list_line_ptrid(Flist *F){
	if (F != NULL){
		if (F->selected != NULL){
			return(F->selected->ptrid);
		}
	}
	return(NULL);
}

Flistline *active_list_line(Flist *F){
	if (F != NULL) return(F->selected);
	return(NULL);
}

char *active_list_line_text(Flist *F){
	if (F != NULL){
		if (F->selected != NULL){
			return(F->selected->text);
		}
	}
	return(NULL);
}

/**** button ***************************************************************************/

void print_Fbutton(form *form, Fbutton *button, int active){
	int i, j, width;

	width = button->width;
	if (width > form->width - button->x) width = form->width - button->x - 1;

	wattrset(form->window, COLOR_PAIR(button->bgcolor*16+button->fgcolor));
	if (active) wattron(form->window, A_REVERSE);

	// clear the button area and print the text

	for (i = button->x, j = 0; j < width; i++, j++){
		mvwaddch(form->window, button->y, i, ' ');
	}
	if ((button->style)&(STYLE_LEFT_JUSTIFY)){
		mvwaddstr(form->window, button->y, button->x, button->text); 
	}
	else if ((button->style)&(STYLE_RIGHT_JUSTIFY)){
		mvwaddstr(form->window, button->y, 
		button->x + button->width - strlen(button->text), button->text); 
	}
	else {
		mvwaddstr(form->window, button->y, 
		button->x + (button->width)/2 - strlen(button->text)/2, button->text); 
	}
}

Fbutton *add_Fbutton(char *name, int x, int y, int width, int bgcolor, int fgcolor, int id, int style){
	Fbutton *new;

	new = malloc(sizeof(Fbutton));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Fbutton(:1)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->x = x;
	new->y = y;
	new->width = width;
	new->bgcolor = bgcolor;
	new->fgcolor = fgcolor;
	new->style = style;
	new->eventid = id;

	new->text = malloc(strlen(name)+1);	
        if (new->text == NULL){
		plog("Cannot allocate memory in add_Fbutton(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	strcpy(new->text, name);
	return(new);	
}

int remove_Fbutton(Fbutton *button){
	if (button == NULL) return(0);
	
	if (button->text != NULL) free(button->text);
	free(button);
	return(1);
}

int process_Fbutton_events(Fbutton *F, int event){
	if (event == KEY_UP) return(E_PREV);
	else if (event == KEY_DOWN) return(E_NEXT);
	else if (event == KEY_LEFT) return(E_PREV);
	else if (event == KEY_RIGHT) return(E_NEXT);
	else if (event == KEY_ENTER || event == 10) return (F->eventid);
	return(event);
}


/**** checkbox ***************************************************************************/

void print_Fcheckbox(form *form, Fcheckbox *checkbox, int active){
	char lc, mc, rc;

	// print the text title

	wattrset(form->window, checkbox->attrib);
	wattroff(form->window, A_REVERSE);

	if (active) wattron(form->window, A_BOLD);
	mvwaddstr(form->window, checkbox->texty, checkbox->textx, checkbox->text); 

	if ((checkbox->style)&(STYLE_CHECKBOX_ROUND)){
		lc = '(';
		rc = ')';
	}		
	else if ((checkbox->style)&(STYLE_CHECKBOX_TRIANGLE)){
		lc = '<';
		rc = '>';
	}
	else{
		lc = '[';
		rc = ']';
	}
	
	if (checkbox->selected){	
		if ((checkbox->style)&(STYLE_CHECKBOX_STAR)){
			mc = '*';
		}		
		else{
			mc = 'X';
		}		
	}
	else mc = ' ';
	
	mvwaddch(form->window, checkbox->y, checkbox->x-1, lc); 
	mvwaddch(form->window, checkbox->y, checkbox->x, mc); 
	mvwaddch(form->window, checkbox->y, checkbox->x+1, rc); 
	wattroff(form->window, A_BOLD);

}


Fcheckbox *add_Fcheckbox(char *name, int x, int y, int textx, int texty, int attrib, int style){
	Fcheckbox *new;

	new = malloc(sizeof(Fcheckbox));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Fcheckbox(:1)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->x = x;
	new->y = y;
	new->textx = textx;
	new->texty = texty;
	new->attrib = attrib;
	new->style = style;
	new->selected = 0;

	new->text = malloc(strlen(name)+1);	
        if (new->text == NULL){
		plog("Cannot allocate memory in add_Fcheckbox(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	strcpy(new->text, name);
	return(new);
}

int remove_Fcheckbox(Fcheckbox *checkbox){
	if (checkbox == NULL) return(0);
	
	if (checkbox->text != NULL) free(checkbox->text);
	free(checkbox);
	return(1);
}

int process_Fcheckbox_events(Fcheckbox *B, int event){
	if (event == KEY_UP) return(E_PREV);
	else if (event == KEY_DOWN) return(E_NEXT);
	else if (event == KEY_LEFT) return(E_PREV);
	else if (event == KEY_RIGHT) return(E_NEXT);
	else if (event == KEY_ENTER || event == 10 || event == ' '){
		B->selected = B->selected ^ 1;
		return (E_NONE);
	}
	return(event);
}

int Fcheckbox_value(Fcheckbox *B){
	if (B != NULL) return(B->selected);
	else return(0);
}

void set_Fcheckbox_value(Fcheckbox *B, int value){
	if (B == NULL) return;
	B->selected = value;
}

/**** radio buttons (checkbox array) ***************************************************/

void print_Fcheckbox_array(form *form, Fcheckbox_array *checkboxes, int active){
	char lc, mc, rc;
	Fcheckbox_array_box *current;

	// print the text title

	wattrset(form->window, checkboxes->attrib);
	wattroff(form->window, A_REVERSE);

	if (active) wattron(form->window, A_BOLD);
	mvwaddstr(form->window, checkboxes->texty, checkboxes->textx, checkboxes->text); 
	wattroff(form->window, A_BOLD);

	current = checkboxes->list;
	while (current != NULL){
		// checkboxes in an array are either all or none active
		print_Fcheckbox(form, current->checkbox, active);
		current = current->next;
	}
}

Fcheckbox_array *add_Fcheckbox_array(char *name, int textx, int texty, int attrib, int style){
	Fcheckbox_array *new;

	new = malloc(sizeof(Fcheckbox_array));	
	if (new == NULL){
		plog("Cannot allocate memory in add_Fcheckbox_array(:1)\n");   
		exit(MEMALLOC_ERR);   
	}
	new->textx = textx;
	new->texty = texty;
	new->attrib = attrib;
	new->style = style;
	new->selected = NULL;
	new->list = NULL;
	new->last = NULL;

	new->text = malloc(strlen(name)+1);	
        if (new->text == NULL){
		plog("Cannot allocate memory in add_Fcheckbox_array(:2)\n");   
		exit(MEMALLOC_ERR);   
	}
	strcpy(new->text, name);
	return(new);
}

Fcheckbox_array_box *add_Fcheckbox_to_array(Fcheckbox_array *array, int id, Fcheckbox *box){
	Fcheckbox_array_box *new;

	new = malloc(sizeof(Fcheckbox_array_box));	
        if (new == NULL){
		plog("Cannot allocate memory in add_Fcheckbox_array_box(:1)\n");   
		exit(MEMALLOC_ERR);   
	}

	new->id = id;
	new->checkbox = box;
	new->prev = array->last;
	new->next = NULL;

	if (array->last != NULL) (array->last)->next = new;
	array->last = new;
	if (array->list == NULL) array->list = new;
	
	if (array->selected == NULL){
		array->selected = new;
		set_Fcheckbox_value((array->selected)->checkbox, 1);
	}

	return(new);	
}

int remove_Fcheckbox_array(Fcheckbox_array *array){
	Fcheckbox_array_box *current, *next;

	if (array == NULL) return(0);
	current = array->list;
	while (current != NULL){
		next = current->next;
		if (current->checkbox != NULL) remove_Fcheckbox(current->checkbox);
		free(current);
		current = next;
	} 	
	free(array);
	return(1);
}

int process_Fcheckbox_array_events(Fcheckbox_array *B, int event){
	Fcheckbox_array_box *current;
	if (event == KEY_UP || event == KEY_LEFT) {
		if (B->list != NULL){
			set_Fcheckbox_value(B->selected->checkbox, 0);
			if (B->selected->prev != NULL) B->selected = (B->selected)->prev;
			else B->selected = B->last;				
			set_Fcheckbox_value((B->selected)->checkbox, 1);
		}
	}
	else if (event == KEY_DOWN || event == KEY_RIGHT) {
		if (B->list != NULL){
			set_Fcheckbox_value((B->selected)->checkbox, 0);
			if (B->selected->next != NULL) B->selected = (B->selected)->next;
			else B->selected = B->list;
			set_Fcheckbox_value((B->selected)->checkbox, 1);
		} 
	}
	
	return(event);
}

int Fcheckbox_array_selected_id(Fcheckbox_array *B){
	Fcheckbox_array_box *current;
	
	current = B->list;
	while (current != NULL){
		if (current == B->selected) return(current->id);
		current = current->next;
	} 
	return(-1);
}

int Fcheckbox_array_select(Fcheckbox_array *B, Fcheckbox_array_box *selected){
	Fcheckbox_array_box *current;
	
	current = B->list;
	while (current != NULL){
		if (current == selected){
			B->selected = selected;
			set_Fcheckbox_value(current->checkbox, 1);
		}
		else set_Fcheckbox_value(current->checkbox, 0);
		current = current->next;

	} 
	return(-1);
}

int Fcheckbox_array_select_by_id(Fcheckbox_array *B, int id){
	Fcheckbox_array_box *current;

	current = B->list;
	while (current != NULL){
		if (current->id == id){
			B->selected = current;
			set_Fcheckbox_value(current->checkbox, 1);
		}
		else set_Fcheckbox_value(current->checkbox, 0);

		current = current->next;
	} 
	return(-1);
}

/**** progress bar *******************************************************************/


/*
void progress_bar(WINDOW *win, int posy, int posx, int size, int percent){
        int to_fill; 
        int i;
 
        to_fill = (size*percent)/100;
        wattrset(win, COLOR_PAIR(PROGRESS_COLOR_F));
        wattron(win, A_REVERSE);
        wattron(win, A_BOLD);
        for (i=0; i<size; i++){
                if (i==to_fill+1){
                        if (has_colors){
                                wattrset(win, COLOR_PAIR(PROGRESS_COLOR_B));
                                wattron(win, A_REVERSE);
                        }
                        else {
                                wattrset(win, A_NORMAL);
                        }
                }
                mvwaddch(win, posy, posx+i, ' ');
        }
        wattrset(win, A_NORMAL);
}


*/

