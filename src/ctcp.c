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
#include <strings.h>
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
#include <stdarg.h>

#include "autodefs.h"
#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "common.h"
#include "log.h"
#include "network.h"
#include "screen.h"
#include "dcc.h"
#include "ctcp.h"
#include "config.h"
#include "main.h"

static int ctcpreqcount = 0;
static time_t ctcplastreq = 0;

int parse_ctcp(char *bufferout, char *bufferin){ 	
	char decoded[MAXDATASIZE];
	int i, j;
	int lquote, mquote;
	int ctcp;

	lquote=FALSE;
	mquote=FALSE;
	ctcp=FALSE;
	strcpy(bufferout, "");

	bzero(decoded, MAXDATASIZE);
	for (i=0, j=0; i<strlen(bufferin); i++){
		if (bufferin[i]==1){
			if (lquote) lquote = FALSE;
			else {
				lquote = TRUE;
				ctcp = TRUE;
			}
		}
		else if (bufferin[i]==16) mquote = TRUE;			
		else {
			if (mquote && lquote) {
				mquote = FALSE;
				if (bufferin[i]=='0'){
					decoded[j]=0;
					j++;
				}
				else if (bufferin[i]=='n'){
					decoded[j]='\n';
					j++;
				}
				else if (bufferin[i]=='r'){
					decoded[j]='\r';
					j++;
				}
				else if (bufferin[i]==16){
					decoded[j]=16;
					j++;
				}
			}
			else {
				decoded[j]=bufferin[i];
				j++;
			}
		}
	}
	strcpy(bufferout, decoded);
	return(ctcp);
}

int execute_ctcp(server *server, char *command, char *cmdnick, char *cmduser, char *cmdhost, char *dest){  	
	char message[MAXDATASIZE];
	char dcccmd[MAXDATASIZE];
	char dcctype[MAXDATASIZE];
	char dccinfo[MAXDATASIZE];
	char commandstring[MAXDATASIZE];
	char timestamp[64];
	char *replystate;
	time_t now;
	int ready;
	int replied;


	strncpy(commandstring, command, MAXDATASIZE);

	now = time(NULL);
	ready = 0;
	replied = 0;
	replystate = "throttled";

	/* check if the response needs to be throttled or not */
	if (configuration.ctcpthrottle <= 0);
	else if (configuration.ctcpthrottle < 60){
		if ((now - ctcplastreq) > (60 / configuration.ctcpthrottle)){
			ready = 1;
			replystate = "replied";
		}
	}
	else {
		if (now > ctcplastreq){
			ctcpreqcount = 0;
			ready = 1;
			replystate = "replied";
		}
		else if (ctcpreqcount < configuration.ctcpthrottle / 60){
			ready = 1;
			replystate = "replied";
		}
	}

	if (strncasecmp(command,"PING",4)==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("PING", "%s", command + 6), cmdnick, "");
		//sprintf(reply, "NOTICE %s :%cPING %s%c\n", cmdnick, 1, command+6, 1);
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP PING Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"VERSION")==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("VERSION", "%s", DEFAULT_CTCPVERSION), cmdnick, "");
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP VERSION Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"SOURCE")==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("SOURCE", "%s", DEFAULT_CTCPSOURCE), cmdnick, "");
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP SOURCE Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"FINGER")==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("FINGER", "%s", configuration.ctcpfinger), cmdnick, "");
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP FINGER Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"USERINFO")==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("USERINFO", "%s", configuration.ctcpuserinfo), cmdnick, "");
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP USERINFO Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"CLIENTINFO")==0){
		if (ready) sendcmd_server(server, "NOTICE", create_ctcp_command("CLIENTINFO", "%s", DEFAULT_CTCPCLIENTINFO), cmdnick, "");
		//sprintf(reply, "NOTICE %s :%cCLIENTINFO %s%c\n", cmdnick, 1, DEFAULT_CTCPCLIENTINFO, 1);
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP CLIENTINFO Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}
	else if (strcasecmp(command,"TIME")==0){
		if (ready){ 
			strcpy(timestamp, ctime(&now));
			timestamp[strlen(timestamp) - 1] = 0;
			sendcmd_server(server, "NOTICE", create_ctcp_command("TIME", "%s", timestamp), cmdnick, "");
		}
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP TIME Request from %s... %s.\n", cmdnick, replystate);
		replied = 1;
	}

	if (replied){
		ctcplastreq = now;
		ctcpreqcount++;
		return(TRUE);
	}	


	if (strncasecmp(command,"ERROR", 5)==0){
		vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "CTCP %s reported from %s... %s\n", command, cmdnick);
		print_all(message);
		return(TRUE);
	}

	else if (strncasecmp(command,"DCC", 3)==0){
		sscanf(command, "%s %s %[^\n]", dcccmd, dcctype, dccinfo);
		//get_next_word(command, scratch);
		//get_next_word(command, scratch);
		
		if (strcasecmp(dcctype, "SEND")==0){
			dcc_file *D;
			char filename[MAXDATASIZE];
			unsigned long hostip;
			unsigned long hostiph;
			struct in_addr hostaddr;
			
			unsigned short port;
			unsigned int size;
									
			if (sscanf(dccinfo, "%s %lu %hd %d", filename, &hostip, &port, &size) != 4){
				vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Unknown DCC SEND request format: %s\n", commandstring);
				return(FALSE);
			}

			if (!config_user_exists(&configuration, CONFIG_IGNORED_USER_LIST, cmdnick) || 
				config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, cmdnick)){
			
				D = add_incoming_dcc_file(transferscreen, cmdnick, filename, hostip, port, size);
				if (D == NULL){
					vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Unable to create DCC resources\n");
					return(FALSE);
				}

				hostiph = ntohl(hostip); 
				hostaddr.s_addr = hostiph; 
				vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "DCC SEND file request for %s from %s port %hu\n", 
					filename, inet_ntoa(hostaddr), port);
				return (1);
			}
			else {
				vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Ignoring DCC SEND file request from %s for %s from %s port %hu\n", 
					cmdnick, filename, inet_ntoa(hostaddr), port);
			}
			return(TRUE);
		}					
		else if (strcasecmp(dcctype, "CHAT")==0){
			dcc_chat *D;
			unsigned long hostip;
			unsigned long hostiph;
			struct in_addr hostaddr;
			unsigned short port;
									
			if (sscanf(dccinfo, "%lu %hd", &hostip, &port) != 2){
				if (sscanf(dccinfo, "chat %lu %hd", &hostip, &port) != 2){
					vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Unknown DCC CHAT request format: %s\n", commandstring);
					return(FALSE);
				}
			}

			if (!config_user_exists(&configuration, CONFIG_IGNORED_USER_LIST, cmdnick) || 
				config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, cmdnick)){

				D = add_incoming_dcc_chat(cmdnick, dest, server, hostip, port);

				if (D == NULL){
					vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Unable to create DCC resources\n");
					return(FALSE);
				}

				hostiph = ntohl(hostip); 
				hostaddr.s_addr = hostiph; 
				vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "DCC CHAT request from %s port %hu\n", inet_ntoa(hostaddr), port);
				set_dccchat_update_status(D, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				return(TRUE);
			}
			else{
				vprint_all_attribs(CTCP_COLOR_F, CTCP_COLOR_B, "Ignoring DCC CHAT request from %s at %s port %hu\n", 
					cmdnick, inet_ntoa(hostaddr), port);
			}
		}
	}
	return(FALSE);
}


int translate_ctcp_message (char *command, char *cmdnick, char *cmduser, char *cmdhost, char *message){
	char scratch[MAXDATASIZE];

	if (strncmp(command,"ACTION", 6)==0){
		sprintf(scratch, "%c%d,%d* %s %s%c%d,%d *\n", 3, CTCP_COLOR_F, CTCP_COLOR_B, 
			cmdnick, command+7, 3, CTCP_COLOR_F, CTCP_COLOR_B);
		strcpy(message, scratch);
		return(TRUE);
	}
	else return (FALSE);
}

char *create_ctcp_message(char *message, ...){
	static char buffer[MAXDATASIZE];
        va_list ap;
	char string[MAXDATASIZE];
        
	va_start(ap, message);
	vsprintf(string, message, ap);
        va_end(ap);

	sprintf(buffer, "%c%s%c", 1, string, 1);
	return(buffer);
}

char *create_ctcp_command(char *command, char *parameters, ...){
        va_list ap;
	static char buffer[MAXDATASIZE];
	char string[MAXDATASIZE];
        
	va_start(ap, parameters);
	vsprintf(string, parameters, ap);
        va_end(ap);
        
	sprintf(buffer, "%c%s %s%c", 1, command, string, 1);
	return(buffer);
}


