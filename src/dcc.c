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

#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "common.h"
#include "network.h"
#include "screen.h"
#include "log.h"
#include "dcc.h"

/* start incoming dcc chat, connect to server */
int start_incoming_dcc_chat(dcc_chat *D){
	unsigned long hostiph;
	struct sockaddr_in initiator;       
	struct in_addr hostaddr;
	int alarm_occured;

	if (D == NULL) return (0);

	D->active=0;
	D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
	if (D->dccfd == -1) {
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "DCC Chat: Cannot create a socket\n");
		return(0);
	}

	hostiph = ntohl(D->hostip); 
	hostaddr.s_addr = hostiph; 		
        initiator.sin_family = AF_INET;                          
        initiator.sin_port = htons(D->port);         
        initiator.sin_addr.s_addr = hostiph;
        bzero(&(initiator.sin_zero), 8);     

	alarm_occured=0;
	alarm(CONNECTTIMEOUT);	
	if (connect (D->dccfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 || alarm_occured){
		alarm_occured=0;	
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "DCC Error while connecting to %s: %s\n", 
			inet_ntoa(hostaddr), strerror(errno));
		return(0);
        }
	fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
	D->active=1;
	D->serverstatus=1;
	return(1);
}


int start_outgoing_dcc_chat(dcc_chat *D){
        int length;
        char buffer[1024];
        fd_set readfds;
        struct timeval timeout;
        int serr, berr;
	int size;
	int fd, new_fd;
	int portstartrange, portendrange, port;
	struct sockaddr_in serveraddr, clientaddr;
	struct hostent *localhost;

	portstartrange = configuration.dccstartport;
	portendrange = configuration.dccendport;
	
	/* if not yet done, configure and bind the socket to listen */
	if (D->serverstatus == 0){
		D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
		fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
		if (D->dccfd == -1) {
			vprint_dcc_chat(D, "Error (%d %s) opening DCC chat server socket.\nDCC chat functionality will not be available.\n", 
				errno, strerror(errno));
			D->serverstatus = -1;
			return(-1);
        	}

		/* find the next available free port */
		for (port = portstartrange; port <= portendrange; port++){
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(port);
			serveraddr.sin_addr.s_addr = INADDR_ANY;

			memset(&(serveraddr.sin_zero), 0, 8);

			berr = bind(D->dccfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
			
			/* if port in already in use, try the next one in the range */
			if (berr == -1 && errno == EADDRINUSE){}
			else if (berr == -1){
				vprint_dcc_chat(D, "Error (%d %s) binding DCC chat server socket.\nDCC chat functionality will not be available.\n", 
				errno, strerror(errno));
			}
			else{
				berr = 0;
				break;		
			}
		}
		if (berr == -1){
			D->serverstatus = -1;
			return(-1);
		}

		//serr = fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
	        listen(D->dccfd, 1);

		D->serverstatus = 1;
		D->localport = ntohs(serveraddr.sin_port);

		localhost = gethostbyname(configuration.dcchostname);
		if (localhost == NULL){
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error starting DCC chat: lookup for \"%s\" failed\n", 
				configuration.dcchostname);
			return(0);
		}

		vprint_all("Using %s (%s) for DCC source host\n", configuration.dcchostname,
			inet_ntoa(*((struct in_addr*)localhost->h_addr)));

		D->localip = htonl(((struct in_addr *)(localhost->h_addr))->s_addr);

		vprint_dcc_chat(D, "DCC chat server listening on port %d\n", ntohs(serveraddr.sin_port));
		vprint_dcc_chat(D, "Awaiting connection from %s...\n", D->nick);
		return(1);
	}

	/* once server initialization is complete, wait for new connection */
	else if (D->serverstatus == 1){
	        size = sizeof(struct sockaddr_in);
	        new_fd = accept(D->dccfd, (struct sockaddr *)&clientaddr, &size);
		if (new_fd != -1){	
			vprint_dcc_chat(D, "Accepting DCC connection from %s port %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			/* at this point close the listening socket and replace it with the active one */
			close(D->dccfd);
			D->dccfd = new_fd;
			D->active = 1;
			return(1);
		}		
	}
	return(0);
}


/* add incoming file data to the list */
dcc_file *add_incoming_dcc_file(transfer *transfer, char *nick, char *filename, unsigned long hostip, unsigned int port, int size){
        dcc_file *firstdcc;
        dcc_file *new;
	char filenamex[1024];
	char filepath[1024];
	char filestamp[1024];
	struct tm *t;
	time_t ct;
	FILE *fp;
 

	sprintf(filepath, "%s/%s", configuration.dccdlpath, filename);

	/* check if the file exists, and if it does, append a timestamp extension */
	fp = fopen(filepath, "rb");
	if (fp != NULL && configuration.dccduplicates == 1){
		ct = time(NULL);
		t = localtime(&ct);
		sprintf(filestamp, "%s.%04d%02d%02d%02d%02d%02d", filename, t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "DCC file %s exists, saving as %s\n", filename, filestamp);
		sprintf(filepath, "%s/%s", configuration.dccdlpath, filestamp);
		fclose(fp);
		strcpy(filenamex, filestamp);
	}
	else strcpy(filenamex, filename);

	fp = fopen(filepath, "wb");
	if (fp == NULL){
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "DCC File: Couldn't open file %s for writing\n", filepath);

		return(NULL);
	}

        firstdcc = transfer->dcclist;
        new = calloc(sizeof(dcc_file), 1);
        if (new==NULL){
		plog ("Cannot allocate memory for DCC file transfer in add_incoming_dcc_file(:1)");
		exit (-1);
	}
        if (firstdcc != NULL){
        	firstdcc->prev = new;
                new->next = firstdcc;
                new->prev = NULL;
                transfer->dcclist = new;
	}
        else {
		new->next = NULL;
                new->prev = NULL;
                transfer->dcclist = new;
	}
	transfer->dcclisttop = new;
	transfer->selectedfile = new;
        strcpy(new->filename, filenamex);
        strcpy(new->nick, nick);
        new->hostip = hostip;
        new->port = port;
        new->size = size; 
        new->type = DCC_RECEIVE; 
	new->transfer = transfer;
	new->byte=0;
	new->dccfp = fp;
	new->allowed = 0;
        return (new);
}

/* add outgoing file data to the list */
dcc_file *add_outgoing_dcc_file(transfer *transfer, char *nick, char *filename){
	FILE *fp;
        dcc_file *firstdcc;
        dcc_file *new;

	fp = fopen(filename, "rb");
	if (fp == NULL){
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "DCC File: Couldn't open file %s for reading\n", filename);
		return(NULL);
	}

        firstdcc = transfer->dcclist;
        new = calloc(sizeof(dcc_file), 1);
        if (new==NULL){
		plog ("Cannot allocate memory for DCC file transfer in add_outgoing_dcc_file(:1)");
		exit (-1);
	}
        if (firstdcc != NULL){
        	firstdcc->prev = new;

                new->next = firstdcc;
                new->prev = NULL;
                transfer->dcclist = new;
	}
        else {
		new->next = NULL;
                new->prev = NULL;
                transfer->dcclist = new;
	}
	transfer->dcclisttop = new;
	transfer->selectedfile = new;
        strcpy(new->filename, filename);
        strcpy(new->nick, nick);
        new->type = DCC_SEND; 
	new->transfer = transfer;
	new->active = 0;

	new->dccfp = fp;
	new->byte=0;

	/* find the length of the outgoing file */
	fseek(new->dccfp, 0, SEEK_END);
	new->size = ftell(new->dccfp);
	fseek(new->dccfp, 0, SEEK_SET);
	time(&(new->last_activity_at));
	time(&(new->last_updated_at));

        return (new);
}


/* start incoming file transfer, connect to remote server */
int start_incoming_dcc_file(dcc_file *D){  	
	unsigned long hostiph;
	struct sockaddr_in initiator;       
	struct in_addr hostaddr;
	int alarm_occured;
			
	if (D == NULL) return (0);

	D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
	if (D->dccfd == -1) {
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "DCC File: Cannot create a socket for %s\n", D->filename);
		remove_dcc_file(D);		
		return(0);
	}

	hostiph = ntohl(D->hostip); 
	hostaddr.s_addr = hostiph; 		
        initiator.sin_family = AF_INET;                          
        initiator.sin_port = htons(D->port);         
        initiator.sin_addr.s_addr = hostiph;
        bzero(&(initiator.sin_zero), 8);     

	alarm_occured=0;
	alarm(CONNECTTIMEOUT);	
	if (connect (D->dccfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 || alarm_occured){
		if (alarm_occured){
			alarm_occured = 0;
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Timeout while connecting to %s\n", inet_ntoa(hostaddr));
		}
		else{
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "DCC Error while connecting to %s: %s\n", 
				inet_ntoa(hostaddr), strerror(errno));
		}
		return(0);
        }
	fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
	time(&(D->starttime));
	time(&(D->last_activity_at));
	time(&(D->last_updated_at));
	D->active=1;
	return(1);
}

/* start outgoing file transfer, create a local server */
int start_outgoing_dcc_file(dcc_file *D){
	//char message[MAXDATASIZE];
	//struct in_addr hostaddr;
        int serr, berr;
	int size;
	int fd, new_fd;
	int portstartrange, portendrange, port;
	struct sockaddr_in serveraddr, clientaddr;
	struct hostent *localhost;

	portstartrange = configuration.dccstartport;
	portendrange = configuration.dccendport;
			
	if (D == NULL) return (0);

	/* if not yet done, configure and bind the socket to listen */
	if (D->serverstatus == 0){
		D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
		if (D->dccfd == -1) {
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error (%d %s) opening DCC send server socket for filename %s.\n", 
				errno, strerror(errno), D->filename);
			D->serverstatus = -1;
			return(0);
        	}

		/* find the next available free port */
		for (port = portstartrange; port <= portendrange; port++){
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(port);
			serveraddr.sin_addr.s_addr = INADDR_ANY;

			memset(&(serveraddr.sin_zero), 0, 8);

			berr = bind(D->dccfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
			
			/* if port in already in use, try the next one in the range */
			if (berr == -1 && errno == EADDRINUSE){}
			else if (berr == -1){
				vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error (%d %s) binding DCC send server socket.\n", 
					errno, strerror(errno));
			}
			else{
				berr = 0;
				break;		
			}
		}
		if (berr == -1){
			D->serverstatus = -1;
			return(-1);
		}

		serr = fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
	        listen(D->dccfd, 1);

		D->serverstatus = 1;
		D->localport = ntohs(serveraddr.sin_port);

		localhost = gethostbyname(configuration.dcchostname);
		if (localhost == NULL){
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error starting DCC send: lookup for \"%s\" failed\n", 
				configuration.dcchostname);
			return(0);
		}

		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_F, "Using %s (%s) for DCC source host\n", configuration.dcchostname,
			inet_ntoa(*((struct in_addr*)localhost->h_addr)));

		D->localip = htonl(((struct in_addr *)(localhost->h_addr))->s_addr);

		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_F, "DCC send server listening on port %d\n", ntohs(serveraddr.sin_port));
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_F, "Awaiting connection from %s...\n", D->nick);
		return(1);
	}

	/* once server initialization is complete, wait for new connection */
	else if (D->serverstatus == 1){
	        size = sizeof(struct sockaddr_in);
	        new_fd = accept(D->dccfd, (struct sockaddr *)&clientaddr, &size);
		if (new_fd != -1){	
			vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_F, "Accepting DCC connection from %s port %d\n", 
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			/* at this point close the listening socket and replace it with the active one */
			close(D->dccfd);
			D->dccfd = new_fd;
			D->active = 1;
			time(&(D->starttime));
			time(&(D->last_activity_at));
			time(&(D->last_updated_at));
			return(1);
		}		
	}
	return(0);
}

int get_dcc_file(dcc_file *D){
	char buffer[MAXDCCPACKET];
	int len;
	unsigned char ack[5];

	if (D->type == DCC_RECEIVE){
		len = recv(D->dccfd, buffer, MAXDCCPACKET, 0);
		if (len > 0){
			//vprint_all("DCC %d byte(s) recvd\n", len);
			D->byte += len;

			fwrite(buffer, len, 1, D->dccfp);
			if (ferror(D->dccfp)){
				vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error %d, %s writing to file %s. Closing connection...\n", 
					errno, strerror(errno), D->filename);
				remove_dcc_file(D);		
				return(-1);
			}			
			time(&(D->last_activity_at));
			gen_dccack(D->byte, ack);
			send(D->dccfd, ack, 4, 0);
			//send_ball(D->dccfd, ack, 4);
			
		}
		else if (len == 0){
			if (D->byte == D->size){
				gen_dccack(D->byte, ack);
				send(D->dccfd, ack, 4, 0);
				//send_ball(D->dccfd, ack, 4);
			}
			else{
				vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error %d, %s while receiving %s. Closing connection...\n", 
				errno, strerror(errno), D->filename);
				remove_dcc_file(D);		
				return(-1);
			}
		}
		else if (len == -1){
			//fprintf(stderr, "DCC read would block\r\n");
			gen_dccack(D->byte, ack);
			send(D->dccfd, ack, 4, 0);
			//send_ball(D->dccfd, ack, 4);
			return(0);			
		}
	}		
	if (D->byte >= D->size) D->active=0;
	return(len);
}

int put_dcc_file(dcc_file *D){
	struct timeval timeout;      	
	fd_set writefds;
	char buffer[MAXDCCPACKET];
	int numbytes, serr, len;
	
	len=0;

	if (D->type==DCC_SEND){
		numbytes = fread(buffer, 1, MAXDCCPACKET, D->dccfp);				
		if (feof(D->dccfp)){
			// return(0);
		}
		else if (ferror(D->dccfp)){
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error %d, %s reading file %s. Closing connection...\n", 
				 errno, strerror(errno), D->filename);
			remove_dcc_file(D);		
			return(-1);
		}			
	
		if (numbytes > 0){
			len = send(D->dccfd, buffer, numbytes, 0);
			// vprint_all("Sent %d bytes %s\n", len, D->filename);
		
			/* if the entire packet was written, nothing else needs to be done for now */
			if (len == numbytes) D->byte += len;

			/* nothing was sent */
			else if (len == -1){
				vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "DCC error sending %s: %d %s\n", D->filename, errno, strerror(errno));
				remove_dcc_file(D);		
				return(-1);			
			}
			/* if the packet wasn't completed, rewind to the last sent byte */
			else{
				D->byte += len;
				fseek(D->dccfp, D->byte, SEEK_SET);
			}
			time(&(D->last_activity_at));
		}
	}

	//if (D->byte >= D->size) D->active=0;
	return(len);
}

void gen_dccack(unsigned long byte, unsigned char *ack){
	bzero(ack, 5);
	ack[3]=byte&0xff;	
	ack[2]=(byte&0xff00)>>8;
	ack[1]=(byte&0xff0000)>>16;
	ack[0]=(byte&0xff000000)>>24;
	//vprint_all("Sending DCC ack %d, %d, %d, %d: value %lu\n", ack[3], ack[2], ack[1], ack[0], byte);
}

int get_dccack(dcc_file *D){
	unsigned char ackbuffer[5];
	int len;
	unsigned long ack;

	// vprint_all("looking for ack\n", ack);

	if (D->type==DCC_SEND){
		/* get the 4 ack bytes */
		len = recv(D->dccfd, ackbuffer, 4, 0);
		if (len == 4){

			ack = ackbuffer[3];
			ack |= ackbuffer[2] << 8;
			ack |= ackbuffer[1] << 16;
			ack |= ackbuffer[0] << 24;
			
			D->ackbyte = ack;
			//vprint_all("Received DCC ack %d, %d, %d, %d: value %lu\n", ackbuffer[3], ackbuffer[2], ackbuffer[1], ackbuffer[0], ack);			
			return(1);

		}
		else if (len > 0){
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Received %d byte DCC ack for %s. Closing connection...\n", len, D->filename);
			remove_dcc_file(D);		
			return(-1);
		}			
		else{
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_F, "Error %d, %s receiving DCC ack for %s. Closing connection...\n", 
			errno, strerror(errno), D->filename);
			remove_dcc_file(D);		
			return(-1);
		}
	}
	return(-1);

}

int dcc_file_exists(transfer *T, dcc_file *D){
	dcc_file *current;

	if (D == NULL || T == NULL) return(0);

	current = T->dcclist;
	while(current != NULL){
		if (current == D) return(1);
		current = current->next;
	}
	return(0);
}

void remove_dcc_file(dcc_file *D){
	if (D == NULL) return;

	if (D->dccfp != NULL) fclose (D->dccfp);
	if (D->dccfd != 0) close (D->dccfd);

	if (D->next == NULL && D->prev == NULL){
		((transfer *)D->transfer)->dcclist = NULL;
	}
	else if (D->prev == NULL){
		((transfer *)D->transfer)->dcclist = D->next;
		((dcc_file *)D->next)->prev = NULL;
	}
	else if (D->next == NULL){
		((dcc_file *)D->prev)->next = NULL;
	}
	else{
		((dcc_file *)D->prev)->next = D->next;
		((dcc_file *)D->next)->prev = D->prev;
	}
	(D->transfer)->dcclisttop = (D->transfer)->dcclist;
	(D->transfer)->selectedfile = (D->transfer)->dcclist;

	free(D);
}

void remove_dcc_chat(dcc_chat *D){
	D->type=-1;
	D->active=0;
	close (D->dccfd);
}

