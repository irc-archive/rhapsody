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
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "defines.h"
#include "parser.h"
#include "log.h"

void command_parse(char *message, char *command, char *params, char *nick, char *user, char *host){
	char prefix[1024];

	prefix[0]=0;
	command[0]=0;
	nick[0]=0;
	user[0]=0;
	host[0]=0;
	params[0]=0;

	if ( sscanf(message, ":%s %s %[^\n]", prefix, command, params)==3){
		sscanf(prefix, "%[^!]!%[^@]@%s", nick, user, host);
	}
	else if (sscanf(message, "%s :%[^\n]", command, params)!=2){
		sscanf(message, "%s %[^\n]", command, params);
	}	

	#if (DEBUG & D_PARSER)
	 	plog ("%s\n\r", message);
        	plog ("Prefix:%s\n\rCommand:%s\n\rParams:%s\n\r", prefix, command, params);
        	plog ("Nick:%s\n\rUser:%s\n\rHost:%s\n\r", nick, user, host);
	#endif
}

int get_next_param(char *message, char *param){
	char temp[1024];
	if (sscanf(message, ":%[^\n\r]", param)==1) {
		strcpy(message, "");
		return(0);
	}	
	else if (sscanf(message, "%s %[^\n\r]", param, temp)>=1) {
		strcpy(message, temp);
		return(1);
	}
	else {
		param[0]=0;
		return(-1);
	}
}

int get_next_word(char *message, char *param){
	char temp[1024];
	if (sscanf(message, "%s %[^\n\r]", param, temp)==2) {
		strcpy(message, temp);
		return(1);
	}	
	else if (sscanf(message, "%s", param)==1) {
		strcpy(message, "");
		return(1);
	}
	else {
		strcpy(param,"");
		strcpy(message,"");		
		return(0);
	}
}

/*
<message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
<prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
<command>  ::= <letter> { <letter> } | <number> <number> <number>
<SPACE>    ::= ' ' { ' ' }
<params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]

<middle>   ::= <Any *non-empty* sequence of octets not including SPACE
               or NUL or CR or LF, the first of which may not be ':'>
<trailing> ::= <Any, possibly *empty*, sequence of octets not including
                 NUL or CR or LF>

<crlf>     ::= CR LF


  1)    <SPACE> is consists only of SPACE character(s) (0x20).
        Specially notice that TABULATION, and all other control
        characters are considered NON-WHITE-SPACE.

  2)    After extracting the parameter list, all parameters are equal,
        whether matched by <middle> or <trailing>. <Trailing> is just
        a syntactic trick to allow SPACE within parameter.

  3)    The fact that CR and LF cannot appear in parameter strings is
        just artifact of the message framing. This might change later.

  4)    The NUL character is not special in message framing, and
        basically could end up inside a parameter, but as it would
        cause extra complexities in normal C string handling. Therefore
        NUL is not allowed within messages.

  5)    The last parameter may be an empty string.

  6)    Use of the extended prefix (['!' <user> ] ['@' <host> ]) must
        not be used in server to server communications and is only
        intended for server to client messages in order to provide
        clients with more useful information about who a message is
        from without the need for additional queries.

   Most protocol messages specify additional semantics and syntax for
   the extracted parameter strings dictated by their position in the
   list.  For example, many server commands will assume that the first
   parameter after the command is the list of targets, which can be
   described with:

   <target>     ::= <to> [ "," <target> ]
   <to>         ::= <channel> | <user> '@' <servername> | <nick> | <mask>
   <channel>    ::= ('#' | '&') <chstring>
   <servername> ::= <host>
   <host>       ::= see RFC 952 [DNS:4] for details on allowed hostnames
   <nick>       ::= <letter> { <letter> | <number> | <special> }
   <mask>       ::= ('#' | '$') <chstring>
   <chstring>   ::= <any 8bit code except SPACE, BELL, NUL, CR, LF and
                    comma (',')>

*/

