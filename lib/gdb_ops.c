/************************************************************************/
/*	
/*			   gdb_ops.c
/*	
/*	      GDB - Asynchronous Operations and Their Synchronous
/*		    Counterparts
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_ops.c,v $
/*	$Author: vrt $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_ops.c,v 1.1 1993-04-28 09:38:13 vrt Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*	These routines provide a suite of asynchronous operations 
/*	on connections.  Note: operations relating to databases 
/*	are found in gdb_db.c.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_gdb_ops_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_ops.c,v 1.1 1993-04-28 09:38:13 vrt Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef vax
extern u_long htonl();
#endif vax

/************************************************************************/
/*	
/*				send_object 
/*	
/*	Synchronous form of start_sending_object.  Returns either
/*	OP_CANCELLED, or OP_RESULT(op).
/*	
/*	Starts an asynchronous send of the object, then waits for that 
/*	to complete.
/*	
/************************************************************************/

int
send_object(con, objp, type)
CONNECTION con;
char *objp;
int type;
{
	register OPERATION op;
	register int retval;

       /*
        * Do the send and wait for it to complete
        */
	op = create_operation();
	start_sending_object(op, con, objp, type);
	(void) complete_operation(op);

       /*
        * Check the return value, clean up, return to caller.
        */
	if (OP_STATUS(op) == OP_COMPLETE)
		retval =  OP_RESULT(op);
	else
		retval = OP_STATUS(op);
	delete_operation(op);
	return retval;
}

/************************************************************************/
/*	
/*			start_sending_object 
/*	
/*	Start the asynchronous transmission of a gdb object.
/*	Note that this routine must be passed the address of the object,
/*	not the object itself.
/*	
/*	The following three routines work together, and may be considered
/*	as a single entity implementing the operation.  The first merely
/*	saves away its arguments and queues the operation on the designated
/*	connection.  This stays there until they percolate to the head of
/*	the queue.  The second is the initialization routine, which is
/*	called by the connection maintenance logic when the operation
/*	first reaches the head of the queue.  This routine encodes
/*	the supplied data for transmission, and then sends it.  If the
/*	transmission executes synchronously, then the third routine is
/*	called immediately to clean up.  If not, the third routine is
/*	marked as the 'continuation' routine, which will cause its 
/*	invocation when the transmission completes.
/*	
/*	The data is preceded by its length expressed as a long in 
/*	network byte order.
/*	
/************************************************************************/

/*
 * State of this operation is stored here while the operation is queued
 * and when it becomes active.  This structure maps *arg.
 */
struct obj_data {
	char 	*objp;				/* address of the object to */
						/* be sent */
	int	type;				/* type code for the object */
						/* to be sent*/
	char    *flattened;			/* address of first byte */
						/* of flattened data */
	int	len;				/* length of the flattened */
						/* data */
};

int g_isnobj();
int g_csnobj();

int
start_sending_object(op, con, objp, type)
OPERATION op;
CONNECTION con;
char *objp;
int type;
{
	struct obj_data *arg;

       /*
        * Make sure the supplied connection and operation are legal
        */
	GDB_CHECK_CON(con, "start_sending_object")
	GDB_CHECK_OP(op, "start_sending_object")

       /*
        * Allocate a structure to hold our state; fill it in.
        */
	arg = (struct obj_data *)db_alloc(sizeof(struct obj_data));
	arg->objp = objp;
	arg->type = type;

       /*
        * Queue this as an operation at the tail of the outbound
        * half-connection queue.
        */
	initialize_operation(op, g_isnobj, (char *)arg, (int (*)())NULL);
	(void) queue_operation(con, CON_OUTPUT, op);
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_isnobj
	/*	
	/*	Init routine for sending an object.  This routine is 
	/*	called by the connection management logic when the send
	/*	request percolates to the top of the queue.  This routine
	/*	reformats the data into an appropriate form for transmission.
	/*	The format used is a length, represented as a long in 
	/*	network byte order, followed by the data itself.  The
	/*	continuation routine below is called, either synchronously 
	/*	or asynchronously, once the transmission is complete.
	/*	
	/*----------------------------------------------------------*/

int
g_isnobj(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct obj_data *arg;
{
       /*
        * Find out the encoded length of the data
        */
	arg->len = FCN_PROPERTY(arg->type, CODED_LENGTH_PROPERTY)
	  		       (arg->objp, hcon);

       /*
        * Allocate space and flatten (encode) the data
        */
	arg->flattened = db_alloc(arg->len+sizeof(long));
	*(u_long *)arg->flattened = htonl((u_long)arg->len);
	FCN_PROPERTY(arg->type, ENCODE_PROPERTY)
	  		       (arg->objp, hcon, arg->flattened+sizeof(long));

       /*
        * Set up continuation routine in case it's needed after the return
        */
	op->fcn.cont = g_csnobj;

       /*
        * Start sending the data, maybe even complete
        */
	if (gdb_send_data(hcon, arg->flattened, arg->len + sizeof(long)) == 
	    OP_COMPLETE) {		
		return g_csnobj(op, hcon, arg)	;/* this return is a little */
						/* subtle.  As continuation */
						/* routines call each other */
						/* synchronously, the last */
						/* one determines whether we */
						/* completed or are still */
						/* running.  That status */
						/* percolates back through */
						/* the entire call chain. */
	} else {
		return OP_RUNNING;
	}
}


	

	
	
	/*----------------------------------------------------------*/
	/*	
	/*	                 g_csnobj
	/*	
	/*	Continuation routine for sending an object.  Since there is
	/*	only one transmission, started by the init routine, this is
	/*	called when that transmission is done, and it does all the
	/*	associated clean up.
	/*	
	/*----------------------------------------------------------*/

int
g_csnobj(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct obj_data *arg;
{
	op->result = OP_SUCCESS;		/* set result for application*/
	db_free((char *)arg->flattened, arg->len + sizeof(long));
						/* free the sent data */
	db_free((char *)arg, sizeof(struct obj_data));	/* free the state */
							/* structure */
	return OP_COMPLETE;
}


/************************************************************************/
/*	
/*			receive_object 
/*	
/*	Synchronous form of start_receiving_object.  Returns either
/*	OP_CANCELLED, or OP_RESULT(op).
/*	
/*	Calls asynchronous start_receiving_object, then waits for
/*	it to complete.
/*	
/************************************************************************/

int
receive_object(con, objp, type)
CONNECTION con;
char *objp;
int type;
{
	register OPERATION op;
	register int retval;

       /*
        * Issue the operation and wait for it to complete
        */
	op = create_operation();
	start_receiving_object(op, con, objp, type);
	(void) complete_operation(op);

       /*
        * Analyze result, clean up, and return to caller.
        */
	if (OP_STATUS(op) == OP_COMPLETE)
		retval =  OP_RESULT(op);
	else
		retval = OP_STATUS(op);
	delete_operation(op);
	return retval;
}

/************************************************************************/
/*	
/*			start_receiving_object
/*	
/*	Start the asynchronous receipt of a gdb object.  Note that this
/*	routine must be passed the address of the object, not the object
/*	itself.  In the case of structured objects, this routine may 
/*	allocate the necessary storage.  The work to build the object is
/*	done by the object's decode routine.
/*	
/*	The following four routines work together, and may be
/*	considered as a single entity implementing the operation.  The
/*	first merely saves away its arguments and queues the operation
/*	on the designated connection.  These stay there until they
/*	percolate to the head of the queue.  The second is the
/*	initialization routine, which is called by the connection
/*	maintenance logic when the operation first reaches the head of
/*	the queue.  This routine initiates a read for the length of
/*	the object, and sets up a continuation routine to read the
/*	object itself.  When the object itself has been read, a second
/*	continuation routine decodes it and the operation completes.
/*	
/*	The data is preceded by its length expressed as a long in 
/*	network byte order.
/*	
/************************************************************************/
/************************************************************************/
/*
/*			preempt_and_start_receiving_object 
/*	
/*	Similar to above, but may be called only from an active operation
/*	(i.e. an init or continue routine) on an inbound half connection.
/*	The receive effectively pre-empts the old operation, which wil
/*	continue after the receive is done.
/*	
/*	Note: the preempt and non-preempt forms share init and continuation
/*	routines, so they are packaged together here.
/*	
/*	
/************************************************************************/

struct robj_data {
	char 	*objp;				/* address of the object to */
						/* be received */
	int	type;				/* type code for the object */
						/* to be received */
	char    *flattened;			/* address of first byte */
						/* of flattened data */
	int	len;				/* length of the flattened */
						/* data */
};

int g_ircobj();
int g_c1rcobj();
int g_c2rcobj();

	/*----------------------------------------------------------*/
	/*	
	/*		start_receiving_object
	/*	
	/*----------------------------------------------------------*/

int
start_receiving_object(op, con, objp, type)
OPERATION op;
CONNECTION con;
char *objp;
int type;
{
	struct robj_data *arg;

       /*
        * Make sure the supplied connection is a legal one
        */
	GDB_CHECK_CON(con, "start_receiving_object")
	GDB_CHECK_OP(op, "start_receiving_object")

       /*
        * Allocate a place to save our state, fill that in
        */
	arg = (struct robj_data *)db_alloc(sizeof(struct robj_data));
	arg->objp = objp;
	arg->type = type;

       /*
        * Queue us as an operation at the tail of the inbound queue
        */
	initialize_operation(op, g_ircobj, (char *)arg, (int (*)())NULL);
	(void) queue_operation(con, CON_INPUT, op);
}

	/*----------------------------------------------------------*/
	/*	
	/*		preempt_and_start_receiving_object
	/*	
	/*----------------------------------------------------------*/

int
preempt_and_start_receiving_object(op, oldop, objp, type)
OPERATION op;
OPERATION oldop;
char *objp;
int type;
{
	struct robj_data *arg;

       /*
        * Make sure the supplied connection is a legal one
        */
	GDB_CHECK_OP(op, "preempt_and_start_receiving_object")
	GDB_CHECK_OP(oldop, "preempt_and_start_receiving_object")

       /*
        * Allocate a place to save our state, fill that in
        */
	arg = (struct robj_data *)db_alloc(sizeof(struct robj_data));
	arg->objp = objp;
	arg->type = type;

       /*
        * Queue us as an operation at the HEAD of the inbound queue
        */
	initialize_operation(op, g_ircobj, (char *)arg, (int (*)())NULL);
	(void) g_preempt_me(oldop, op);
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_ircobj
	/*	
	/*	Initialization routine for receiving an object.  
	/*	Called when the receive operation percolates to the
	/*	top of the queue.  First, we must receive the single
	/*	'long' which carries the length of the rest of the data.
	/*	We do that now, either synchronously or asynchronously.
	/*	
	/*----------------------------------------------------------*/

int
g_ircobj(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct robj_data *arg;
{
	op->fcn.cont = g_c1rcobj;
	if(gdb_receive_data(hcon, (char *)&(arg->len), sizeof(long)) == OP_COMPLETE) {
		return g_c1rcobj(op, hcon, arg);/* this return is a little */
						/* subtle.  As continuation */
						/* routines call each other */
						/* synchronously, the last */
						/* one determines whether we */
						/* completed or are still */
						/* running.  That status */
						/* percolates back through */
						/* the entire call chain. */
	} else {
		return OP_RUNNING;
	}
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_c1rcobj
	/*	
	/*	At this point, we have received the length.  Now, allocate
	/*	the space for the rest of the data, and start receiving
	/*	it.
	/*	
	/*----------------------------------------------------------*/

int
g_c1rcobj(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct robj_data *arg;
{
#ifdef  vax
	extern u_long ntohl();
#endif  vax
	
       /*
        * Now we know the length of the encoded data, convert the length
        * to local byte order, and allocate the space for the receive.
        */
	arg->len = (int) ntohl((u_long)arg->len);
	arg->flattened = db_alloc(arg->len);

       /*
        * Now start receiving the encoded object itself.  If it all comes in
        * synchronously, then just go on to the c2 routine to decode it and
        * finish up.  Else return OP_RUNNING, so the rest of the system 
        * can get some work done while we wait.
        */
	op->fcn.cont = g_c2rcobj;
	if(gdb_receive_data(hcon, arg->flattened, arg->len ) == OP_COMPLETE) {
		return g_c2rcobj(op, hcon, arg);
	} else {
		return OP_RUNNING;
	}
}

	/*----------------------------------------------------------*/
	/*	
	/*		      g_c2rcobj
	/*	
	/*	At this point, all the data has been received.  Decode
	/*	it into the place provided by the caller, free all
	/*	temporarily allocated memory, and return.
	/*	
	/*----------------------------------------------------------*/

int
g_c2rcobj(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct robj_data *arg;
{
       /*
        * Decode the received data into local representation.
        */
	FCN_PROPERTY(arg->type, DECODE_PROPERTY)
	                  (arg->objp, hcon, arg->flattened);
	op->result = OP_SUCCESS;
	db_free(arg->flattened, arg->len);	/* free the received data */
	db_free((char *)arg, sizeof(struct robj_data));	/* free the state */
							/* structure */
	return OP_COMPLETE;
}

/************************************************************************/
/*	
/*			complete_operation
/*	
/*	Wait for a given operation to complete, allowing everything
/*	to progress in the meantime.  Returns the last known status
/*	of the operation, which in general will be OP_COMPLETE unless
/*	errors were encountered.
/*	
/*	We do this by (1) calling gdb_progress to assure that all
/*	possible progress has been made, which is always a good thing
/*	to do when we get the chance and (2) looping on calls to 
/*	con_select, which will make all possible future progress, 
/*	but without burning cycles unnecessarily in the process.
/*	
/************************************************************************/

int
complete_operation(op)
OPERATION op;
{
	(void) gdb_progress();

	while(op->status != OP_COMPLETE && op->status != OP_CANCELLED)
		(void) con_select(0, (fd_set *)NULL, (fd_set *)NULL,
			   (fd_set *)NULL, (struct timeval *)NULL);

	return op->status;

}


/************************************************************************/
/*	
/*			cancel_operation
/*	
/*	Attempts to cancel an operation.  At the time of this writing,
/*	there are no instances of cancellable operations, so this
/*	routine will generally fail to cancel any operation which
/*	has reached the front of the queue.  Many operations also
/*	currently fail to reclaim their state when cancelled, which can
/*	lead to loss of malloc'd memory.
/*	
/************************************************************************/

int
cancel_operation(op)
OPERATION op;
{
	register HALF_CONNECTION hcon = op->halfcon;

	if (op->status != OP_RUNNING && op->status != OP_QUEUED)
		return op->status; 

	if (hcon == NULL)
		GDB_GIVEUP("cancel_operation: operation is queued but half connection is unknown")

       /*
        * If we're at the head of the queue and running, then we have to
        * call the cancelation routine for this particular operation so
        * it can clean up.  POSSIBLE BUG: it may be that we should also
        * call this when the connection is queued but not at the front.
        */
	if (op->prev == (OPERATION)hcon) {
		if (op->status == OP_RUNNING && op->cancel != NULL)
			(*op->cancel)(op->halfcon, op->arg);
	}

       /*
        * Looks safe, now cancel it.
        */
	op->next->prev = op->prev;		/* de-q it */
	op->prev->next = op->next;		/* "  "  " */
	op->status = OP_CANCELLED;
	op->halfcon = NULL;

	return OP_CANCELLED;
}

/************************************************************************/
/*	
/*			start_listening
/*	
/*	Start the asynchronous acquisition of a connection.  This
/*	results in the queuing of a GDB "OPERATION" to do the
/*	requested listening.
/*	
/*	This is a queuable operation, but it is normally issued only
/*	from start_accepting_client_.  The distinction is that this
/*	routine JUST accepts the connection, while
/*	start_accepting_client also receives the startup tuple from
/*	the client.
/*	
/************************************************************************/

/*
 * Structure holding our state during this asynchronous operation
 */
struct lis_data {
	char    *otherside;			/* data returned from an */
						/* accept */
	int	*otherlen;			/* length of the otherside */
						/* field */
	int	*fdp;				/* ptr to the fd of the */
					 	/* newly accepted */
						/* connection */
};

int g_ilis();
int g_clis();

int
gdb_start_listening(op, con, otherside, lenp, fdp)
OPERATION op;
CONNECTION con;
char *otherside;
int  *lenp;
int  *fdp;
{
	struct lis_data *arg;

	GDB_INIT_CHECK

       /*
        * Make sure the supplied listening connection and op are legal
        */
	GDB_CHECK_CON(con, "start_listening")
	GDB_CHECK_OP(op, "start_listening")

       /*
        * Allocate and fill in structure to hold our state
        */
	arg = (struct lis_data *)db_alloc(sizeof(struct lis_data));
	arg->otherside = otherside;
	arg->otherlen = lenp;
	arg->fdp = fdp;

       /*
        * Queue the async operation which will really accept the connection.
        * This will delay (without blocking GDB) until a client actually
        * attempts a connection.
        */
	initialize_operation(op, g_ilis, (char *)arg, (int (*)())NULL);
	(void) queue_operation(con, CON_INPUT, op);
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_ilis
	/*	
	/*	Init routine for doing a listen (really an accept).
	/*	
	/*----------------------------------------------------------*/

int
g_ilis(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct lis_data *arg;
{
	int rc;

       /*
        * Set up continuation routine in case it's needed after the return
        */
	op->fcn.cont = g_clis;

       /*
        * Try doing the accept now, and then decide whether to go
        * right on to the continuation routine or to let things hang
        * for the moment.  If we don't complete here, then the con_select 
        * and hcon_progress logic will retry and call the continuation
        * routine when a client does in fact arrive.
        */
	rc = gdb_try_an_accept(hcon, arg->otherside, arg->otherlen, arg->fdp);
	if (rc==OP_COMPLETE) {		
		return g_clis(op, hcon, arg);	/* this return is a little */
						/* subtle.  As continuation */
						/* routines call each other */
						/* synchronously, the last */
						/* one determines whether we */
						/* completed or are still */
						/* running.  That status */
						/* percolates back through */
						/* the entire call chain. */
	} else {
		return OP_RUNNING;
	}
}


	
	/*----------------------------------------------------------*/
	/*	
	/*	                 g_clis
	/*	
	/*	Continuation routine for accepting a connection.
	/*
	/*	At this point, the fd has been accepted and all
	/*      the necessary information given back to the caller.
	/*	
	/*----------------------------------------------------------*/

int
g_clis(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct lis_data *arg;
{
	op->result = OP_SUCCESS;		
	db_free((char *)arg, sizeof(struct lis_data));	
						/* free the state structure */
	return OP_COMPLETE;
}


/************************************************************************/
/*	
/*			start_accepting_client
/*	
/*	Start the asynchronous acquisition of a client.  
/*	
/*	Unlike start_listening, this operation is called directly from
/*	applications to do the work of asynchronously making a
/*	connection to the next client trying to reach this server.
/*	This queueable operation queues ahead of itself a
/*	start_listen, which will asynchronously acquire the physical
/*	connection to the client.  Once we regain control here, we
/*	verify compatible protocol levels by exchanging protocol i.d.
/*	bytes, and put the connection into non-blocking mode (it
/*	actually may start non-blocking because it is accepted on a
/*	non-blocking socket, but the verify protocol routines,
/*	unfortunately, put it back into blocking mode to avoid spin
/*	looping.)  Finally, we re-queue ourselves onto the inbound
/*	half of the new connection (which means that we are now
/*	blocked on its activity, not that of the listening connection)
/*	and use the new connection to read a startup string from the
/*	client, and then complete.
/*	
/*	The return values from this are not quite what you might expect.
/*	In general, the operation will show complete, rather than cancelled,
/*	if it gets as far as creating the new connection at all.  If
/*	subsequent activities result in errors from system calls, then
/*	this operation will complete with a status of OP_COMPLETE and a 
/*	result of OP_CANCELLED.  In this case, the applications IS given
/*	a connection descriptor for the new connection, and that descriptor
/*	has an errno value indicating why the failure occurred.  The 
/*	caller must then sever this connection to free the descriptor.
/*	
/************************************************************************/

struct acc_data {
	char    *otherside;			/* data returned from an */
						/* accept */
	int	*otherlen;			/* length of the otherside */
						/* field */
	OPERATION listenop;			/* used to listen for */
						/* the fd */
	OPERATION receiveop;			/* used when receiving */
						/* tuple from the client */
	CONNECTION con;				/* the connection we're */
						/* trying to create */
	CONNECTION *conp;			/* this is where the caller */
						/* wants the connection */
						/* returned */
	TUPLE *tuplep;				/* pointer to tuple we */
						/* are going to receive */
						/* from new client */
};

int g_iacc();
int g_i2acc();

int
start_accepting_client(listencon, op, conp, otherside, lenp, tuplep)
CONNECTION listencon;
OPERATION op;
CONNECTION *conp;
char *otherside;
int  *lenp;
TUPLE *tuplep;
{
	struct acc_data *arg;

	GDB_INIT_CHECK

       /*
        * Make sure the supplied connection and operation are legal
        */
	GDB_CHECK_CON(listencon, "start_accepting_client")
	GDB_CHECK_OP(op, "start_accepting_client")

       /*
        * Allocate and fill in the structure which will hold our
        * state while attempting to accept the client.
        */
	arg = (struct acc_data *)db_alloc(sizeof(struct acc_data));
	arg->otherside = otherside;
	arg->otherlen = lenp;
	arg->conp = conp;
	*conp = NULL;				/* in case we fail */
	arg->listenop = create_operation();
	arg->receiveop = create_operation();
	arg->con = g_make_con();
	arg->tuplep = tuplep;
	*tuplep = NULL;				/* in case we fail */

       /*
        * Queue an operation ahead of us which will accept an fd and
        * put it in arg->con->in.  As a byproduct, pick up the from
        * information that we return to the caller.
        */
	gdb_start_listening(arg->listenop, listencon,
			    arg->otherside, 
			    arg->otherlen, &(arg->con->in.fd));
	
       /*
        * Now queue us behind it.  By the time we run our init routine,
        * a connection should have been acquired.
        */
	initialize_operation(op, g_iacc, (char *)arg, (int (*)())NULL);
	(void) queue_operation(listencon, CON_INPUT, op);
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_iacc
	/*	
	/*	Init routine for accepting a connection.  By the 
	/*	time this runs, the accept has been done, the 
	/*	'from' data put in position for the caller, and
	/*	the fd plugged into the connection descriptor.
	/*	If all went well, fill out the connection descriptor
	/*	and then requeue us on that to do the receive of
	/*	the requested tuple.
	/*	
	/*----------------------------------------------------------*/

int
g_iacc(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct acc_data *arg;
{
	register CONNECTION con = arg->con;

       /*
        * Set up 2nd init routine for after we re-queue ourselves
        */
	op->fcn.cont = g_i2acc;
 
       /*
        * See whether we successfully accepted a connection.  If
        * not, we just cancel ourselves.  If so, fill out the
        * connection descriptor and related data structures properly,
        * then requeue ourselves on the new connection.
        */
	if (OP_STATUS(arg->listenop) != OP_COMPLETE ||
	    OP_RESULT(arg->listenop) != OP_SUCCESS ||
	    con->in.fd <=0) {
		    (void) sever_connection(con);
		    g_clnup_accept(arg);
		    op->result = OP_CANCELLED;
		    return OP_CANCELLED;
	}

       /*
        * OK, we got an fd, but the connection and related structures 
        * aren't really set up straight, and the fd must be put
        * into non-blocking mode.  There really should be a common
        * routine for this, since some of the logic exists in 2
        * or 3 places.
        */
	con->status = CON_STARTING;
	con->out.fd = con->in.fd;
	g_ver_iprotocol(con);			/* make sure we're at */
						/* same level of protocol */
	if (con->status == CON_UP) {
               /*
                * We've successfully started the connection, now mark
                * it for non-blocking I/O.  Also, update the high water
                * mark of fd's controlled by our system.
                */
		int nb = 1;
		if(ioctl(con->in.fd, FIONBIO, (char *)&nb)== (-1)) {
			g_stop_with_errno(con);
			*arg->conp = con;	/* give failed con to */
						/* caller so he can find */
						/* errno */
			gdb_perror("gdb: ioctl for non-block failed");
			g_clnup_accept(arg);
			op->result = OP_CANCELLED; 
			return OP_COMPLETE;	/* we didn't really, but */
						 /* we want caller to look */
						 /* at the connection so he */
						 /* can find errno*/
		}
		if (con->in.fd +1 > gdb_mfd)	/* highest con in use for */
			gdb_mfd = con->in.fd + 1; /* use by select routines */
               /* 
                * Allocate an input buffer, if necessary, and reset
                * buffer pointers so first request will result in a
                * long read into the buffer
                */
		g_allocate_connection_buffers(con);

	} else {
		*arg->conp = con;		/* give failed con to */
						/* caller so he can find */
						/* errno */
		g_clnup_accept(arg);
		op->result = OP_CANCELLED;
		return OP_COMPLETE;
	}

       /*
        * Before we requeue ourselves on the new connection, queue
        * up a receive for the expected tuple.  Then we'll be 
        * sure that it's there by the time we run.  We'll show up
        * after the receive in g_i2acc, our continuation routine.
        */
	start_receiving_object(arg->receiveop, con, (char *)(arg->tuplep),
			       TUPLE_T);
       /*
        * Requeue ourselves behind the receive operation.
        */

	(void) requeue_operation(con, CON_INPUT, op);
	return OP_REQUEUED;
}


	
	/*----------------------------------------------------------*/
	/*	
	/*			g_i2acc
	/*	
	/*	Second init routine for accepting a connection. 
	/*	This one is run after the operation is requeued on
	/*	the new connection.  By the time we run here, the
	/*	attempt to receive the tuple has already been made.
	/*	We just check on status and clean-up.
	/*	
	/*----------------------------------------------------------*/

int
g_i2acc(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct acc_data *arg;
{
	int rc;

	rc = OP_STATUS(arg->receiveop);		/* if it completes, then */
						/* so do we! */
	*arg->conp = arg->con; 			/* give caller the new con */
	if (rc != OP_COMPLETE) 
		(void) g_stop_connection(arg->con);
       /*
        * Release all transient data structures.
        */
	g_clnup_accept(arg);
	
	return OP_COMPLETE;
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_clnup_accept
	/*	
	/*	Free all data structures used by start_accepting_client.
	/*	
	/*----------------------------------------------------------*/

int
g_clnup_accept(arg)
struct acc_data *arg;
{
	delete_operation(arg->listenop);
	delete_operation(arg->receiveop);
	db_free((char *)arg, sizeof(struct acc_data));	
}
