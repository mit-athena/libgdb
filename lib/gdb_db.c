/************************************************************************/
/*	
/*				gdb_db.c
/*	
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_db.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_db.c,v 1.1 1993-10-12 03:25:10 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*	This source file defines the routines used to access a
/*	relational database.  These represent the top-most layer of
/*	GDB, and they depend on all of the other GDB layers.  The
/*	counterpart to the functions found here is the code in
/*	dbserv.qc, which is the Ingres implementation of a GDB server.
/*	
/*	Most of these routines are implemented as asynchronous services.
/*	The synchronous versions simply call the async, then await
/*	completion.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_gdb_db_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_db.c,v 1.1 1993-10-12 03:25:10 probe Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include <strings.h>
#include "gdb.h"
#ifdef HESIOD
#include <hesiod.h>
#endif HESIOD



/************************************************************************/
/*	
/*				start_accessing_db 
/*	
/*	Used by an application to establish access to a specified
/*	remote database.  This routine is an asynchronous GDB operation.
/*	
/*	1) Creates a new db structure to describe the database.
/*	
/*	2) Parses the supplied db_ident into a server and a db name
/*	
/*	3) Opens a connection to the server, sending the name of the 
/*	   database as an argument
/*	
/*	4) Asynchronously receives a return code from the server
/*	   to indicate whether the database could be accessed
/*	
/*	5) If successful, the handle on the new structure is placed into
/*	   the variable supplied by the caller.  Otherwise, the structure
/*	   is de_allocated and failure status is returned to the caller.
/*	
/*	Note: this code was adapted from an earlier synchronous 
/*	      implementation and there may yet be some loose ends.
/*	
/************************************************************************/

#define MIN_NAME_LEN 1				/* used to catch suspicously */
						/* short nams */

DATABASE g_make_db();
int      g_iadb();

/*
 * This structure contains the state of the ongoing operation.  It
 * maps the *arg variable.
 */
struct adb_data {
	DATABASE db;
	OPERATION get_retcode;
};

int
start_accessing_db (op, db_ident, db_handle)
OPERATION op;
char *db_ident;
DATABASE *db_handle;               
{
	register DATABASE db;			/* the newly created */
						/* structure */
	register struct adb_data *arg;		/* holds our state */
						/* during async operation*/
	char *server;				/* pointer to server:port */
	char *db_name;				/* Ingres name for the db */

	CONNECTION connexn;			/* the connection to the */
						/* database server*/

	/*----------------------------------------------------------*/
	/*	
	/*	Make sure parameters are correct, then allocate a
	/*	structure.
	/*	
	/*----------------------------------------------------------*/

	GDB_INIT_CHECK

	db_name = NULL;
	server = NULL;

	GDB_CHECK_OP(op, "start_accessing_db")

	/*----------------------------------------------------------*/
	/*	
	/*	parse the supplied database identifier, producing
	/*	a string named server in the form "host:port"
	/*	and a string db_name with the Ingres database name.
	/*	
	/*----------------------------------------------------------*/

	if ((db_ident == NULL)|| (strlen(db_ident)<MIN_NAME_LEN)) {
		fprintf (gdb_log,"access_db: correct syntax is db_name@server -or- hesiod_db_name\n");
		*db_handle = NULL;
		OP_STATUS(op) = OP_CANCELLED;
		return (OP_CANCELLED);
	}

	if (!g_parse_db_ident(db_ident, &server, &db_name)) {
		*db_handle = NULL;
		OP_STATUS(op) = OP_CANCELLED;
		return (OP_CANCELLED);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Make a database descriptor and fill in basic fields
	/*	
	/*----------------------------------------------------------*/

	db = g_make_db();
        *db_handle = db;
        db->name = db_name; 				 
	db->server = server;

	/*----------------------------------------------------------*/
	/*	
	/*	Create a connection to the server.
	/*	
	/*----------------------------------------------------------*/

	connexn = start_server_connection (server, db_name);

	if (connexn==NULL || connection_status(connexn) != CON_UP) {
	     connection_perror(connexn, "Error starting server connection");
	     fprintf (gdb_log, "gdb:access_db: couldn't connect to server %s \n", server);
	     g_tear_down(*db_handle);
	     OP_STATUS(op) = OP_CANCELLED;
	     return (OP_CANCELLED);
	}

        db->connection = connexn;		/* for permanent use */
						/* in accessing the db */

       /*
        * Start asynchronously receiving the return code from the 
        * data base server.  May take awhile, since Ingres is so
        * slow to start up.  We do it asynchronously, so application
        * won't block.  Note: return code is received into the 
        * arg structure, which persists after we return to our caller.
        * The g_iadb routine is scheduled to run once the return code
        * is available, and that routine inspects it.
        */

	arg = (struct adb_data *)db_alloc(sizeof(struct adb_data));
	arg->get_retcode = create_operation();
	arg->db = db;

	start_receiving_object (arg->get_retcode, connexn, 
				(char *)&(OP_RESULT(op)), INTEGER_T);	
       /*
        * Error handling (if we couldn't get started receiving, or if
        * it blew up fast enough for us to see it here.)
        */
	if (OP_STATUS(arg->get_retcode) == OP_CANCELLED) {
	    g_tear_down (*db_handle);
	    OP_STATUS(op) = OP_CANCELLED;
	    delete_operation(arg->get_retcode);
	    db_free((char *)arg, sizeof(struct adb_data));
	    return OP_CANCELLED;
        }

       /*
        * We've successfully queued the receive of the return code.
        * That's about all we have to do if things go well, but if the
        * operation fails later, we have to be there to clean up.  To
        * get control back, we queue ourselves as a second operation
        * so we can see how the first did, and so we can free up arg.
        */
	initialize_operation(op, g_iadb, (char *)arg, (int (*)())NULL); 
	(void) queue_operation(connexn, CON_INPUT, op);

	return OP_RUNNING;		
}


	/*----------------------------------------------------------*/
	/*	
	/*			g_parse_db_ident
	/*	
	/*	Based on the db_identifier, determine the host,
	/*	service name, and database name.
	/*	
	/*----------------------------------------------------------*/

int 
g_parse_db_ident(db_ident, serverp, dbnamep)
char *db_ident;					/* db i.d. as spec'd by */
						/* user*/
char **serverp;					/* we allocate string */
						/* with host:port here */
char **dbnamep;					/* we allocate string */
						/* with Ingres dbname */
						/* here */
{
	char db_name[100];			/* name of database */
	char host[100];				/* host name */
	char service[100];			/* service name */
	char dbtype[100];			/* should be "gdb" */
	char proto_type[100];			/* protocol family */
	char **hes_result;			/* strings returned by */
						/* hesiod*/
	register int count;			/* number of fields parsed */
	register char *at_point;		/* where the @ is */
	char *colon_point;			/* where the : is */

       /*
        * Try to parse as dbname@host
        */
	at_point = index(db_ident, '@');

       /*
        * If we got an @, then it is fully qualified, otherwise
        * call hesiod to resolve it.  Note: we cannot change base string
        * 
        * In either case, we develop the three fields: host, service, db_name
        */
	
	if (at_point != NULL) {			/* fully qualified */
		(void) strncpy(db_name, db_ident, at_point-db_ident);
		*(db_name+(at_point-db_ident)) = '\0';
		(void) strcpy(host, at_point+1);
		colon_point = index(host, ':');
		if (colon_point != NULL) {	/* explicit service */
			(void) strcpy(service, colon_point+1);
			*colon_point = '\0';
		} else				/* default service */
			(void) strcpy(service, GDB_DB_SERVICE);	
	} else {         			/* hesiod */
#ifdef HESIOD
		hes_result = hes_resolve(db_ident, GDB_HESIOD_NAMETYPE);
		if (hes_result == NULL) {
			fprintf(gdb_log, "GDB database name %s could not be resolved by hesiod\n", db_ident);
			return FALSE;
		}
		count = sscanf(*hes_result, "%s %s %s %s %s", dbtype, 
			       proto_type, host, service, db_name);
		if (count != 5) {
			fprintf(gdb_log, "Hesiod returned illegal string \"%s\"\nfor requested database \"%s\"\n", *hes_result, db_ident);
			return FALSE;
		}
#else !HESIOD
		fprintf(gdb_log, "GDB built without hesiod support but database name %s does\nnot specify a host\n", db_ident);
		return FALSE;
#endif !HESIOD
	}

       /*
        * Now that we've got the pieces, put them in the form required
        * for starting a connection
        */

	*serverp = (char *)db_alloc(strlen(host) + strlen(service) +2);
	(void) sprintf(*serverp, "%s:%s", host, service);
	*dbnamep = (char *)db_alloc(strlen(db_name)+1);
	(void) strcpy(*dbnamep, db_name);
	return TRUE;
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_iadb
	/*	
	/*	Init routine for getting return code on accessing a
	/*	database.  If all went well, (or even if it didn't), then
	/*	we are done.  All we have to do is clean up the stuff we've
	/*	allocated.
	/*	
	/*----------------------------------------------------------*/

int
g_iadb(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct adb_data *arg;
{
	int rc;

       /*
        * Figure out how the receipt of the return code went
        */
	rc = OP_STATUS(arg->get_retcode);

       /*
        * Release all transient data structures.
        */
	if (rc != OP_COMPLETE || op->result != DB_OPEN)
		g_tear_down(arg->db);
	else
	        DB_STATUS(arg->db) = DB_OPEN;
	  
	delete_operation(arg->get_retcode);
	db_free((char *)arg, sizeof(struct adb_data));
	
	return rc;
}

/************************************************************************/
/*	
/*			g_tear_down
/*	
/*	This is called by access_db and perf_db_op when a fatal error 
/*	is reached.  This version simply tears down everything, and
/*	severs the connection.  The server will detect loss of connection
/*	and do cleanup at its end.
/*	 
/************************************************************************/

int
g_tear_down (db_handle)
DATABASE db_handle;
{
	register DATABASE db = db_handle; 

       /*	
	* If the db is opened, and the connexn is severed, 
	* some error handling, closing of the db should be done 
	* at the server.
	*	
	* Also, at the server, perhaps a return code to indicate
	* that user tried to open non-existant db???
	*/
	if (db==NULL)
	        return;
       	(void) sever_connection (db->connection);

       /*
        * Free up the separately allocated strings to which the
        * database descriptor points
        */
	gdb_fstring(db->server);
	gdb_fstring(db->name);

       /*
        * Free the descriptor itself
        */
	db_free ((char *)db,sizeof(struct db_struct));
	return;
}

/************************************************************************/
/*	
/*			  g_make_db
/*	
/*	Allocate and initialize a database descriptor structure.
/*	
/************************************************************************/

DATABASE
g_make_db()
{
	register DATABASE db;

	db = (DATABASE)db_alloc (sizeof(struct db_struct));
	db->id = GDB_DB_ID;
	db->connection = NULL;
	db->name = NULL;
	db->server = NULL;
	DB_STATUS(db) = DB_CLOSED;
	
	return db;
}
/************************************************************************/
/*	
/*				access_db 
/*	
/*	Used by applications to synchronously access a database.
/*	Does a start_accessing_db and waits for it to complete.
/*	
/************************************************************************/

int
access_db (db_ident, db_handle)
char *db_ident;
DATABASE *db_handle;               
{
	register OPERATION op;
	register int status;
	register int result;

	GDB_INIT_CHECK

       /*
        * Create an operation and use it to asynchronously access
        * the database
        */
	op = create_operation();
	(void) start_accessing_db(op, db_ident, db_handle);

       /*
        * Wait for it to complete, note whether the operation completed
        * at all, and if so, whether it returned a successful result
        * in accessing the database.  Then reclaim the space used for
        * the operation.
        */
	(void) complete_operation(op);
	status = OP_STATUS(op);
	result = OP_RESULT(op);

	delete_operation(op);

       /*
        * Tell the caller either that we were interrupted, or pass
        * on the actual result of accessing the database.  If it
        * failed, then tear everything down after all.
        */
	if (status==OP_COMPLETE)
		return result;
	else
		return status;
}
/************************************************************************/
/*	
/*		  	start_performing_db_operation
/*	
/*	Asynchronously performs  any operation except for a query
/*	on the remote database.
/*	
/*	The operation is encoded as a GDB string and sent to the server.
/*	
/*	An integer return code is received back and returned to the caller.
/*	
/*	Note that this operation executes on both the outbound and inbound
/*	half connections.  Since there is no explicit sync between the two
/*	directions, operations like this pipeline freely from requestor
/*	to server, but there is no way to cancel this operation once it
/*	has started without severing the accompanying connection.
/*	
/************************************************************************/

int g_ipdb();

/*
 * The state of our ongoing operation.  This is passed from here to the
 * init routine and on to the continuation routines.  It maps the *arg
 * variable.
 */
struct pdb_data {
	DATABASE db;				/* the database we're */
						/* working on */
	OPERATION send_request;			/* used to send the string */
						/* containing the db oper. */
						/* to be performed */
	OPERATION get_retcode;			/* used to get back the */
						/* response to our request */
	STRING s;				/* the operation string */
						/* itself.  This is sent. */
};

#define MIN_REQUEST_LEN 1			/* used to catch */
						/* suspiciously short */
						/* requests */

int
start_performing_db_operation (op, db_handle,request)
OPERATION op;
DATABASE db_handle; 
char *request;
{
	register struct pdb_data *arg;		/* holds our state */
						/* during async operation*/
	register DATABASE db = db_handle;	/* fast working copy */

	/*----------------------------------------------------------*/
	/*	
	/*	Make sure parameters are valid
	/*	
	/*----------------------------------------------------------*/

	GDB_CHECK_OP(op, "start_performing_db_operation ")
        if (db==NULL) {
		fprintf (gdb_log, "gdb: start_performing_db_operation: supplied database is NULL\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

	GDB_CHECK_DB(db, "start_performing_db_operation")

	if (DB_STATUS(db) != DB_OPEN) {
		fprintf (gdb_log, "gdb: start_performing_db_operation: request to closed database ");
		OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
	}

        if (db->connection == NULL) {
                fprintf (gdb_log,
			 "gdb: start_performing_db_operation: connection severed, request cancelled\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

        if (connection_status(db->connection) != CON_UP ) {
                fprintf (gdb_log, "gdb: start_performing_db_operation: problems maintaining connection ");
		connection_perror(db->connection, "Reason for connection failure");
		fprintf (gdb_log, "request cancelled \n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

        if ((request == NULL) || (strlen (request)<MIN_REQUEST_LEN)) { 
                fprintf (gdb_log, "gdb: start_performing_db_operation: request either missing or too short\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }


	/*----------------------------------------------------------*/
	/*	
	/*	Asynchronously send the request to the server
	/*	
	/*----------------------------------------------------------*/

       /*
        * Allocate a structure to hold our state while we're gone
        * waiting for this to complete.
        */

	arg = (struct pdb_data *)db_alloc(sizeof(struct pdb_data));
	arg->db = db;
	arg->send_request = create_operation();

       /*
        * Send the request string to the server
        */
	STRING_DATA(arg->s) = request;
	MAX_STRING_SIZE(arg->s) = strlen (request) +1;
	start_sending_object (arg->send_request, db->connection, 
				(char *)&(arg->s), STRING_T);	
	if (OP_STATUS(arg->send_request) == OP_CANCELLED) {
	    OP_STATUS(op) = OP_CANCELLED;
	    delete_operation(arg->send_request);
	    db_free((char *)arg, sizeof(struct pdb_data));
	    return OP_CANCELLED;
        }

	/*----------------------------------------------------------*/
	/*	
	/*	Asynchronously receive the return code (note, we
	/*	really don't know whether the request has even been
	/*	sent yet...doesn't really matter.)
	/*	
	/*----------------------------------------------------------*/


	arg->get_retcode = create_operation();

       /*
        *  The initialize_operation immediately below must be here, before
        *  the start_receiving_object.  Reason: both set op->result, 
        *  and there would be a race if the two were in the other order.
        */
	initialize_operation(op, g_ipdb, (char *)arg, (int (*)())NULL);
	start_receiving_object (arg->get_retcode, db->connection, 
				(char *)&(OP_RESULT(op)), INTEGER_T);	
       /*
        * Check for early failure of our attempt to receive return code
        */
	if (OP_STATUS(arg->get_retcode) == OP_CANCELLED) {
		OP_STATUS(op) = OP_CANCELLED;
		(void) cancel_operation(arg->send_request);
						/* this could be a bug, */
						/* because we introduce */
						/* indeterminism into */
						/* the reply stream, probably*/
						/* should shutdown the whole */
						/* db here */
		delete_operation(arg->send_request);
		delete_operation(arg->get_retcode);
		db_free((char *)arg, sizeof(struct adb_data));
		return OP_CANCELLED;
        }

       /* 
        * We've successfully queued the receive of the return code.
        * To get control back once the return code is in, we queue
        * ourselves as a second operation so we can see how the first
        * did, and so we can free up arg.
        */
	(void) queue_operation(db->connection, CON_INPUT, op);
	return OP_RUNNING;
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_ipdb
	/*	
	/*	Init routine for getting return code on performing a db
	/*	operation.  If all went well, (or even if it didn't),
	/*	then we are done.  All we have to do is clean up the
	/*	stuff we've allocated.
	/*	
	/*----------------------------------------------------------*/

int
g_ipdb(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct pdb_data *arg;
{
	int rc1, rc2;

       /*
        * Figure out how both the request and the reply went
        */
	rc1 = OP_STATUS(arg->send_request);
	rc2 = OP_STATUS(arg->get_retcode);

       /*
        * Release all transient data structures.
        */
	if (rc1 != OP_COMPLETE || rc2 != OP_COMPLETE)
		g_tear_down(arg->db);
	  
	delete_operation(arg->send_request);
	delete_operation(arg->get_retcode);
	db_free((char *)arg, sizeof(struct pdb_data));
	
	return rc2;
}

/************************************************************************/
/*	
/*			perform_db_operation
/*	
/*	Do a database operation synchronously.  This just calls
/*	the async routine and waits for it to complete.
/*	
/************************************************************************/

perform_db_operation (db_handle,request)
DATABASE db_handle; 
char *request;
{
	register OPERATION op;
	register int status;
	register int result;

       /*
        * Create an OPERATION and use it to asynchronously perform
        * the database operation
        */
	op = create_operation();
	(void) start_performing_db_operation(op, db_handle, request);

       /* 
        * Wait for it to complete, note whether the operation
        * completed at all, and if so, whether it returned a
        * successful result.  Then reclaim the space used for the
        * operation.
        */
	(void) complete_operation(op);
	status = OP_STATUS(op);
	result = OP_RESULT(op);

	delete_operation(op);

       /*
        * Tell the caller either that we were interrupted, or pass
        * on the actual result of accessing the database.  If it
        * failed, then tear everything down after all.
        */
	if (status==OP_COMPLETE)
		return result;
	else
		return status;
}
/************************************************************************/
/*	
/*			  start_db_query
/*	
/*	Asynchronously performs a database query on the remote
/*	database.
/*	
/*	1) The operation is encoded as a GDB string and sent to the server.
/*	
/*	2) An integer return code is received back and returned to the caller.
/*	
/*	3) If the return code indicates success, then we go into a loop
/*	   receiving the retrieved data.  Each returned tuple is preceeded by
/*	   a so-called yes/no flag, which indicates whether tuple data is
/*	   really to follow.  Last tuple is followed by a NO flag.
/*	
/*	Note that this operation executes on both the outbound and inbound
/*	half connections.  Since there is no explicit sync between the two
/*	directions, operations like this pipeline freely from requestor
/*	to server, but there is no way to cancel this operation once it
/*	has started without severing the accompanying connection.
/*	
/*	Suggestion:  for anyone interested in learning to write their
/*	own GDB operations, this is a good example of the kind of 
/*	tricks one can play.  This is NOT the simplest example.
/*	
/************************************************************************/

int g_idbq();
int g_cdbq();

/*
 * 			dbq_data (*arg)
 * 
 * Holds our state throughout all the processing.  Maps *arg.  Passed
 * from init to continuation routines.
 */
struct dbq_data {
       /*
        * Following may be used throughout processing
        */
	DATABASE db;				/* the database we're */
						/* working on */
	RELATION rel;
	TUPLE_DESCRIPTOR tpd;
       /*
        * used primarily in first phase for sending query and getting
        * return code
        */
	OPERATION send_query;			/* used to send the string */
						/* containing the query */
						/* to be performed */
	OPERATION send_descriptor;		/* used to send the tuple */
						/* descriptor to the server */
	OPERATION get_retcode;			/* used to get back the */
						/* response to our request */
	STRING s;				/* the operation string */
						/* itself.  This is sent. */
       /*
        * Following are used during later phase to receive the tuples 
        */
	int state;				/* are we expecting a yes/no */
						/* or a tuple next? */
#define YESNO 1
#define TUPDATA 2
	int yesno;				/* an indicator of whether */
						/* another tuple is to follow*/
#define YES 1
	OPERATION receive_yesno_or_data;
	TUPLE tup;				/* a place to put */
						/* the next tuple */
};

int
start_db_query (op, db_handle,rel, query)
OPERATION op;
DATABASE db_handle; 
RELATION rel;
char *query;
{
	register struct dbq_data *arg;		/* holds our state */
						/* during async operation*/
	register DATABASE db = db_handle;	/* fast working copy */

	/*----------------------------------------------------------*/
	/*	
	/*	Make sure parameters are valid.
	/*	
	/*----------------------------------------------------------*/

	GDB_CHECK_OP(op, "start_db_query ")

        if (rel ==NULL) {
		fprintf (gdb_log, "gdb: query_db: input rel is null \n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

        if (db==NULL) {
		fprintf (gdb_log, "gdb: start_db_query: supplied database is NULL\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

	GDB_CHECK_DB(db, "start_db_query")

	if (DB_STATUS(db) != DB_OPEN) {
		fprintf (gdb_log, "gdb: start_db_query: request to closed database ");
		OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
	}

        if (db->connection == NULL) {
                fprintf (gdb_log,"gdb: start_db_query: connection severed, request cancelled\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

        if (connection_status(db->connection) != CON_UP ) {
                fprintf (gdb_log,"gdb: start_db_query: problems maintaining connection ");
		connection_perror(db->connection, "Reason for connection failure");
		fprintf (gdb_log,"request cancelled \n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }

        if (query == NULL || *query == '\0') { 
                fprintf (gdb_log, "gdb: start_db_query: request string is null\n");
                OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
        }


	/*----------------------------------------------------------*/
	/*	
	/*	Asynchronously send the query to the server
	/*	
	/*----------------------------------------------------------*/

       /*
        * Allocate a structure to hold our state while we're gone
        * waiting for this to complete.
        */

	arg = (struct dbq_data *)db_alloc(sizeof(struct dbq_data));
	arg->db = db;
	arg->rel = rel;
	arg->send_query = create_operation();

       /*
        * Send the query string to the server.  We prepend the verb
        * "retrieve" to the callers QUEL query string.
        */
	(void) string_alloc(&(arg->s), strlen(query)+11);
	(void) strcpy(STRING_DATA(arg->s), "retrieve ");
	(void) strcat(STRING_DATA(arg->s), query);
       	MAX_STRING_SIZE(arg->s) = strlen (query) +11;
	start_sending_object (arg->send_query, db->connection, 
				(char *)&(arg->s), STRING_T);	
	if (OP_STATUS(arg->send_query) == OP_CANCELLED) {
	    OP_STATUS(op) = OP_CANCELLED;
	    delete_operation(arg->send_query);
	    string_free(&(arg->s));
	    db_free((char *)arg, sizeof(struct dbq_data));
	    return OP_CANCELLED;
        }

       /* 
        * Behind the request itself, queue an asynchronous send for
        * the tuple descriptor.  This gives the server info on the
        * names and types of the fields referenced in the query
        * string.
        */
	arg->send_descriptor = create_operation();
	arg->tpd = DESCRIPTOR_FROM_RELATION(arg->rel);

	start_sending_object (arg->send_descriptor, db->connection, 
				(char *)&(arg->tpd), TUPLE_DESCRIPTOR_T);
	if (OP_STATUS(arg->send_descriptor) == OP_CANCELLED) {
	    OP_STATUS(op) = OP_CANCELLED;
	    (void) cancel_operation(arg->send_query);
						/* this could be a bug, */
						/* because we introduce */
						/* indeterminism into */
						/* the reply stream, probably*/
						/* should shutdown the whole */
						/* db here */
	    delete_operation(arg->send_query);
	    delete_operation(arg->send_descriptor);
	    string_free(&(arg->s));
	    db_free((char *)arg, sizeof(struct dbq_data));
	    return OP_CANCELLED;
        }

	/*----------------------------------------------------------*/
	/*	
	/*	Asynchronously receive the return code (note, we
	/*	really don't know whether the query/and the descriptor
	/*	have even been sent yet...doesn't really matter.)
	/*	
	/*----------------------------------------------------------*/

	arg->get_retcode = create_operation();
	start_receiving_object (arg->get_retcode, db->connection, 
				(char *)&(OP_RESULT(op)), INTEGER_T);	
	if (OP_STATUS(arg->get_retcode) == OP_CANCELLED) {
	    OP_STATUS(op) = OP_CANCELLED;
	    (void) cancel_operation(arg->send_query);
						/* this could be a bug, */
						/* because we introduce */
						/* indeterminism into */
						/* the reply stream, probably*/
						/* should shutdown the whole */
						/* db here */
	    (void) cancel_operation(arg->send_descriptor);
	    string_free(&(arg->s));
	    delete_operation(arg->send_query);
	    delete_operation(arg->send_descriptor);
	    delete_operation(arg->get_retcode);
	    db_free((char *)arg, sizeof(struct adb_data));
	    return OP_CANCELLED;
        }


	/*----------------------------------------------------------*/
	/*	
	/*	We've successfully queued the receive of the return
	/*	code. That's about all we have to do if things go
	/*	well, but if the operation fails later, we have to be
	/*	there to clean up.  To get control back, we queue
	/*	ourselves as a second operation so we can see how the
	/*	first did, and so we can free up arg.
	/*	
	/*----------------------------------------------------------*/

	initialize_operation(op, g_idbq, (char *)arg, (int (*)())NULL);
	(void) queue_operation(db->connection, CON_INPUT, op);

	return OP_RUNNING;
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_idbq
	/*	
	/*	Init routine for getting return code on performing a
	/*	bd query.  If there was an error, then we are done except for
	/*	cleaning up all the dynamic memory we allocated.
	/*	If the return code was 0,then we must asynchronously
	/*	do the following iteratively until a no is received:
	/*	
	/*	  while (async_receive(yes/no) == yes) {
	/*	       async receive new tuple
	/*	       add it to the relation 
	/*	  } 
	/*	
	/*----------------------------------------------------------*/

int
g_idbq(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct dbq_data *arg;
{
	int rc1, rc2, rc3;

	/*----------------------------------------------------------*/
	/*	
	/*	See how the three asynchronous operations went,and
	/*	clean up after them.
	/*	
	/*----------------------------------------------------------*/

       /*
        * Get return code from each of 3 ops
        */
	rc1 = OP_STATUS(arg->send_query);
	rc2 = OP_STATUS(arg->send_descriptor);
	rc3 = OP_STATUS(arg->get_retcode);

       /*
        * Release all transient data structures which were used in the
        * preliminary operations.
        */
	delete_operation(arg->send_query);
	delete_operation(arg->get_retcode);
	string_free(&(arg->s));

	/*----------------------------------------------------------*/
	/*	
	/*	If we've failed for any reason, then mark ourselves
	/*	complete and return.
	/*	
	/*----------------------------------------------------------*/

	if (rc1 != OP_COMPLETE || rc2 != OP_COMPLETE|| rc3 != OP_COMPLETE
	    || OP_RESULT(op) != OP_SUCCESS) {
		    OP_STATUS(op) = rc3;	/* we must have done */
						/* about as well as */
						/* the last one */
		
		    db_free((char *)arg, sizeof(struct dbq_data));
		    return rc3;			/* tell the dispatcher */
						/* that we're either */
						/* cancelled or complete */
        }
	  
	/*----------------------------------------------------------*/
	/*	
	/*	We've successfully received a return code of 0 from
	/*	Ingres, which means we are now going to begin the
	/*	yes/no loop.
	/*	
	/*----------------------------------------------------------*/

	op->fcn.cont = g_cdbq;			/* after the preempting */
						/* receive completes, the */
						/* dispatcher will call */
						/* this routine. */
	arg->state = YESNO;			/* tell continuation routine */
						/* that we're receiving */
						/* a yes/no */
	arg->tup = NULL;			/* so we won't try to clean */
						/* it up */
	arg->receive_yesno_or_data = create_operation();
	
	preempt_and_start_receiving_object(arg->receive_yesno_or_data,
					   op,
					   (char *)&(arg->yesno),
					   INTEGER_T);
	return OP_PREEMPTED;	
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_cdbq
	/*	
	/*	Continuation routine for receiving results of a query.
	/*	This is called repeatedly each time either a yes/no or
	/*	a new tuple is received.  It repeatedly preempts itself
	/*	to receive the next yes/no or tuple until a 'no'
	/*	is finally received.
	/*	
	/*----------------------------------------------------------*/

int
g_cdbq(op, hcon, arg)
OPERATION op;
HALF_CONNECTION hcon;
struct dbq_data *arg;
{
	/*----------------------------------------------------------*/
	/*	
	/*	See whether the preempting operation completed
	/*	successfully.  If not, we just clean up and cancel.
	/*	
	/*----------------------------------------------------------*/

	if (OP_STATUS(arg->receive_yesno_or_data) != OP_COMPLETE) {
		delete_operation(arg->receive_yesno_or_data);
		if (arg->tup != NULL)
			delete_tuple(arg->tup);
		db_free((char *)arg, sizeof(struct dbq_data));
		OP_STATUS(op) = OP_CANCELLED;
		return OP_CANCELLED;
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Whatever it was, we received it cleanly.  If it was
	/*	tuple data, then add it to the relation and prepare to
	/*	receive a yesno.  If it was a YES, then prepare to
	/*	receive the tuple data.  If it was a NO, then we're
	/*	all done.
	/*	
	/*	Note that g_cdbg will be recalled by the dispatcher
	/*	after the preempting routines have completed.
	/*	
	/*----------------------------------------------------------*/

       /*
        * New TUPLE DATA
        */

	if (arg->state == TUPDATA) {
		ADD_TUPLE_TO_RELATION(arg->rel, arg->tup);
		arg->tup = NULL;		/* so we won't try to */
						/* delete it in case of error*/
		reset_operation(arg->receive_yesno_or_data);
		arg->state = YESNO;
		preempt_and_start_receiving_object(arg->receive_yesno_or_data,
						   op,
						   (char *)&(arg->yesno),
						   INTEGER_T);
		return OP_PREEMPTED;	
	}

       /*
        * We just received a yes or no. If it's a YES, prepare to
        * receive some more tuple data.
        */
	if (arg->yesno == YES)  {
		arg->tup = create_tuple(arg->tpd);
		reset_operation(arg->receive_yesno_or_data);
		arg->state = TUPDATA;
		preempt_and_start_receiving_object(arg->receive_yesno_or_data,
						   op,
						   (char *)arg->tup,
						   TUPLE_DATA_T);
		return OP_PREEMPTED;	
	}

       /*
        * We just received a NO.  Looks like we're all done cleanly.
        */
	delete_operation(arg->receive_yesno_or_data);
	if (arg->tup != NULL)
		delete_tuple(arg->tup);
	db_free((char *)arg, sizeof(struct dbq_data));
	OP_STATUS(op) = OP_COMPLETE;
	return OP_COMPLETE;
}

/************************************************************************/
/*	
/*				db_query 
/*	
/*	Perform a relational query on the specified database.
/*	
/*	This just calls the asynchronous form of doing a query and
/*	waits for it to complete.
/*	
/*	
/************************************************************************/

int
db_query(db_handle, rel, query)
DATABASE db_handle; 
RELATION rel;
char *query;
{
	register OPERATION op;
	register int status;
	register int result;

       /*
        * Create an operation and use it to asynchronously perform
        * the operation
        */
	op = create_operation();
	(void) start_db_query(op, db_handle, rel, query);

       /* 
        * Wait for it to complete, note whether the operation
        * completed at all, and if so, whether it returned a
        * successful result.  Then reclaim the space used for the
        * operation.
        */
	(void) complete_operation(op);
	status = OP_STATUS(op);
	result = OP_RESULT(op);

	delete_operation(op);

       /*
        * Tell the caller either that we were interrupted, or pass
        * on the actual result of accessing the database.  If it
        * failed, then tear everything down after all.
        */
	if (status==OP_COMPLETE)
		return result;
	else
		return status;
}
