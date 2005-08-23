/*****************************************************************************/
/*                                                                           */
/*  Copyright (C) 2005 Adrian Gonera                                         */
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
#include <string.h>
#include <strings.h>
#include <time.h>
#include <ctype.h>

#include "autodefs.h"
#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "log.h"
#include "ncolor.h"
#include "common.h"
#include "config.h"
#include "screen.h"


int read_config(char *config_file, config *C){
	char line[MAXLINELEN];
	char scope[MAXSCOPELEN];
	char attrib[MAXLINELEN];
	char value[MAXLINELEN];
	char value2[MAXLINELEN];
	FILE *fp;
	int conv;
	int fg, bg;

	// init the config struct first

	if (C == NULL) return (-1);

	C->nick[0] = 0;
	C->alt_nick[0] = 0;
	C->user[0] = 0;
	C->hostname[0] = 0;
	C->domain[0] = 0;
	C->userdesc[0] = 0;
	C->dcculpath[0] = 0;
	C->dccdlpath[0] = 0;
	C->serverfavorite = NULL;
	C->channelfavorite = NULL;
	C->ctcpfinger[0] = 0;
	C->ctcpuserinfo[0] = 0;
	C->timestampformat[0] = 0;

	/* init the default color scheme */
	C->menu_color_fg = DEFAULT_MENU_COLOR_F;
	C->menu_color_bg = DEFAULT_MENU_COLOR_B;

	C->form_color_fg = DEFAULT_FORM_COLOR_F;
	C->form_color_bg = DEFAULT_FORM_COLOR_B;
	C->formfield_color_fg = DEFAULT_FORMFIELD_COLOR_F;
	C->formfield_color_bg = DEFAULT_FORMFIELD_COLOR_B;
	C->formbutton_color_fg = DEFAULT_FORMBUTTON_COLOR_F;
	C->formbutton_color_bg = DEFAULT_FORMBUTTON_COLOR_B;

	C->win_color_fg = DEFAULT_WIN_COLOR_F;
	C->win_color_bg = DEFAULT_WIN_COLOR_B;
	
	C->message_color = DEFAULT_MESSAGE_COLOR;
	C->error_color = DEFAULT_ERROR_COLOR;
	C->notice_color = DEFAULT_NOTICE_COLOR;
	C->ctcp_color = DEFAULT_CTCP_COLOR;
	C->dcc_color = DEFAULT_DCC_COLOR;
	C->join_color = DEFAULT_JOIN_COLOR;
	C->rename_color = DEFAULT_RENAME_COLOR;
	C->kick_color = DEFAULT_KICK_COLOR;
	C->mode_color = DEFAULT_MODE_COLOR;
	C->invite_color = DEFAULT_INVITE_COLOR;
	
	assume_default_colors(C->win_color_fg, C->win_color_bg);
	
	fp = fopen(config_file, "r");

	if (fp != NULL){
		strcpy(scope, "");
		while(1){
			bzero(line, MAXLINELEN);
			if(ferror(fp)){
				fprintf(stderr, "Error reading config file\n");
				return(-1);
			}
			fgets(line, MAXLINELEN-1, fp);
			
			// now do all the parsing of the config file if not a comment line
			if (line[0] != '#'){
				// if not within a scope
				if (strcmp(scope, "") == 0){
					conv = sscanf(line, "%[^= ] = %[^\n]", attrib, value);
					// printf ("%d, %s, %s, %s\n", conv, attrib, value, line);
					if (conv == 2){
						if (strncasecmp(attrib, "NICK", MAXNICKLEN) == 0){
							strcpy(C->nick, value);
						}
						else if (strncasecmp(attrib, "ALT_NICK", MAXNICKLEN) == 0){
							strcpy(C->alt_nick, value);
						}
						else if (strncasecmp(attrib, "USER", MAXNICKLEN) == 0){
							strcpy(C->user, value);
						}
						else if (strncasecmp(attrib, "HOST", MAXHOSTLEN) == 0){
							strcpy(C->hostname, value);
						}
						else if (strncasecmp(attrib, "DOMAIN", MAXDOMLEN) == 0){
							strcpy(C->domain, value);
						}
						else if (strncasecmp(attrib, "NAME", MAXDESCLEN) == 0){
							strcpy(C->userdesc, value);
						}
						else if (strncasecmp(attrib, "MODE", MAXDESCLEN) == 0){
							strcpy(C->mode, value);
						}
						else if (strncasecmp(attrib, "DCCDOWNLOADPATH", MAXDESCLEN) == 0){
							strcpy(C->dccdlpath, value);
						}
						else if (strncasecmp(attrib, "DCCUPLOADPATH", MAXDESCLEN) == 0){
							strcpy(C->dcculpath, value);
						}
						else if (strncasecmp(attrib, "DCCBLOCKSIZE", MAXDESCLEN) == 0){
							C->dccblocksize = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCACCEPT", MAXDESCLEN) == 0){
							C->dccaccept = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCDUPLICATES", MAXDESCLEN) == 0){
							C->dccduplicates = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCHOSTNAME", MAXDESCLEN) == 0){
							strcpy(C->dcchostname, value);
						}
						else if (strncasecmp(attrib, "DCCSTARTPORT", MAXDESCLEN) == 0){
							C->dccstartport = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCENDPORT", MAXDESCLEN) == 0){
							C->dccendport = atoi(value);
						}

						else if (strncasecmp(attrib, "CTCPREPLY", MAXDESCLEN) == 0){
							C->ctcpreply = atoi(value);
						}
						else if (strncasecmp(attrib, "CTCPTHROTTLE", MAXDESCLEN) == 0){
							C->ctcpthrottle = atoi(value);
						}
						else if (strncasecmp(attrib, "CTCPFINGER", MAXDESCLEN) == 0){
							strcpy(C->ctcpfinger, value);
						}
						else if (strncasecmp(attrib, "CTCPUSERINFO", MAXDESCLEN) == 0){
							strcpy(C->ctcpuserinfo, value);
						}

						/* timestamps */
						else if (strncasecmp(attrib, "TIMESTAMPFORMAT", MAXTIMELEN) == 0){
							strcpy(C->timestampformat, value);
						}
						else if (strncasecmp(attrib, "CHANNELTIMESTAMPS", MAXDESCLEN) == 0){
							C->channeltimestamps = atoi(value);
						}
						else if (strncasecmp(attrib, "CHATTIMESTAMPS", MAXDESCLEN) == 0){
							C->chattimestamps = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCTIMESTAMPS", MAXDESCLEN) == 0){
							C->dcctimestamps = atoi(value);
						}


						/* misc settings */
						else if (strncasecmp(attrib, "AUTOSAVE", MAXDESCLEN) == 0){
							C->autosave = atoi(value);
						}
						else if (strncasecmp(attrib, "CONNECTTIMEOUT", MAXDESCLEN) == 0){
							C->connecttimeout = atoi(value);
						}

						/* color settings */
						else if (strncasecmp(attrib, "SCREENCOLOR", MAXDESCLEN) == 0){
							sscanf(value, "%d,%d", &fg, &bg);
							C->win_color_fg = fg;
							C->win_color_bg = bg;
						}
						else if (strncasecmp(attrib, "MENUCOLOR", MAXDESCLEN) == 0){
							sscanf(value, "%d,%d", &fg, &bg);
							C->menu_color_fg = fg;
							C->menu_color_bg = bg;
						}
						else if (strncasecmp(attrib, "FORMCOLOR", MAXDESCLEN) == 0){
							sscanf(value, "%d,%d", &fg, &bg);
							C->form_color_fg = fg;
							C->form_color_bg = bg;
						}
						else if (strncasecmp(attrib, "FORMFIELDCOLOR", MAXDESCLEN) == 0){
							sscanf(value, "%d,%d", &fg, &bg);
							C->formfield_color_fg = fg;
							C->formfield_color_bg = bg;
						}
						else if (strncasecmp(attrib, "FORMBUTTONCOLOR", MAXDESCLEN) == 0){
							sscanf(value, "%d,%d", &fg, &bg);
							C->formbutton_color_fg = fg;
							C->formbutton_color_bg = bg;
						}
						else if (strncasecmp(attrib, "MESSAGECOLOR", MAXDESCLEN) == 0){
							C->message_color = atoi(value);
						}
						else if (strncasecmp(attrib, "ERRORCOLOR", MAXDESCLEN) == 0){
							C->error_color = atoi(value);
						}
						else if (strncasecmp(attrib, "NOTICECOLOR", MAXDESCLEN) == 0){
							C->notice_color = atoi(value);
						}
						else if (strncasecmp(attrib, "CTCPCOLOR", MAXDESCLEN) == 0){
							C->ctcp_color = atoi(value);
						}
						else if (strncasecmp(attrib, "DCCCOLOR", MAXDESCLEN) == 0){
							C->dcc_color = atoi(value);
						}
						else if (strncasecmp(attrib, "JOINCOLOR", MAXDESCLEN) == 0){
							C->join_color = atoi(value);
						}
						else if (strncasecmp(attrib, "RENAMECOLOR", MAXDESCLEN) == 0){
							C->rename_color = atoi(value);
						}
						else if (strncasecmp(attrib, "KICKCOLOR", MAXDESCLEN) == 0){
							C->kick_color = atoi(value);
						}
						else if (strncasecmp(attrib, "MODECOLOR", MAXDESCLEN) == 0){
							C->mode_color = atoi(value);
						}
						else if (strncasecmp(attrib, "INVITECOLOR", MAXDESCLEN) == 0){
							C->invite_color = atoi(value);
						}
					}
					conv = sscanf(line, "%[^{ ] %[{]", attrib, value);
					//printf ("%d, %s, %s, %s\n", conv, attrib, value, line);
					if (conv == 2) strcpy(scope, attrib);
				}
				else if (strcasecmp(scope, "SERVER") == 0){
					conv = sscanf(line, "%s %s %s", attrib, value, value2);
					//printf ("%d, %s, %s, %s\n", conv, attrib, value, line);
					if (conv >= 2) add_config_server(C, attrib, atoi(value), value2, LIST_ORDER_BACK);
					else if (conv == 1 && strcmp(attrib, "}") == 0) strcpy(scope, ""); 
				}
				else if (strcasecmp(scope, "CHANNEL") == 0){
					conv = sscanf(line, "%s", attrib);
					if (conv == 1 && strcmp(attrib, "}") == 0) strcpy(scope, ""); 
					else if (conv == 1) add_config_channel(C, attrib, LIST_ORDER_BACK);
				}
				else if (strcasecmp(scope, "NICK") == 0){
					conv = sscanf(line, "%s", attrib);
					if (conv == 1 && strcmp(attrib, "}") == 0) strcpy(scope, ""); 
					else if (conv == 1) add_config_user(C, CONFIG_FAVORITE_USER_LIST, attrib, LIST_ORDER_BACK);
				}
				else if (strcasecmp(scope, "IGNORE") == 0){
					conv = sscanf(line, "%s", attrib);
					if (conv == 1 && strcmp(attrib, "}") == 0) strcpy(scope, ""); 
					else if (conv == 1) add_config_user(C, CONFIG_IGNORED_USER_LIST, attrib, LIST_ORDER_BACK);
				}
			}
			if (feof(fp)) break;
		}
		return(0);
	}

	/* if the config file could not be opened, set some defaults */
	else{
		strcpy(C->nick, loginuser);
		strcpy(C->alt_nick, DEFAULT_ALTNICKNAME);
		strcpy(C->user, loginuser);
		strcpy(C->hostname, hostname);
		strcpy(C->domain, domain);
		strcpy(C->userdesc, loginuser);
		strcpy(C->dcculpath, homepath);
		strcpy(C->dccdlpath, homepath);
		strcpy(C->dcchostname, hostname);
		strcpy(C->timestampformat, DEFAULT_TIMESTAMP);

		C->dccstartport = DEFAULT_DCCSTARTPORT;
		C->dccendport = DEFAULT_DCCENDPORT;
		C->dccblocksize = DEFAULT_DCCBLOCKSIZE;
		C->autosave = DEFAULT_AUTOSAVE;
		C->connecttimeout = CONNECTTIMEOUT;

		C->ctcpreply = DEFAULT_CTCPREPLY;
		C->ctcpthrottle = DEFAULT_CTCPTHROTTLE;

		/* if blank, set some parameters to default values */

		if (strlen(loginuser) == 0){
			strcpy(C->nick, DEFAULT_NICKNAME);
			strcpy(C->user, DEFAULT_NICKNAME);
			strcpy(C->userdesc, DEFAULT_USERDESC);
		}
		if (strlen(hostname) == 0){
			strcpy(C->hostname, DEFAULT_HOSTNAME);
			strcpy(C->dcchostname, DEFAULT_HOSTNAME);
		}
		if (strlen(domain) == 0){
			strcpy(C->domain, DEFAULT_DOMAIN);
		}
		if (strlen(homepath) == 0){
			strcpy(C->dcculpath, DEFAULT_HOMEPATH);
			strcpy(C->dccdlpath, DEFAULT_HOMEPATH);
		}	
		return(-1);
	}
	return(-1);
}
	
/** servers ************************************************************************/

int add_config_server(config *C, char *name, unsigned int port, char *password, int order){
	config_server *new;

	new = malloc(sizeof(config_server));
	if (new == NULL){
		plog("Couldn't allocate sever list memory in add_config_server()");
		return(-1);
	}
	strncpy(new->name, name, MAXSERVERLEN-1);
	new->port = port;
	strncpy(new->password, password, MAXPASSLEN-1);
	
	
	if (order == LIST_ORDER_FRONT){
		if (C->serverfavorite == NULL) new->next = NULL;
		else{
			(C->serverfavorite)->prev = new;
			new->next = C->serverfavorite;
		}
		new->prev = NULL;
		C->serverfavorite = new;
		if (C->lastserverfavorite == NULL) C->lastserverfavorite = new;
	}
	else if (order == LIST_ORDER_BACK){
		if (C->lastserverfavorite == NULL) new->prev = NULL;
		else{
			(C->lastserverfavorite)->next = new;
			new->prev = C->lastserverfavorite;
		}
		new->next = NULL;
		C->lastserverfavorite = new;
		if (C->serverfavorite == NULL) C->serverfavorite = new;
	}
	return(0);
}

int remove_config_server(config *C, config_server *S){
	config_server *prev;
	config_server *next;

	if (S != NULL){
		prev = S->prev;
		next = S->next;

		if (prev != NULL) prev->next = next;
		if (next != NULL) next->prev = prev;

		if (C->serverfavorite == S) C->serverfavorite = next;
		if (C->lastserverfavorite == S) C->lastserverfavorite = prev;
		free (S);
		return(1);
	}
	return(0);
}

int config_server_exists(config *C, char *name, unsigned int port){
	config_server *current;
	
	current = C->serverfavorite;
	while(current != NULL){
		if (strcmp(current->name, name) == 0 && current->port == port){
			return(1);
		}
		current = current->next;
	}
	return(0);
}


/** channel ************************************************************************/

int add_config_channel(config *C, char *name, int order){
	config_channel *new;

	new = malloc(sizeof(config_channel));
	if (new == NULL){
		plog("Couldn't allocate channel list memory in add_config_channel()");
		return(-1);
	}
	strncpy(new->name, name, MAXCHANNELLEN-1);

	if (order == LIST_ORDER_FRONT){
		if (C->channelfavorite == NULL) new->next = NULL;
		else{
			(C->channelfavorite)->prev = new;
			new->next = C->channelfavorite;
		}
		C->channelfavorite = new;
		new->prev = NULL;
		if (C->lastchannelfavorite == NULL) C->lastchannelfavorite = new;
	}

	else if (order == LIST_ORDER_BACK){
		if (C->lastchannelfavorite == NULL) new->prev = NULL;
		else{
			(C->lastchannelfavorite)->next = new;
			new->prev = C->lastchannelfavorite;
		}
		C->lastchannelfavorite = new;
		new->next = NULL;
		if (C->channelfavorite == NULL) C->channelfavorite = new;
	}

	return(0);
}

int remove_config_channel(config *C, config_channel *S){
	config_channel *prev;
	config_channel *next;

	if (S != NULL){
		prev = S->prev;
		next = S->next;

		if (prev != NULL) prev->next = next;
		if (next != NULL) next->prev = prev;

		if (C->channelfavorite == S) C->channelfavorite = next;
		if (C->lastchannelfavorite == S) C->lastchannelfavorite = prev;
		free (S);
		return(1);
	}
	return(0);
}

int config_channel_exists(config *C, char *name){
	config_channel *current;
	
	current = C->channelfavorite;
	while(current != NULL){
		if (strcmp(current->name, name) == 0){
			return(1);
		}
		current = current->next;
	}
	return(0);
}


/** user ************************************************************************/

int add_config_user(config *C, int listnum, char *name, int order){
	config_user *new;

	new = malloc(sizeof(config_user));
	if (new == NULL){
		plog("Couldn't allocate channel list memory in add_config_user()");
		return(-1);
	}
	strncpy(new->name, name, MAXNICKLEN-1);

	if (listnum == CONFIG_FAVORITE_USER_LIST){	
		if (order == LIST_ORDER_FRONT){
			if (C->userfavorite == NULL) new->next = NULL;
			else{
				(C->userfavorite)->prev = new;
				new->next = C->userfavorite;
			}
			C->userfavorite = new;
			new->prev = NULL;
			if (C->lastuserfavorite == NULL) C->lastuserfavorite = new;
		}
		else if (order == LIST_ORDER_BACK){
			if (C->lastuserfavorite == NULL) new->prev = NULL;
			else{
				(C->lastuserfavorite)->next = new;
				new->prev = C->lastuserfavorite;
			}
			C->lastuserfavorite = new;
			new->next = NULL;
			if (C->userfavorite == NULL) C->userfavorite = new;
		}
	}
	else{
		if (order == LIST_ORDER_FRONT){
			if (C->userignored == NULL) new->next = NULL;
			else{
				(C->userignored)->prev = new;
				new->next = C->userignored;
			}
			C->userignored = new;
			new->prev = NULL;
			if (C->lastuserignored == NULL) C->lastuserignored = new;
		}
		else if (order == LIST_ORDER_BACK){
			if (C->lastuserignored == NULL) new->prev = NULL;
			else{
				(C->lastuserignored)->next = new;
				new->prev = C->lastuserignored;
			}
			C->lastuserignored = new;
			new->next = NULL;
			if (C->userignored == NULL) C->userignored = new;
		}
	}
	return(0);
}

int remove_config_user(config *C, int listnum, config_user *U){
	config_user *prev, *next;

	if (U != NULL){
		prev = U->prev;
		next = U->next;

		if (prev != NULL) prev->next = next;
		if (next != NULL) next->prev = prev;
		
		if (listnum == CONFIG_FAVORITE_USER_LIST && C->userfavorite == U) C->userfavorite = next;
		else if (C->userignored == U) C->userignored = next;
		free (U);
		return(1);
	}
	return(0);
}

int remove_config_user_by_name(config *C, int listnum, char *name){
	return(remove_config_user(C, listnum, config_user_exact(C, listnum, name)));
}

int config_user_exists(config *C, int listnum, char *name){
	config_user *current;
	
	if (listnum == CONFIG_FAVORITE_USER_LIST) current = C->userfavorite;
	else current = C->userignored;

	while(current != NULL){
		if (string_match(name, current->name)){
			return(1);
		}
		current = current->next;
	}
	return(0);
}

int config_user_exists_exact(config *C, int listnum, char *name){
	config_user *current;
	
	if (listnum == CONFIG_FAVORITE_USER_LIST) current = C->userfavorite;
	else current = C->userignored;

	while(current != NULL){
		if (strcmp(name, current->name) == 0){
			return(1);
		}
		current = current->next;
	}
	return(0);
}

int string_match(char *str, char *exp){
	int i;

	// vprint_all("Strings \"%s\" -> \"%s\"\n",str, exp);
	// vprint_all("Chars   '%c' -> '%c'\n",str[0], exp[0]);
	if (exp[0] == '*'){
		if (strlen(exp) == 1) return(1);    			
		for (i=0; i<strlen(str); i++){
			if (string_match(&str[i], &exp[1]) == 1) return(1);
		}
		return(0);
	}
	else if (exp[0] == '?' || toupper(exp[0]) == toupper(str[0])){
		if (strlen(exp) == 1 && strlen(str) == 1) return(1);
		return(string_match(&str[1], &exp[1]));
	}
	else return(0);
}

config_user *config_user_exact(config *C, int listnum, char *name){
	config_user *current;
	
	if (listnum == CONFIG_FAVORITE_USER_LIST) current = C->userfavorite;
	else current = C->userignored;

	while(current != NULL){
		if (strcmp(name, current->name) == 0){
			return(current);
		}
		current = current->next;
	}
	return(NULL);
}

/** file ops *************************************************************************/

server *new_server_config_connect(config *C){
	server *S;
	config_server *A;

	A = (C->serverfavorite);
	if (C == NULL) return (NULL);
	S = add_server(A->name, A->port, C->nick, C->user, C->hostname, C->domain, C->userdesc);
	return (S);
}

server *new_server_config(config *C){
	server *S;
	config_server *A;

	A = (C->serverfavorite);
	if (C == NULL) return (NULL);
	S = add_server("", 0, C->nick, C->user, C->hostname, C->domain, C->userdesc);
	return (S);
}

int writeconfig(char *config_file, config *C){
	FILE *fp;
	config_server *curr_server;
	config_channel *curr_channel;
	config_user *curr_user;

	C->changed = 0;

	fp = fopen(config_file, "w");
	if (fp == NULL) return(0);
		

	fprintf(fp, "###############################################################################\n");
	fprintf(fp, "#                                                                             #\n");
	fprintf(fp, "# %-75s #\n", "Configuration file created by "CODE_ID);
	fprintf(fp, "#                                                                             #\n");
	fprintf(fp, "###############################################################################\n");
	fprintf(fp, "\n");
	fprintf(fp, "# Do not put comments in this file. This file is automatically generated when\n");
	fprintf(fp, "# you save your configuration.\n"); 
	fprintf(fp, "\n");
	fprintf(fp, "# The config syntax consists of a attribute = value pair for most configuration\n");
	fprintf(fp, "# options. Attributes that may have several values, use the following\n");
	fprintf(fp, "# definition:\n");
	fprintf(fp, "# attribute {\n");
	fprintf(fp, "#       value 1\n");
	fprintf(fp, "#       value 2\n");
	fprintf(fp, "#       ...\n");
	fprintf(fp, "# }\n");
	fprintf(fp, "\n");
	fprintf(fp, "nick = %s\n", C->nick);
	fprintf(fp, "alt_nick = %s\n", C->alt_nick);
	fprintf(fp, "name = %s\n", C->userdesc);
	fprintf(fp, "user = %s\n", C->user);
	fprintf(fp, "host = %s\n", C->hostname);
	fprintf(fp, "domain = %s\n", C->domain);
	fprintf(fp, "mode = %s\n", C->mode);
	fprintf(fp, "\n");


	fprintf(fp, "# Server favorites are contained within curly brackets and have the following syntax:\n");
	fprintf(fp, "# server {\n");
	fprintf(fp, "#       server1 port1\n");
	fprintf(fp, "#       server2 port2\n");
	fprintf(fp, "# }\n");
	fprintf(fp, "\n");
	fprintf(fp, "server {\n");
	curr_server = C->serverfavorite;
	while(curr_server != NULL){
		fprintf(fp, "	%s %d %s\n", curr_server->name, curr_server->port, curr_server->password);
		curr_server = curr_server->next;
	}
	fprintf(fp, "}\n");
	fprintf(fp, "\n");


	fprintf(fp, "# Channel favorite list is also contained within curly brackets.\n");
	fprintf(fp, "channel {\n");
	curr_channel = C->channelfavorite;
	while(curr_channel != NULL){
		fprintf(fp, "	%s\n", curr_channel->name);
		curr_channel = curr_channel->next;
	}
	fprintf(fp, "}\n");
	fprintf(fp, "\n");


	fprintf(fp, "# Favorite user list is contained within curly brackets as well.\n");
	fprintf(fp, "nick {\n");
	curr_user = C->userfavorite;
	while(curr_user != NULL){
		fprintf(fp, "	%s\n", curr_user->name);
		curr_user = curr_user->next;
	}
	fprintf(fp, "}\n");
	fprintf(fp, "\n");

	fprintf(fp, "# User ignore list is contained within curly brackets as well.\n");
	fprintf(fp, "ignore {\n");
	curr_user = C->userignored;
	while(curr_user != NULL){
		fprintf(fp, "	%s\n", curr_user->name);
		curr_user = curr_user->next;
	}
	fprintf(fp, "}\n");
	fprintf(fp, "\n");

	fprintf(fp, "# DCC Options\n");
	fprintf(fp, "\n");
	fprintf(fp, "dcchostname = %s\n", C->dcchostname);
	fprintf(fp, "dccstartport = %d\n", C->dccstartport);
	fprintf(fp, "dccendport = %d\n", C->dccendport);
	fprintf(fp, "\n");
	fprintf(fp, "dccdownloadpath = %s\n", C->dccdlpath);
	fprintf(fp, "dccuploadpath = %s\n", C->dcculpath);
	fprintf(fp, "dccblocksize = %d\n", C->dccblocksize);
	fprintf(fp, "dccaccept = %d\n", C->dccaccept);
	fprintf(fp, "dccduplicates = %d\n", C->dccduplicates);
	fprintf(fp, "\n");

	fprintf(fp, "# CTCP Options\n");
	fprintf(fp, "\n");
	fprintf(fp, "ctcpreply = %d\n", C->ctcpreply);
	fprintf(fp, "ctcpthrottle = %d\n", C->ctcpthrottle);
	fprintf(fp, "ctcpfinger = %s\n", C->ctcpfinger);
	fprintf(fp, "ctcpuserinfo = %s\n", C->ctcpuserinfo);
	fprintf(fp, "\n");

	fprintf(fp, "# Misc Options\n");
	fprintf(fp, "\n");
	fprintf(fp, "autosave = %d\n", C->autosave);
	fprintf(fp, "connecttimeout = %d\n", C->connecttimeout);
	fprintf(fp, "timestampformat = %s\n", C->timestampformat);
	fprintf(fp, "channeltimestamps = %d\n", C->channeltimestamps);
	fprintf(fp, "chattimestamps = %d\n", C->chattimestamps);
	fprintf(fp, "dcctimestamps = %d\n", C->dcctimestamps);
	fprintf(fp, "\n");

	fprintf(fp, "# Color Settings\n");
	fprintf(fp, "\n");
	fprintf(fp, "screencolor = %d,%d\n", C->win_color_fg, C->win_color_bg);
	fprintf(fp, "menucolor = %d,%d\n", C->menu_color_fg, C->menu_color_bg);
	fprintf(fp, "formcolor = %d,%d\n", C->form_color_fg, C->form_color_bg);
	fprintf(fp, "formfieldcolor = %d,%d\n", C->formfield_color_fg, C->formfield_color_bg);
	fprintf(fp, "formbuttoncolor = %d,%d\n", C->formbutton_color_fg, C->formbutton_color_bg);
	fprintf(fp, "\n");
	fprintf(fp, "messagecolor = %d\n", C->message_color);
	fprintf(fp, "errorcolor = %d\n", C->error_color);
	fprintf(fp, "noticecolor = %d\n", C->notice_color);
	fprintf(fp, "ctcpcolor = %d\n", C->ctcp_color);
	fprintf(fp, "dcccolor = %d\n", C->dcc_color);
	fprintf(fp, "joincolor = %d\n", C->join_color);
	fprintf(fp, "renamecolor = %d\n", C->rename_color);
	fprintf(fp, "modecolor = %d\n", C->mode_color);
	fprintf(fp, "kickcolor = %d\n", C->kick_color);
	fprintf(fp, "invitecolor = %d\n", C->invite_color);

	if (!ferror(fp)){
		fclose(fp);	
		return(1);
	}
	else {
		fclose(fp);
		return(0);
	}
}


