/************************************************************************/
/*	
/*				tst2.c
/*	
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst2.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst2.c,v 1.1 1993-10-12 03:24:53 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*
/*
/*	A test program for the client library interface.  This program
/*	is one of a pair used to test the transmission of structured data
/*	between two sites.  This program takes 3 arguments:
/*	
/*		hostname		name of the host at which
/*					the tst3 program is running
/*	
/*		tuplecount		the number of tuples to put into
/*					each relation that is sent
/*	
/*		sendcount		the number of times to send a 
/*					relation (max is 1499)
/*	
/*	Warning: there is some stray code lying around in this from 
/*		 earlier versions that is commented out in the left
/*		 margin.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_tst2_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst2.c,v 1.1 1993-10-12 03:24:53 probe Exp $";
#endif


#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"
#include <sys/file.h>

char *field_names[] = {"desc",	
		       "code",
		       "man",
		       "cost",
		       "count"};
FIELD_TYPE field_types[] = {STRING_T,	    /* desc */
			    INTEGER_T,      /* code */
			    STRING_T,	    /* man */
			    REAL_T,	    /* cost */
			    INTEGER_T};	    /* count */

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
	 * Declare the names of fields their types
	 */

	int  field_count = 5;
	int i;
	/*
	 * The following defines are for convenience in addressing
	 * the fields.
	 */

#define DESC  0
#define CODE  1
#define MAN   2
#define COST  3
#define COUNT 4

	/*
	 * Declare the relation and related data structures for
	 * storing the transmitted data.
	 */

	TUPLE_DESCRIPTOR tuple_desc;
	RELATION outbound_data, decoded_rel;

	/*
	 * Declarations for misc. variables
	 */
	
	TUPLE   t;			/* next tuple to print */
	int coded_len;
	char *coded_dat;

	int	rc;			/* A return code */	
	int     loopcount = 0;			/* times around transmit loop*/

	int     sendcount;

	/*
	 * Declarations for connection to our peer and all the operations
         * we may queue up
	 */
#define MAXOPS 1500				/* change tst3 when you */
						/* change this !*/
	CONNECTION peer;

	OPERATION  pending_op[MAXOPS];

	LIST_OF_OPERATIONS op_list;


/************************************************************************/
/*	
/*			Execution begins here
/*	
/************************************************************************/


	gdb_init();

	if (argc != 4) {
               /* 
                * This is a test of the way that this works Here we go
                * again.
                */
		fprintf(stderr, "tst2 <hostname> <tuplecount> <sendcount>\n");
		exit (4);
	}

	sendcount = atoi(argv[3]);		/* number of times to send */

	if (sendcount > MAXOPS-1) {
		fprintf(stderr,"sendcount may not be greater than %d\n", MAXOPS-1);
		exit (8);
	}

	for (i=0; i<sendcount+1+100 /* for rigged test below */; i++)
		pending_op[i] = create_operation();

	/*----------------------------------------------------------*/
	/*	
	/*	try for a connection
	/*	
	/*----------------------------------------------------------*/

#ifdef  CODEDFILE
	coded_file = open("socket.dat", O_CREAT | O_WRONLY, 0x777);

	if (coded_file <0 ) {
		perror("Cannot open socket.dat");
		exit (8);
	}
#endif  CODEDFILE

	printf("Attempting connection to host: %s\n", argv[1]);

	peer = start_peer_connection(argv[1]);

	printf("start_peer_connection returned 0x%x\n", peer);

	if (peer != NULL) {
		printf("\t status = %d  in.fd = %d  out.fd = %d\n",
		       peer->status, peer->in.fd, peer->out.fd);
	}

/*	peer -> out.fd = coded_file;		/* fudge the output fd */

	/*----------------------------------------------------------*/
	/*	
	/*	create some structured data
	/*	
	/*----------------------------------------------------------*/

	tuple_desc = create_tuple_descriptor(field_count, field_names,
					     field_types);

	printf("tst.c: tuple desc created.. attempting to create relation\n");
	outbound_data = create_relation(tuple_desc);

	printf("tst.c: relation created\n");


/*	print_tuple_descriptor("Test Tuple Descriptor", tuple_desc); 

	printf("tst.c: descriptor formatted, formatting relation\n");

	print_relation("Test Relation", outbound_data);
*/
	printf("Creating tuples\n");

	for (i=0; i<atoi(argv[2]); i++) {
		t = create_tuple(tuple_desc);
		
		initialize_tuple(t);

/*		fprintf(stderr, "Following tuple should contain null fields:\n\n");

		print_tuple("A NULL TUPLE", t);

*/		*(int *)FIELD_FROM_TUPLE(t, CODE) = i+1;

		*(double *)FIELD_FROM_TUPLE(t, COST) = 12.34 * (i+1);
		string_alloc((STRING *)FIELD_FROM_TUPLE(t,MAN), 20);
		strcpy(STRING_DATA(*((STRING *)FIELD_FROM_TUPLE(t,MAN))),
		       "Manager field data");
		ADD_TUPLE_TO_RELATION(outbound_data, t);		
	}


	printf("tst.c: relation initialized\n");

/*	print_relation("Test Relation", outbound_data); */

	/*----------------------------------------------------------*/
	/*	
	/*	Send the count of transmissions we're going to do
	/*	to the other side.  Wait for that transmission to
	/*	complete.
	/*	
	/*----------------------------------------------------------*/

	printf("\n\n\nAttempting transmission of sendcount\n\n");

        start_sending_object(pending_op[0], peer, (char *)&sendcount, 
			     INTEGER_T);

	complete_operation(pending_op[0]);

	printf("Transmission of count is complete\n");

	/*----------------------------------------------------------*/
	/*	
	/*	Prepare a gdb list of operations to be used later in 
	/*	the select.  For now, we'll wait only on the last one.
	/*	
	/*----------------------------------------------------------*/

	op_list = create_list_of_operations(1, pending_op[sendcount]);

	/*----------------------------------------------------------*/
	/*	
	/*	Now transmit the same relation over and over again,
	/*	queuing up the requests to transmit as fast as we can.
	/*	Then wait for the last of them to report complete.
	/*	
	/*----------------------------------------------------------*/

	printf("\n\n\nAttempting transmission of relation\n\n");

/*
 * Queue sendcount operations to send the relations  Just for fun, queue
 * some extra requests.  In the end, we won't wait for them, and whichever
 * ones are still running when we leave should get cancelled.
 */
	for(i=0; i<sendcount+100; i++)
	        start_sending_object(pending_op[i+1], peer, 
				     (char *)&outbound_data,
				     RELATION_T);

/*
 * Wait for last one to complete
 */
	complete_operation(pending_op[sendcount]);

	printf("Transmission complete\n");

	/*----------------------------------------------------------*/
	/*	
	/*	Delete structured data
	/*	
	/*----------------------------------------------------------*/

	delete_relation(outbound_data);
	delete_tuple_descriptor(tuple_desc);

	/*----------------------------------------------------------*/
	/*	
	/*	Close the connection and exit.
	/*	
	/*----------------------------------------------------------*/

/*	printf("Sleeping 20 seconds\n");
	sleep(20);
*/

	printf("closing connection\n");

	sever_connection(peer);
/*
 * Check on status of operations which were not matched at the receiver
 */
	printf("\n\nFinal operation status list:\n");
	for(i=0; i<sendcount+101; i++) {
		switch (pending_op[i]->status) {
		      case OP_COMPLETE:
			printf("O");
			break;
		      case OP_CANCELLED:
			printf("A");
			break;
		      default:
			printf("?");
			break;
		}
		if ((i+1)%50 == 0) {
			printf("\n");
			continue;
		}
		if ((i+1)%10 == 0) {
			printf(" ");
		}
	}

	printf("\n");

	return ;
}
