/************************************************************************/
/*	
/*				tst6.c
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst6.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst6.c,v 1.1 1993-10-12 03:25:00 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*			**************************
/*	
/*	A test program for the client library interface.  This program 
/*	establishes a connection with a client.  
/*	
/*	
/************************************************************************/

#ifndef lint
static char rcsid_tst6_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst6.c,v 1.1 1993-10-12 03:25:00 probe Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"
#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>

int  coded_file;

int
main(argc, argv)
int argc;
char *argv[];
{
	/************************************************************
	 *	            DECLARATIONS			    *
	 ************************************************************/
	/*
	 * The following defines are for convenience in addressing
	 * the fields.
	 */

	/*
	 * Declare the relation and related data structures for
	 * storing the retrieved data.
	 */

	RELATION retrieved_data[1500];
	TUPLE t, peer_tuple;

	LIST_OF_OPERATIONS op_list;

	int coded_len;
	char *coded_dat;

	int	rc;			/* A return code */	
	int loopcount = 0;			/* times around receive loop */
	int tuplecount = 0;			/* number of tuples */
	int     i;
	int	sendcount;
	CONNECTION listener, peer;

	OPERATION  pending_op[1500], listenop, replyop;

	int fd;

	struct sockaddr_in otherside;
	int     othersize = sizeof(otherside);

	gdb_init();


	for (i=0; i<1500; i++) {
		pending_op[i] = create_operation();
	}

	if (argc != 2) {
		fprintf(stderr, "tst2 <hostname>\n");
		exit (4);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Create a listening connection
	/*	
	/*----------------------------------------------------------*/

	listener = create_listening_connection("9425");

	if (listener == NULL) {
		fprintf(stderr, "tst6: could not create listening connection\n");
		exit (4);
	}


 	/*----------------------------------------------------------*/
	/*	
	/*	try for a connection
	/*	
	/*----------------------------------------------------------*/

	listenop = create_operation();

	printf("Queueing operation to talk to peer\n");
/*	gdb_start_listening(listenop, listener, (char *)&otherside, 
			    &othersize, &fd); */

	peer_tuple = NULL;

	gdb_log = fopen("gdblog.tst6", "w");
	gdb_debug(GDB_LOG);			/* turn on logging */
	start_accepting_client(listener, listenop, &peer,
			       (char *)&otherside,
			       &othersize, &peer_tuple);

	printf("Operation queued, status = %d\n",OP_STATUS(listenop));

	complete_operation(listenop);

	printf("Operation completed, status = %d, result = %d, peer=0x%x othersize=%d\n",
	       OP_STATUS(listenop), OP_RESULT(listenop), peer, othersize);

	/*----------------------------------------------------------*/
	/*	
	/*	Now, reply to the client, accepting its connection.
	/*	
	/*----------------------------------------------------------*/

	replyop = create_operation();

	start_replying_to_client(replyop, peer, GDB_ACCEPTED, "", ""); 

	printf("Reply operation queued, status = %d\n",OP_STATUS(listenop));

	complete_operation(listenop);

	printf("Reply operation completed, status = %d, result = %d\n",
	       OP_STATUS(listenop), OP_RESULT(listenop));

	/*----------------------------------------------------------*/
	/*	
	/*	try to receive the encoded data
	/*	
	/*----------------------------------------------------------*/



       	printf("\n\n\nAttempting receipt of sendcount\n\n");

        start_receiving_object(pending_op[0], peer, (char *)&sendcount, 
			     INTEGER_T);

/*	printf("pending_op.status=%d pending_op.result=%d\n", pending_op->status, pending_op->result); */

	complete_operation(pending_op[0]);

	printf("Receive is complete\n");

	printf("%d transmissions are expected\n",sendcount);

	/*----------------------------------------------------------*/
	/*	
	/*	Prepare a gdb list of operations to be used later in 
	/*	the select.  For now, we'll wait only on the last one.
	/*	
	/*----------------------------------------------------------*/

	op_list = create_list_of_operations(1, pending_op[sendcount]);

       	printf("\n\n\nAttempting receipts of relations\n\n");

	for(i=0; i<sendcount; i++)
	        start_receiving_object(pending_op[i+1], peer, (char *)&retrieved_data[i], 
			     RELATION_T);


	complete_operation(pending_op[sendcount]);

	printf("Receive is complete as reported by complete_operation\n");

	for(i=sendcount; i>0; i--)
		if (OP_STATUS(pending_op[i])==OP_COMPLETE)
			break;

	if (i<sendcount)
		printf("Last successfully completed receipt was number %d\n",
		       i);
	else {
		for(t=FIRST_TUPLE_IN_RELATION(retrieved_data[sendcount-1]);
		    t!=NULL; t=NEXT_TUPLE_IN_RELATION(retrieved_data[sendcount-1],t))
		  tuplecount++;

		printf("Received relation contains %d tuples\n",tuplecount);
	}

	printf("closing connection\n");

	sever_connection(peer);

	return 0;
}
