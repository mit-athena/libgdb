/************************************************************************/
/*	
/*				tst3.c
/*	
/*	A test program for the client library interface.  This program 
/*	establishes a connection with a peer, and then attempts to read
/*	in a relation.
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst3.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst3.c,v 1.1 1993-10-12 03:24:55 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/************************************************************************/

#ifndef lint
static char rcsid_tst3_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst3.c,v 1.1 1993-10-12 03:24:55 probe Exp $";
#endif


#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"
#include <sys/file.h>

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
	TUPLE t;

	LIST_OF_OPERATIONS op_list;

	int coded_len;
	char *coded_dat;

	int	rc;			/* A return code */	
	int loopcount = 0;			/* times around receive loop */
	int tuplecount = 0;			/* number of tuples */
	int     i;
	int	sendcount;

	CONNECTION peer;

	OPERATION  pending_op[1500];

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
	/*	try for a connection
	/*	
	/*----------------------------------------------------------*/

/*	coded_file = open("retriever.dat", O_CREAT | O_WRONLY, 0x777);

	if (coded_file <0 ) {
		perror("Cannot open retriever.dat");
		exit (8);
	}
*/
	printf("Attempting connection to host: %s\n", argv[1]);

	peer = start_peer_connection(argv[1]);

	printf("start_peer_connection returned 0x%x\n", peer);

	if (peer != NULL) {
		printf("\t status = %d  in.fd = %d  out.fd = %d\n",
		       peer->status, peer->in.fd, peer->out.fd);
	}

/*	peer -> out.fd = coded_file;		/* fudge the output fd */

	printf("peer is at 0x%x contains 0x%x\n", &peer, peer);

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

	/*----------------------------------------------------------*/
	/*	
	/*	Delete the received relations
	/*	
	/*----------------------------------------------------------*/

	for(i=0; i<sendcount; i++)
		delete_relation(retrieved_data[i]);

	/*----------------------------------------------------------*/
	/*	
	/*	Close the connection
	/*	
	/*----------------------------------------------------------*/

	printf("closing connection\n");

	sever_connection(peer);

	return 0;
}
