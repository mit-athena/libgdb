/************************************************************************/
/*	
/*		          tfsr (test forking server)
/*			  --------------------------
/*	
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tfsr.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tfsr.c,v 1.1 1993-10-12 03:24:48 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/************************************************************************/
/*	
/*	PURPOSE
/*	-------
/*	
/*	A GDB server program demonstrating techniques for asynchronously
/*	communicating with an arbitrary number of clients by forking 
/*	a new server process for each incoming client.
/*	
/*	Each forked child  receives a stream of integers,
/*	which it interprets as ASCII characters.  The characters are
/*	converted to uppercase, and then sent back to the client from
/*	which they came.  
/*	
/*	NOTE
/*	----
/*	
/*	This program is interface compatible with tsr.c.  Clients
/*	cannot tell which style of server they are using.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_tfsr_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tfsr.c,v 1.1 1993-10-12 03:24:48 probe Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"


int
main(argc, argv)
int argc;
char *argv[];
{
	/*----------------------------------------------------------*/
	/*	
	/*		       LOCAL VARIABLES
	/*	
	/*----------------------------------------------------------*/

	CONNECTION client;			/* talk on this to client */

	int data;				/* receive data here */

	/*----------------------------------------------------------*/
	/*	
	/*			EXECUTION BEGINS HERE
	/*	
	/*			  Check parameters
	/*	
	/*----------------------------------------------------------*/

	if (argc != 2) {
		fprintf(stderr,"Correct form is %s <servicename>\n",
			argv[0]);
		exit(4);
	}

	/*----------------------------------------------------------*/
	/*	
	/*			Initialize
	/*	
	/*----------------------------------------------------------*/

	gdb_init();				/* set up gdb */

	/*----------------------------------------------------------*/
	/*	
	/*	Now, turn ourselves into a forking server.
	/*	
	/*----------------------------------------------------------*/

	client = create_forking_server(argv[1],NULL);
	fprintf(stderr,"forked\n");

	/*----------------------------------------------------------*/
	/*	
	/*	Here we are in the child process for each client.
	/*	Echo the characters.
	/*	
	/*----------------------------------------------------------*/

	while (TRUE) {
		if (receive_object(client, &data, INTEGER_T) ==
		    OP_CANCELLED) {
			fprintf(stderr,"receive error\n");
			exit(4);
		}
		if (data >= 'a' && data <= 'z')
			data += 'A'-'a';	/* upcase the response */
		if (send_object(client, &data, INTEGER_T) ==
		    OP_CANCELLED) {
			fprintf(stderr,"send error\n");
			exit(4);
		}
	}
}
