/************************************************************************/
/*      
/*                      gdb_debug.c
/*      
/*
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_debug.c,v $
/*	$Author: probe $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_debug.c,v 1.1 1993-10-12 03:25:12 probe Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*      Debugging interfaces for gdb.  Most of these are
/*	designed to be called from dbx or Saber as a means of formatting
/*	GDB's data areas.
/*      
/*      routines included are:
/*	
/*		gdb_debug set the debugging status flags
/*      
/*              gd_types_print prints all the available types currently in
/*                      the type table
/*              
/*              gd_con_status prints all the status codes for connections
/*      
/*              gd_sum_cons summarizes all current connections detailing 
/*                      for each flags, half connections with their 
/*                      respective operation queues.
/*      
/*              gd_op_q (halfcon) prints the queue associated with a 
/*                      specified half connection (not directly callable) 
/*              
/*              gd_op_status a listing of the available states of an
/*                      operation
/*      
/*      
/************************************************************************/

#ifndef lint
static char rcsid_gdb_debug_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb_debug.c,v 1.1 1993-10-12 03:25:12 probe Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include "gdb.h"

/************************************************************************/
/*	
/*			gdb_debug
/*	
/*	Toggle a debugging flag.  Warning:  the interface to this routine
/*	may change over time.
/*	
/************************************************************************/

int
gdb_debug(flag)
unsigned int flag;
{
	gdb_Debug ^= flag;			/* toggle the flag */
}

/************************************************************************/
/*	
/*			print_relation
/*	
/*	Formats a relation to the gdb_log file (defaults to stderr)
/*	
/************************************************************************/

int
print_relation(name, relation)
char *name;
RELATION relation;
{
	FCN_PROPERTY(RELATION_T,FORMAT_PROPERTY)(name, (char *)&(relation));
}
/************************************************************************/
/*	
/*			print_tuple
/*	
/*	Formats a tuple to the gdb_log file (defaults to stderr)
/*	
/************************************************************************/

int
print_tuple(name, tuple)
char *name;
TUPLE tuple;
{
	FCN_PROPERTY(TUPLE_T,FORMAT_PROPERTY)(name, (char *)&(tuple));
}

/************************************************************************/
/*	
/*			print_tuple_descriptor
/*	
/*	Formats a tuple_descriptor to the gdb_log file (defaults to stderr)
/*	
/************************************************************************/

int
print_tuple_descriptor(name, tuple_descriptor)
char *name;
TUPLE_DESCRIPTOR tuple_descriptor;
{
	FCN_PROPERTY(TUPLE_DESCRIPTOR_T,FORMAT_PROPERTY)(name, 
					     (char *)&(tuple_descriptor));
}

/************************************************************************/
/*      
/*                      gd_types_print
/*
/*      This is a routine for printing all the available types and 
/*      their typecodes.
/*      
/************************************************************************/

int
gd_types_print ()
{
	register int i;

	fprintf (gdb_log, "\n\nTHE AVAILABLE TYPES WITH THEIR TYPE CODES ARE: \n\n");

	fprintf (gdb_log, "typecode     name\n");

	for (i = 0; i < gdb_n_types; i++) {
		fprintf (gdb_log, "%2d       %s \n", i, STR_PROPERTY(i,NAME_PROPERTY));
	}
}

/************************************************************************/
/*      
/*                      gd_con_status
/*      
/*      This routine will print all the status codes for operations 
/*      This is just a listing of the status numbers located in gdb.h
/*      
/************************************************************************/

int
gd_con_status () 
{
        /*----------------------------------------------------------*/
        /*      
        /*      REMEMBER... these need to be fixed when the connection
        /*      status coded in gdb.h are redefined.
        /*      
        /*----------------------------------------------------------*/

        fprintf (gdb_log, "THE STATUS CODES ARE: \n\n");
        fprintf (gdb_log, " CODE     STATUS\n");
        fprintf (gdb_log, "   1      CON_STOPPED\n");
        fprintf (gdb_log, "   2      CON_UP\n");
        fprintf (gdb_log, "   3      CON_STARTING\n");
        fprintf (gdb_log, "   4      CON_STOPPING\n"); 

}


/************************************************************************/
/*      
/*                      summarize connections (gd_sum_con)
/*      
/************************************************************************/

gd_sum_con (index)
int index;
{
        if ((index > gdb_mcons) || (gdb_cons[index].status<1)) {
                fprintf (gdb_log,"gdb_cons[%d] is not a valid connection \n",index);
                return;
        }

        if (gdb_cons[index].status == CON_STOPPED) {
                fprintf (gdb_log,"connection gdb_cons[%d] is stopped\n",index);
                return;
        }

        /*----------------------------------------------------------*/
        /*      
        /*      REMEMBER this also must be changed when the def'n
        /*      of status fields in gdb.h is changed 
        /*      
        /*----------------------------------------------------------*/

                
        fprintf (gdb_log,"status of connection number %d is %2d \n",index,gdb_cons[index].status);
        fprintf (gdb_log,"The information for each half-connexn: \n\n");

        fprintf (gdb_log,"    the inbound half-connection\n");
        fprintf (gdb_log,"              status: %2d \n",gdb_cons[index].in.status);
        fprintf (gdb_log,"              flags : %2d \n",gdb_cons[index].in.status);
        fprintf (gdb_log,"              file descr: %2d \n",gdb_cons[index].in.fd);
        fprintf (gdb_log,"              The operation queue is:\n");
        gd_op_q (&(gdb_cons[index].in));

        fprintf (gdb_log,"    the outbound half-connection\n");
        fprintf (gdb_log,"              status: %2d \n",gdb_cons[index].out.status);
        fprintf (gdb_log,"              flags : %2d \n",gdb_cons[index].out.status);
        fprintf (gdb_log,"              file descr: %2d \n",gdb_cons[index].out.fd);
        fprintf (gdb_log,"              The operation queue is:\n");
        gd_op_q (&(gdb_cons[index].out));
      }


/************************************************************************/
/*
/*	                      gd_op_q
/*	
/*	Given a half connection, print formatted copies of all its
/*	queued operations on gdb_log.
/*      
/************************************************************************/


int 
gd_op_q (half_con)
HALF_CONNECTION half_con;

{
        int i;                                  /*counter for the ith
                                                  queued op */
        OPERATION current;
        
        current = half_con->op_q_first;  

        i = 0;
        
        if (current == NULL) {
                fprintf (gdb_log,"no operations in queue yet\n");
                return ;
        }


        printf ("OPERATION       STATUS\n\n");

        while (current != (OPERATION)half_con)  {
                fprintf (gdb_log,"%2d              %2d \n", i++ , current->status);
                current = current ->next;
        }
}

/************************************************************************/
/*      
/*                      gd_op_status
/*	
/*      This is a routine in which all the status codes and their 
/*      translations are printed.
/*      
/************************************************************************/
       
int 
gd_op_status ()
{
        /*----------------------------------------------------------*/
        /*      
        /*      REMEMBER these also need to be changed when 
        /*      states of an operation in gdb.h is redefined
        /*      
        /*----------------------------------------------------------*/

          printf ("CODE    OPERATION STATE\n\n");
          printf (" 1      OP_NOT_STARTED\n");
          printf (" 2      OP_QUEUED\n");
          printf (" 3      OP_RUNNING\n");
          printf (" 4      OP_COMPLETE\n");
          printf (" 5      OP_CANCELLING\n");
          printf (" 6      OP_CANCELLED\n");
          printf (" 7      OP_MARKED\n");
}
