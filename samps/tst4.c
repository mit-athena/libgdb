/************************************************************************/
/*	
/*				tst4.c
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst4.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst4.c,v 1.1 1993-10-12 03:24:56 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*	A test program for the client library interface.  This
/*	program is one of a pair used to test the transmission of
/*	structured data between two sites.  It's counterpart is
/*	named tst3.  This program takes 4 arguments:
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
/*		host2			name of a second host to send to
/*					in parallel
/*	
/*	
/*	OPERATION
/*	---------
/*	
/*	This program starts by making a connection to each of the 
/*	two peers named on the command line.  
/*	
/*	It then creates a bunch	 of tuples, each of which has 5
/*	fields, puts some dummy data in the fields, and adds each
/*	of the tuples to an initially null relation.  The result
/*	is a single relation with lots of tuples in it.
/*	
/*	The program then sends an initial message to each of its peers.
/*	Each message consists only of the single integer 'sendcount',
/*	which tells the recipient how many further transmissions to
/*	expect.  We wait for each of these initial transmissions to
/*	complete before proceeding.
/*	
/*	We then loop repeatedly sending the same relation to each of
/*	our peers.  This is all done asynchronously, with trnasmission
/*	to the two peers proceeding independently.  In fact, one of the
/*	peers may die prematurely, and transmission to the other will
/*	complete and this program will proceed cleanly.  To be fancy,
/*	we use the op_select_all call to await completion of these
/*	transmissions.  Normally, op_select_... would be used only
/*	when the program wanted to control fd's of its own (like
/*	the keyboard) asynchronously.
/*	
/*	When the last transmission completes, we write out some debugging
/*	information and exit.
/*	
/*	Actually, there is a bit more subtlty than the above would imply.
/*	Purely as a means of stressing our termination logic, we queue 
/*	an extra 100 tranmssions which are not expected by the peers.  
/*	The peers sever their connections after receiving the number of
/*	tuples they were told to expect, but in fact, some extra operations
/*	may be ongoing, particularly if the socket is doing lots of buffering.
/*	We are therefore testing the asynchronous cancellation of ongoing
/*	transmissions.
/*
/*	
/************************************************************************/

#ifndef lint
static char rcsid_tst4_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/tst4.c,v 1.1 1993-10-12 03:24:56 probe Exp $";
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
	
	TUPLE   t;			
	int coded_len;
	char *coded_dat;

	int	rc;			/* A return code */	
	int     loopcount = 0;			/* times around transmit loop*/

	int     sendcount;

	int     donecount = 0;			/* number of connections */
						/* on which operations */
						/* have completed */

       /*
        * Define the connection descriptors for the connections to
        * our two peers.
        */

#define MAXOPS 1500				/* change tst3 when you */
						/* change this !*/
	CONNECTION peer, peer2;


       /*
        * Define two arrays of pending operations.  We are going to
        * do all of our transmissions asynchronously, and each of
        * these 'operations' holds the state of one pending transmission.
        * 
        * We know we're done when the last of them completes.
        */
	OPERATION  pending_op[MAXOPS],pending2_op[MAXOPS];

	LIST_OF_OPERATIONS op_list;


/************************************************************************/
/*	
/*			Execution begins here
/*	
/************************************************************************/

	gdb_init();				/* initialize gdb */

	if (argc != 5) {
		fprintf(stderr, "tst2 <hostname> <tuplecount> <sendcount> <host2>\n");
		exit (4);
	}

	sendcount = atoi(argv[3]);		/* number of times to send */

	if (sendcount > MAXOPS-1) {
		fprintf(stderr,"sendcount may not be greater than %d\n", MAXOPS-1);
		exit (8);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	We're going to be doing lots of asynchronous operations
	/*	and we need a descriptor to hold the state of each
	/*	one that can be simultaneously active.  Note that
	/*	we use the 0'th one for our initial transmission,
	/*	1 through sendcount for the main transmissions, and
	/*	100 after that for the asynchrony check.  Actually,
	/*	the 0th one could be re-used, since it's all done 
	/*	by the time we hit the main loop, but we con't bother.
	/*	
	/*----------------------------------------------------------*/

	for (i=0; i<sendcount+101; i++) {
		pending_op[i] = create_operation();
		pending2_op[i] = create_operation();
	}

	/*----------------------------------------------------------*/
	/*	
	/*	try for a connection to each of our peers
	/*	
	/*----------------------------------------------------------*/

	printf("Attempting connection to host: %s\n", argv[1]);
	peer = start_peer_connection(argv[1]);
	printf("Attempting connection to host: %s\n", argv[4]);
	peer2 = start_peer_connection(argv[4]);

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

        /*
	 * Put lots of tuples into the relation
	 */
	printf("Creating tuples\n");
	for (i=0; i<atoi(argv[2]); i++) {
		t = create_tuple(tuple_desc);
		
		initialize_tuple(t);


		*(double *)FIELD_FROM_TUPLE(t, COST) = 12.34 * (i+1);
		string_alloc((STRING *)FIELD_FROM_TUPLE(t,MAN), 20);
		strcpy(STRING_DATA(*((STRING *)FIELD_FROM_TUPLE(t,MAN))),
		       "Manager field data");
		ADD_TUPLE_TO_RELATION(outbound_data, t);		
	}


	printf("tst.c: relation initialized\n");


	/*----------------------------------------------------------*/
	/*	
	/*	Send the count of transmissions we're going to do
	/*	to the both of our peers.  Wait for that transmissions to
	/*	complete.
	/*	
	/*	NOTE:  We could make this simpler by using the 
	/*	synchronous form, which is "send_object", but this
	/*	is a better illustration of the more flexible
	/*	asynchronous approach.  Also, this version has
	/*	the advantage that it does the transmissions in
	/*	parallel.
	/*	
	/*----------------------------------------------------------*/

	printf("\n\n\nAttempting transmission of sendcount\n\n");

        start_sending_object(pending_op[0], peer, (char *)&sendcount, 
			     INTEGER_T);
        start_sending_object(pending2_op[0], peer2, (char *)&sendcount, 
			     INTEGER_T);

	/*
	 * Note, order of the next two statements is NOT significant.  Read
         * the spec!
 	 */
	complete_operation(pending_op[0]);
	complete_operation(pending2_op[0]);
	printf("Transmission complete after two complete_ops\n");

	/*----------------------------------------------------------*/
	/*	
	/*	Prepare a gdb list of operations to be used later in 
	/*	the select.  For now, we'll wait only on the last one.
	/*	
	/*	op_select waits for any of a list of operations to 
	/*	complete.  This is a way of putting the list together
	/*	once and for all, so we don't have to use varargs
	/*	on op_select.
	/*	
	/*----------------------------------------------------------*/

	op_list = create_list_of_operations(2, pending_op[sendcount],
					    pending2_op[sendcount]);

	/*----------------------------------------------------------*/
	/*	
	/*	Now transmit the same relation over and over again,
	/*	queuing up the requests to transmit as fast as we can.
	/*	We alternate sending to peer and peer2, but the actual
	/*	order in which things get done depends completely
	/*	on the speed at which the receivers and the sockets
	/*	are running.  Remember:  all of this is done completely
	/*	asynchronously.  For example, it may be the connection
	/*	to 'peer' which happens to make progress at the time
	/*	we queue a new request to 'peer2.'  We always make
	/*	as much progress as we can on all connections as 
	/*	fast as we can.
	/*	
	/*----------------------------------------------------------*/

	printf("\n\n\nAttempting transmission of relation\n\n");

	/*
	 * Queue sendcount operations to send the relations, include some 
	 * extras to check cancellation logic when connections are closed.
	 */
	for(i=0; i<sendcount+100; i++) {
	        start_sending_object(pending_op[i+1], peer, 
				     (char *)&outbound_data,
				     RELATION_T);
	        start_sending_object(pending2_op[i+1], peer2, 
				     (char *)&outbound_data,
				     RELATION_T);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Wait for the last operation on each of the
	/*	connections to complete.  The op_select_all will
	/*	not drop through until both of the operations
	/*	specified in op_list above complete or are cancelled.
	/*	
	/*	Note that we could have done this more easily with
	/*	two calls to 'complete_operation' since we aren't
	/*	using the real power of op_select, but this is
	/*	illustrative of what you can do.  
	/*	
	/*----------------------------------------------------------*/

	printf("Going for broke.\n"); 

	op_select_all(op_list, 0, NULL, NULL, NULL, NULL);


	/*----------------------------------------------------------*/
	/*	
	/*	If all went well, both will have completed, but 
	/*	if one of the connections broke prematurely, the
	/*	corresponding operation will be OP_CANCELLED
	/*	instead of OP_COMPLETE.
	/*	
	/*----------------------------------------------------------*/


	if(pending_op[sendcount]->status != OP_COMPLETE) {
		donecount++;
	}
	if(pending2_op[sendcount]->status != OP_COMPLETE) {
		donecount++;
	}

	printf("missed completion count = %d\n",++loopcount);


	/*----------------------------------------------------------*/
	/*	
	/*	Close the connection.
	/*	
	/*----------------------------------------------------------*/

	printf("closing connection\n");

	sever_connection(peer2);
	sever_connection(peer);

	/*----------------------------------------------------------*/
	/*	
	/*	The following is just for debugging and testing. 
	/*	If you look closely at the loop above, we actually
	/*	queued 100 more sends than we were asked too.  These
	/*	won't actually be received at the other end, but
	/*	if the socket is 'deep', some of them may appear
	/*	to complete from this end.  i.e. we may have managed
	/*	to send all the data for some extras after 'sendcount'.
	/*	Conversely, if the connection was broken prematurely,
	/*	only the first few will have completed, and the others
	/*	should be cancelled.  For general interest, print
	/*	a map indicating how many completed on each connection.
	/*	
	/*----------------------------------------------------------*/

	printf("\n\nFinal operation status list--%s:\n",argv[1]);
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

	printf("\n\nFinal operation status list--%s\n",argv[4]);
	for(i=0; i<sendcount+101; i++) {
		switch (pending2_op[i]->status) {
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


	return 0;
}
