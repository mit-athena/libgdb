/************************************************************************/
/*	
/*		          dbserv
/*			  ------
/*	
/*	The GDB Ingres Database Server Program
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/dbserv.qc,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/dbserv.qc,v 1.1 1993-10-12 03:24:42 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*	
/************************************************************************/
/*	
/*	PURPOSE
/*	-------
/*	
/*	This is a GDB forking server which uses Ingres to implement
/*	the services of the GDB database access layer.  At present,
/*	this is the simplest, most straightforward (and quite synchronous)
/*	implementation.  Optimized versions may be implemented in the
/*	future.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_dbserv_qc[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/dbserv.qc,v 1.1 1993-10-12 03:24:42 probe Exp $";
#endif


#include "mit-copyright.h"
#include <stdio.h>
#include <strings.h>
#include "gdb.h"

#define OVERLAP 10				/* number of tuples */
						/* which may be */
						/* simultaneously queue */
						/* for transmission */
#define SKIP_BLANKS(cp) while (*(cp)==' ') cp++

/************************************************************************/
/*	
/*			   GLOBAL DECLARATIONS
/*	
/************************************************************************/

int last_ingres_error;
int error_handler();
int ret_limit = 0;				/* don't retrieve more */
						/* than this many tuples */
						/* 0 means no limit */

int log = FALSE;				/* don't log anything */
char *host, *user;				/* hostname and username */
						/* of our client */

FILE *logfile;

/************************************************************************/
/*	
/*				MAIN
/*	
/************************************************************************/

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

	STRING request;				/* this string contains the */
						/* ingres command sent */
						/* from the client.  Some */
						/* of the commands are */
						/* specially coded, some */
						/* not */
	TUPLE_DESCRIPTOR tpd;			/* descriptor of the tuple */
						/* which comes from the */
						/* client at startup */

	int index;				/* field index in a tuple */
	char *cp;				/* scratch pointer */
	char *service_name = GDB_DB_SERVICE;	/* the GDB service name */
						/* from /etc/services */

	/*----------------------------------------------------------*/
	/*	
	/*	Check parameters
	/*	
	/*----------------------------------------------------------*/

         /*
          * Check for switches
          */
	while (--argc > 0) {
	       /*
		* -l  ==> set log file name
		*/
		if(strncmp(argv[argc], "-l",2) == 0)  {
			log = TRUE;
			logfile = stderr;
			cp = argv[argc]+2;
			if (*cp != '\0') {
				if (logfile == NULL) {
					fprintf(stderr, "Could not open log file %s\n", cp);
					exit(64);
				}
				setbuf(logfile, (char *)NULL); /* do not buffer */
						       /* writes to the log */
			}
	       /*
		* -s  ==> set service name from /etc/services
		*/
		} else if(strncmp(argv[argc], "-s",2) == 0)  {
			cp = argv[argc]+2;
			if (*cp != '\0') 
				service_name = cp;			
		} else {
			fprintf(stderr,"Correct form is %s <-llogfilename> <-sservicename>\n\n\tdefault logfile is stderr if -l specified\nService name should be in /etc/services or #portnumber\n",
				argv[0]);
			exit(4);
		}
	}

	/*----------------------------------------------------------*/
	/*	
	/*			Initialize
	/*	
	/*----------------------------------------------------------*/

	gdb_init();				/* set up gdb */
  /*      gdb_debug(GDB_NOFORK);  */

	/*----------------------------------------------------------*/
	/*	
	/*	Now, turn ourselves into a forking server.
	/*	
	/*----------------------------------------------------------*/

	client = create_forking_server(service_name, NULL);

	/*----------------------------------------------------------*/
	/*	
	/*	Make a note of the client user id and host 
	/*	
	/*----------------------------------------------------------*/

	tpd = DESCRIPTOR_FROM_TUPLE(gdb_client_tuple);

	index = field_index(tpd, "host");
	host = STRING_DATA(*(STRING *)(FIELD_FROM_TUPLE(gdb_client_tuple,
							index)));
	index = field_index(tpd, "user");
	user = STRING_DATA(*(STRING *)(FIELD_FROM_TUPLE(gdb_client_tuple,
							index)));

	if (log) {
		prefix();
		fprintf(logfile, "New connection\n");
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Tell Ingress that we will handle our own errors
	/*	
	/*----------------------------------------------------------*/

	IIseterr(error_handler);

	/*----------------------------------------------------------*/
	/*	
	/*	Here we are in the child process for each client.
	/*	Check the client request and make sure that it
	/*	is correct.  Opent the database.
	/*	
	/*----------------------------------------------------------*/

	if (!get_db(client)) {
		sever_connection(client);
		exit(8);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	The database requested by the client has been accessed
	/*	and we have told 'him' so.  Repeatedly read in requests
	/*	and process them.
	/*	
	/*----------------------------------------------------------*/

	while (TRUE) {
		if(receive_object(client, (char *)&request, STRING_T) ==
		   OP_CANCELLED) 
			db_giveup(client);
		if (!strncmp(STRING_DATA(request), "term", 4)) {
			if (log) {
				prefix();
				fprintf(logfile,"Client has terminated\n");
			}
			sever_connection(client);
			exit(0);
		}
		do_db_op(client, &request);
		string_free(&request);
	}
}


/************************************************************************/
/*	
/*			     get_db
/*	
/*	Try to access the database requested by the client.  If we can't
/*	then call db_giveup.
/*	
/************************************************************************/

int
get_db(client)
CONNECTION client;
##{
##	char *dbname;
	int reply_code = DB_OPEN;
	TUPLE_DESCRIPTOR tpd;
	int parms_field_index;


       /*
        * Get the name of the database from the client's parms field
        */
	tpd = DESCRIPTOR_FROM_TUPLE(gdb_client_tuple);
	parms_field_index = field_index(tpd, "parms");
	dbname = STRING_DATA(*(STRING *)FIELD_FROM_TUPLE(gdb_client_tuple,
							 parms_field_index));

	last_ingres_error = 0;
##	ingres dbname

	if (last_ingres_error) {
		if (log) {
			prefix();
			fprintf(logfile, "Bad Database: %s\n", dbname);
		}
		reply_code = DB_CLOSED;
	} 
	if(send_object(client, (char *)&reply_code, INTEGER_T) ==
	   OP_CANCELLED)
			db_giveup(client);
	if (log) {
		prefix();
		fprintf(logfile, "Database `%s' opened successfully\n", dbname);
	}
	return (reply_code == DB_OPEN);		/* true only if open */
##}


/************************************************************************/
/*	
/*			error_handler
/*	
/*	Called by ingres in case of error.
/*	
/************************************************************************/

int
error_handler(errnum)
int	*errnum;
{
	last_ingres_error = *errnum;
	return *errnum;
}


/************************************************************************/
/*	
/*			  do_db_op
/*	
/*	Called each time a new request for a database operation is 
/*	received to do the requested operation.
/*	
/************************************************************************/

int
do_db_op(client, requestp)
CONNECTION client;
STRING *requestp;
{
	int reply_code;

	/*----------------------------------------------------------*/
	/*	
	/*	Log the request
	/*	
	/*----------------------------------------------------------*/

	if (log) {
		prefix();
		fprintf(logfile, "Db-op: %s\n", STRING_DATA(*requestp));
	}
	
	/*----------------------------------------------------------*/
	/*	
	/*	If this is a retreive request, then call a special
	/*	routine to handle it.
	/*	
	/*----------------------------------------------------------*/

	if (strncmp(STRING_DATA(*requestp), "retrieve", 8) == 0) {
		do_query(client, requestp);
		return;
	}

	/*----------------------------------------------------------*/
	/*	
	/*	See if this is a GDB only meta-operation
	/*	
	/*----------------------------------------------------------*/

	if (strncmp(STRING_DATA(*requestp), "gdb ", 4) == 0) {
		do_gdb_op(client, STRING_DATA(*requestp)+4);
		return;
	}




	/*----------------------------------------------------------*/
	/*	
	/*	The following mess is what's needed (though undocumented)
	/*	to send an arbitrary QUEL command to ingres.  In the
	/*	future, we will have to validate these for safety.
	/*	
	/*----------------------------------------------------------*/

	last_ingres_error = 0;
	IIwrite(STRING_DATA(*requestp));
	IIsync("dbserv:perform operation for remote client",1);

	/*----------------------------------------------------------*/
	/*	
	/*	Send the return code to our caller
	/*	
	/*----------------------------------------------------------*/

	reply_code = last_ingres_error;
	if(send_object(client, &reply_code, INTEGER_T) ==
	   OP_CANCELLED)
		db_giveup(client);
}


/************************************************************************/
/*	
/*			do_gdb_op
/*	
/*	Implement meta-operations which are not supported by
/*	Ingres but which appear to be supported when using GDB.
/*	
/************************************************************************/

int
do_gdb_op(client, requestp)
CONNECTION client;
char *requestp;
{
	int new_limit;
	int ok=0;
	int bad=4;
	int *rc;

       /*
        * The only gdb operation we support for now is retrieve_limit
        */
	if (sscanf(requestp, "retrieve_limit %d", &new_limit) == 1) {
/* 		ret_limit = new_limit;  */
		rc = &ok;
	} else {
		rc = &bad;
	}
	if(send_object(client, rc, INTEGER_T) ==
	   OP_CANCELLED)
		db_giveup(client);
}


/************************************************************************/
/*	
/*			  do_query
/*	
/*	Called each time a new query request is received.  The retrieve
/*	request is received in the same form as a quel query, except that
/*	the construction >*fldname*< is used as the target of an assignment
/*	to stand for the corresponding field in a GDB tuple to be returned.
/*	
/*	This routine substitutes the appropriate %  construction, based on
/*	the type of the field into the retrieve, and builds the corresponding
/*	variable list array for Ingres.  Because the documented Ingres
/*	interfaces do not support this level of generality, we call the
/*	appropriate II routines directly to actually do the query.
/*	
/*	In this implementation, the entire relation is retrieved before
/*	any results are transmitted, and the entire result relation is
/*	sent back synchronously.
/*	
/************************************************************************/

struct pending {
	OPERATION yes_no;			/* used to indicate */
						/* that data is to follow*/
	OPERATION data_trans;			/* the tuple data is */
						/* queued using this op */
	TUPLE tup;				/* the actual data to */
						/* be sent */
	int busy;				/* TRUE if there is */
						/* pending transmission */
						/* on this one */
};						/* contains the state of */
						/* each pending */
						/* transmission */


int
do_query(client, requestp)
CONNECTION client;
STRING *requestp;
{
	TUPLE_DESCRIPTOR tpd=NULL;		/* describes the fields */
						/* in the relation*/
						/* NULL required because */
						/* receive object checks*/
	char buffer[GDB_MAX_QUERY_SIZE+10];	/* the ingres query is */
						/* actually built here */
						/* the +10 saves us from */
						/* having to check on */
						/* every short append */
	char *nextp = buffer;			/* next unused addres */
						/* in buffer */
	char *endp = buffer + GDB_MAX_QUERY_SIZE; /* char past end */
						  /* of buffer */

	char *nextr = STRING_DATA(*requestp);	/* address of next byte */
						/* of request */

	char *vars[GDB_MAX_QUERY_FIELDS];	/* addresses of vars */
	int  offs[GDB_MAX_QUERY_FIELDS];	/* offsets to the fields */
						/* negative values are */
						/* indicies in the text */
						/* structure below.  (-1) */
						/* corresponds to text[0] */
						/* (-2) is text[-1] and */
						/* so on.  */
	char text[GDB_MAX_RETRIEVED_TEXT_FIELDS][GDB_SIZE_OF_INGRES_TEXT];
						/* all text or character */
						/* type fields are */
						/* retrieved */
						/* here*/
	int text_indx[GDB_MAX_RETRIEVED_TEXT_FIELDS]; /* for each text field */
						/* the index  in the tuple */
						/* of the corresponding */
						/* STRING_T field */
	int next_text = 0;			/* number of text fields */
						/* assigned so far */

	TUPLE tup;				/* each result tuple */
						/* is retrieved here first*/

	register char c;			/* next character to */
						/* parse */

	int nfields = 0;			/* number of fields for */
						/* which we have done */
						/* substitutions*/
	int reply_code;				/* return code sent back */
						/* to client */
	int yes=1, no=0;			/* these codes are sent */
						/* to the client immediately */
						/* prior to each tuple */
						/* The no code indicates */
						/* that the last tuple */
						/* has been sent */

	struct pending  pending[OVERLAP];	/* contains the state of */
						/* each pending */
						/* transmission */
	int next_up;				/* index of next pending */
						/* state entry to use*/
	int first_time = TRUE;			/* true before we send */
						/* the return code to client */
	int tuple_count = 0;			/* total number of tuples */
						/* we've retrieved so far */

	/*----------------------------------------------------------*/
	/*	
	/*	Get the tuple descriptor for the relation to be 
	/*	retrieved.
	/*	
	/*----------------------------------------------------------*/

	if(receive_object(client, &tpd, TUPLE_DESCRIPTOR_T) ==
	   OP_CANCELLED)
		db_giveup(client);


	/*----------------------------------------------------------*/
	/*	
	/*	Parse the request and build it in the form that 
	/*	ingres wants it
	/*	
	/*----------------------------------------------------------*/

	c = *nextr++;
	while (c!='\0') {
		if (c == '>' && *nextr == '*') {
                       /*
                        * Start of an escaped sequence
                        */
			if(!do_field(tpd, &nextp, &nextr, 
				     offs, text_indx, &nfields, &next_text))
				query_parse_fail(client,requestp);
		} else {
                       /*
                        * Just copy this character
                        */
			*nextp++ = c;
		}
		if (nextp>endp) {
			if (log) {
				prefix();
				fprintf(logfile, "Query too long\n");
			}
			query_parse_fail(client, requestp);
			return;
		}
		c = *nextr++;		  
	}

	*nextp = '\0';


	
	/*----------------------------------------------------------*/
	/*	
	/*	Now, do the retrieve into the tuple
	/*	
	/*----------------------------------------------------------*/

	make_tuple(tpd, &tup, nfields, offs,  vars, text);
						/* make a new tuple */
						/* and set up */
						/* vars to point at */
						/* the right fields*/
	last_ingres_error = 0;

       /*
        * The following mess is an incantation which causes ingres to
        * issue the retrieve in the variable buffer, but with left handed
        * variable parameter substitution.  This is just what we want, but
        * unfortunately, none of the documented Ingres interfaces do it.  
        * I figured this out by looking at the output of the eqc preprocessor
        * for a parameterized retrieve.
        */
	setup_pending(pending, tpd);
	next_up = 0;				/* index of next pending */
						/* tuple */
	IIw_left(buffer, vars);
	IIsetup();
	while(IIgettup("dbserv.qc: retrieve for foreign client",1)){
               /*
                * See if we've retrieved too many.  If so, give up
                */
		tuple_count++;
		if (ret_limit>0 && (tuple_count)> ret_limit) {
			IIbreak(0);		/* tell ingres we're */
						/* quitting early*/
			goto bugout;
		}


		/*----------------------------------------------------------*/
		/*	
		/*	Send the return code to our caller
		/*	
		/*----------------------------------------------------------*/
	
		if (first_time) {
			reply_code = last_ingres_error;
			if(send_object(client, &reply_code, INTEGER_T) ==
			   OP_CANCELLED)
				db_giveup(client);
			first_time = FALSE;
		}
		if (pending[next_up].busy) {
			complete_operation(pending[next_up].data_trans);
			if (OP_STATUS(pending[next_up].data_trans) ==
			    OP_CANCELLED)
				db_giveup(client);
			null_tuple_strings(pending[next_up].tup);
		}
		bcopy((char *)tup, (char *)pending[next_up].tup, tpd->data_len);
	        fix_strings(pending[next_up].tup,  text, next_text, text_indx);
		reset_operation(pending[next_up].yes_no);
		start_sending_object(pending[next_up].yes_no, client,
				     (char *)&yes,INTEGER_T);
		reset_operation(pending[next_up].data_trans);
		start_sending_object(pending[next_up].data_trans, client,
				     (char *)pending[next_up].tup,
				     TUPLE_DATA_T);
		pending[next_up].busy = TRUE;
		next_up = (next_up+1)%OVERLAP;
	}

	IIflushtup("dbserv.qc: retrieve for foreign client",2);
       /*
        * Come here when we are quitting prematurely
        */
      bugout:
	delete_tuple(tup);

	if (log) {
		prefix();
		fprintf(logfile, "Ingerr=%d Count=%d\n", last_ingres_error,
			tuple_count);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Send the return code to our caller if we failed,
	/*	or else send the 'no' code to indicate that no more
	/*	tuples follow.  Note also that the synchronous
	/*	send of the no code flushes all pending operations,
	/*	so we don't have to wait explicitly for completion.
	/*	
	/*----------------------------------------------------------*/

	if (first_time) {
		reply_code = last_ingres_error;
		if(send_object(client, &reply_code, INTEGER_T) ==
		   OP_CANCELLED)
			db_giveup(client);
	} 

       /*
        * If we told client that we were successful, he's going to expect
        * a NO at the end, no matter what.
        */
	if (reply_code  == 0) {
		if(send_object(client, &no, INTEGER_T) ==
		   OP_CANCELLED)
			db_giveup(client);
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Delete state structures and tuple descriptor
	/*	
	/*----------------------------------------------------------*/

	teardown_pending(pending);
	delete_tuple_descriptor(tpd);


}


/************************************************************************/
/*	
/*			   db_giveup
/*	
/*	Connection to client has failed.
/*	
/************************************************************************/

int
db_giveup(client)
CONNECTION client;
{
	if (log) {
		prefix();
		fprintf(logfile,"Connection to client has failed\n");
	}
	sever_connection(client);
	exit(16);
}

/************************************************************************/
/*	
/*			setup_pending
/*	
/*	Set up the state of the pending array to its initial null
/*	values.
/*	
/************************************************************************/

int
setup_pending(pending, tpd)
struct pending pending[OVERLAP];
TUPLE_DESCRIPTOR tpd;
{
	register struct pending *pp;

	for (pp=pending; pp<pending+OVERLAP; pp++) {
		pp->busy = FALSE;
		pp->yes_no = create_operation();
		pp->data_trans = create_operation();
		pp->tup = create_tuple(tpd);
		initialize_tuple(pp->tup);
	}
}

/************************************************************************/
/*	
/*			teardown_pending
/*	
/*	Set up the state of the pending array to its initial null
/*	values.
/*	
/************************************************************************/

int
teardown_pending(pending)
struct pending pending[OVERLAP];
{
	register struct pending *pp;

	for (pp=pending; pp<pending+OVERLAP; pp++) {
		delete_operation(pp->yes_no);
		delete_operation(pp->data_trans);
		null_tuple_strings(pp->tup);
		delete_tuple(pp->tup);
	}
}



/************************************************************************/
/*	
/*			do_field
/*	
/*	In the course of parsing a query request we have encountered
/*	the start of the escape sequence which starts a field.  Do the
/*	necessary processing and preparation of fields.
/*	
/************************************************************************/

int
do_field(tpd, nextpp, nextrp, offs, text_indx, nfieldsp, next_textp)
TUPLE_DESCRIPTOR tpd;				/* describes each tuple */
						/* in the relation to be */
						/* retrieved*/
char **nextpp;					/* next output char*/
char **nextrp;					/* next char in the request */
int  offs[GDB_MAX_QUERY_FIELDS];		/* offsets to the fields */
						/* negative values are */
						/* indicies in the text */
						/* structure below */
int text_indx[GDB_MAX_RETRIEVED_TEXT_FIELDS];	/* for each text field, */
						/* index  in tuple */
int *nfieldsp;					/* number of fields done */
						/* so far */
int *next_textp;				/* number of text fields */
						/* assigned so far */
#define FIND_CHAR(cp, char) while (*(cp)!=(char)) cp++
{
	char *fld_end;				/* points past end of name */
						/* in source */
	register char *fldname;			/* name of the field */

	register int fieldnum;			/* index of field we're */
						/* working on */

	int field_in_tuple;			/* index of field in tuple */
	int field_type;				/* GDB type of this field */

	/*----------------------------------------------------------*/
	/*	
	/*	Turn the field name into a null terminated string, in place
	/*	
	/*----------------------------------------------------------*/

	fldname = *nextrp+1;			/* char past the '*' */
	SKIP_BLANKS(fldname);
	fld_end = fldname;
	FIND_CHAR(fld_end, '*');		/* find the terminating '*' */
	*fld_end = '\0';			/* make it a null */
						/* remember that fld_end */
						/* still points where * used */
						/* to be. */

	/*----------------------------------------------------------*/
	/*	
	/*	Allocate a new field, and find it in the tuple
	/*	
	/*----------------------------------------------------------*/

	fieldnum = *nfieldsp;			/* index of field we */
						/* fill in */
	(*nfieldsp)++;				/* increment number of */
						/* fields */

	if (*nfieldsp > GDB_MAX_QUERY_FIELDS) {
		if (log) {
			prefix();
			fprintf(logfile,"do_field: attempted to retrieve too many fields\n current limit is %d\n",
			GDB_MAX_QUERY_FIELDS);
		}
		return FALSE;
	}

	field_in_tuple = field_index(tpd, fldname);

	if(field_in_tuple<0) {
		return FALSE;
	}

	field_type =FIELD_TYPE_IN_TUPLE(tpd, field_in_tuple);

	/*----------------------------------------------------------*/
	/*	
	/*	Set up the query parameters, depending on the type
	/*	of the field.  Most data is retrieved right into the
	/*	tuple, but string fields are handled specially.
	/*	
	/*----------------------------------------------------------*/

	switch (field_type) {
	      case INTEGER_T:
	           add_to_query(nextpp, "%i4");
		   offs[fieldnum] = FIELD_OFFSET_IN_TUPLE(tpd,field_in_tuple);
		   break;
	      case REAL_T:
	           add_to_query(nextpp, "%f8");
		   offs[fieldnum] = FIELD_OFFSET_IN_TUPLE(tpd,field_in_tuple);
		   break;
	      case STRING_T:
	           add_to_query(nextpp, "%c");
		   offs[fieldnum] = - ((*next_textp)+1);
		   text_indx[*next_textp] = field_in_tuple;
		   (*next_textp)++;
		   if (*next_textp >= GDB_MAX_RETRIEVED_TEXT_FIELDS-1) {
			   if (log) {
				   prefix();
				   fprintf(logfile,"do_field: attempted to retrieve too many fields of type text\n current limit is %d\n",
					   GDB_MAX_RETRIEVED_TEXT_FIELDS);
			}
		   }
		   break;
	      default:
		   if (log) {
			   prefix();
			   fprintf(logfile,"do_field: field type defaults for type %d\n",
				   field_type);
		}
		   return FALSE;
	}

	/*----------------------------------------------------------*/
	/*	
	/*	Clean up and leave
	/*	
	/*----------------------------------------------------------*/

	*nextrp = fld_end + 2;			/* parse resumes after the */
						/* <* */
	return TRUE;


}


/************************************************************************/
/*	
/*				add_to_query
/*	
/*	tack a string onto the end of the current partially formed
/*	query.
/*	
/************************************************************************/

int
add_to_query(nextpp, str)
char **nextpp;
char *str;
{
	register char *s = str;
	while(*s != '\0') {
		**nextpp = *s++;
		(*nextpp)++;
	}
}

/************************************************************************/
/*	
/*			   fix_strings
/*	
/*	Because we don't know how long text data is until we 
/*	retrieve it, text data is always retrieved first into the
/*	text buffer.  Here we allocate a real STRING and copy the data.
/*	
/************************************************************************/

int
fix_strings(tup, text, text_fields, text_indx)
TUPLE tup;
char text[GDB_MAX_RETRIEVED_TEXT_FIELDS][GDB_SIZE_OF_INGRES_TEXT];
int text_fields;
int text_indx[GDB_MAX_RETRIEVED_TEXT_FIELDS];	/* for each text field, */
						/* index in tuple */

{
	register TUPLE t = tup;
	register int i;
	STRING *stp;


	/*----------------------------------------------------------*/
	/*	
	/*	Find string fields and take care of them
	/*	
	/*----------------------------------------------------------*/

	for (i=0; i<text_fields; i++) {
		stp = (STRING *)FIELD_FROM_TUPLE(t, text_indx[i]);
		string_alloc(stp, strlen(text[i])+1);
		(void) strcpy(STRING_DATA(*stp), text[i]);		
	}


}

/************************************************************************/
/*	
/*			   make_tuple
/*	
/*	Allocate a new tuple and set up the entries in the vars array
/*	to point at the correct fields.  For most field types, the 
/*	vars array will point directly to the correct field in the tuple.
/*	For certain character string types, the actual pointer is to
/*	an entry in the text array.  Later, after the query is done, the 
/*	tuple will be reconstructed with strings of the correct length.
/*	
/************************************************************************/

int
make_tuple(tpd, tupp, nfields, offs, vars, text)
TUPLE_DESCRIPTOR tpd;				/* describes the structure */
						/* of each tuple */
TUPLE *tupp;					/* the new tuple gets */
						/* created here */
int nfields;					/* the number of fields */
						/* we have to set up*/

int  offs[GDB_MAX_QUERY_FIELDS];		/* offsets to the fields */
						/* negative values are */
						/* indicies in the text */
						/* structure below */
char *vars[GDB_MAX_QUERY_FIELDS];		/* addresses of vars */
char text[GDB_MAX_RETRIEVED_TEXT_FIELDS][GDB_SIZE_OF_INGRES_TEXT]; /*  */
						/* all text or character */
						/* type fields are retrieved */
						/* here*/
{
	register TUPLE t;			/* working copy */
	register int i;				/* index to fields */

	/*----------------------------------------------------------*/
	/*	
	/*	Create a new tuple
	/*	
	/*----------------------------------------------------------*/

	t = create_tuple(tpd);
	initialize_tuple(t);
	*tupp = t;				/* give a copy back to */
						/* our caller */

	/*----------------------------------------------------------*/
	/*	
	/*	Fill in the vars array for each field
	/*	
	/*----------------------------------------------------------*/

	for (i=0; i<nfields; i++) {
		if (offs[i]>=0) {

                       /*
                        * This is an ordinary field stored right in the
                        * tuple
                        */
			vars[i] = (t->data) + offs[i]; /* address of the */
						       /* data */
		} else {
                       /*
                        * This is a text field which goes in the
                        * text array
                        */
			vars[i] = text[-(offs[i]+1)];
		}
	}


}

/************************************************************************/
/*	
/*			query_parse_fail
/*	
/*	Couldn't parse the query.  We'll give up on this client
/*	entirely.
/*	
/************************************************************************/

int query_parse_fail(client, requestp)
CONNECTION client;
STRING *requestp;
{
	int return_code = DB_PARSE_FAIL;
	if (log) {
		prefix();
		fprintf(logfile, "Error parsing query, closing connection\n");
	}
	send_object(client, (char *)&return_code, INTEGER_T);
	sever_connection(client);
	exit(24);
}

/************************************************************************/
/*	
/*			prefix
/*	
/*	Write the user-i.d. prefix into the log
/*	
/************************************************************************/

char *ctime();

int
prefix()
{
	long timeval=(long)time((long *)0);

/*	char *conv = ctime(&timeval); */


	fprintf(logfile, "%ld %s@%s\t",  timeval, user, host);
}
