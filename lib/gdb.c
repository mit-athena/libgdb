
/************************************************************************/
/*	
/*				gdb.c
/*	
/*	Global Database Library - main controlling functions and globally
/*				  shared routines.
/*	
/*	Author: Noah Mendelsohn
/*	IBM T.J. Watson Research and Mit Project Athena
/*
/*	Revised:	8/21/87
/*
/*	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb.c,v $
/*	$Author: ghudson $
/*	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb.c,v 1.3 1996-09-20 04:32:04 ghudson Exp $
/*
/*	Copyright 1987 by the Massachusetts Institute of Technology.
/*	For copying and distribution information, see the file mit-copyright.h
/*	
/*	In addition to defining some utility routines for gdb, this source
/*	file #includes gdb_lib.h, which does the external definitions
/*	for all global data used by the library.  Everyone else gets
/*	externs for this data defined by gdb.h.
/*	
/************************************************************************/

#ifndef lint
static char rcsid_gdb_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/gdb.c,v 1.3 1996-09-20 04:32:04 ghudson Exp $";
#endif

#include "mit-copyright.h"
#include <stdio.h>
#include <string.h>
#include <signal.h> 
#include <pwd.h>
#include "gdb.h"
#include "gdb_lib.h"
#include <errno.h>

extern int sys_nerr;
extern char *sys_errlist[];

extern int errno;

int g_inited = FALSE;				/* gdb_init has not been */
						/* called */

	/*----------------------------------------------------------*/
	/*	
	/*	               gdb_init
	/*	
	/*	Initialize the global database facility.  Must be
	/*	called before any of the other gdb functions
	/*	are used.  Among other things, this function 
	/*	sets to ignore signals for writing on broken pipes.
	/*	
	/*----------------------------------------------------------*/

int
gdb_init()
{
	register int i;
	char hostname[255];			/* name of local host */
	extern uid_t getuid();
	int uid;				/* Unix user-i.d. number */
	char *uname;				/* string form of i.d. */

	struct passwd *pw_struct;		/* passwd entry comes back */
#ifdef SOLARIS
      struct sigaction act;
      (void)sigemptyset(&act.sa_mask);
      act.sa_flags = 0;
#endif
						/* here */
       /*
        * So we know we've been initialized, and we do it only once
        */
	if (g_inited)
		return 0;
	g_inited = TRUE;

       /*
        * Initialize the system defined types table
        */
	gdb_i_stype();

       /*
        * Initialize the connection control data structures.
        */
	gdb_mcons = 0;				/* number of connections */
						/* in use so far */
	for (i=0; i<GDB_MAX_CONNECTIONS; i++) {
		gdb_cons[i].id = GDB_CON_ID;
		gdb_cons[i].in.stream_buffer = NULL;
		gdb_cons[i].in.stream_buffer_length = GDB_STREAM_BUFFER_SIZE;
	}
       /*
        * Initialize the fd maps
        */
	gdb_mfd = 0;

	for (i=0; i<NFDBITS/sizeof(int); i++) {
		gdb_crfds.fds_bits[i] = 0;
		gdb_cwfds.fds_bits[i] = 0;
		gdb_cefds.fds_bits[i] = 0;
	}

       /*
        * Initialize the server/client layer
        */
	gdb_i_srv();

       /*
        * Ignore the signal generated when writing to a pipe which has been
        * closed at the other end.  gdb_move_data handles this condition
        * synchronously.
        */
#ifdef SOLARIS
     act.sa_handler= (void (*)()) SIG_IGN;
     (void) sigaction(SIGPIPE, &act, NULL);
#else
	(void) signal(SIGPIPE, SIG_IGN);
#endif

       /*
        * Make a note of the local host and user name
        */
	if (gethostname(hostname, sizeof(hostname)-1)!=0)
		(void) strcpy(hostname, "????");
	gdb_host = db_alloc(strlen(hostname)+1);
	(void) strcpy(gdb_host, hostname);

	uid = getuid();

	pw_struct = getpwuid(uid);

	if (pw_struct != NULL && pw_struct ->pw_name != NULL &&
	    *pw_struct->pw_name !='\0') 
		uname = pw_struct->pw_name;
	else
		uname = "????";
	gdb_uname = db_alloc(strlen(uname)+1);
	(void) strcpy(gdb_uname, uname);	
	
	return 0;
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_chk_init
	/*	
	/*	Make sure gdb has been initialized, blow up if not.
	/*	
	/*----------------------------------------------------------*/

int
g_chk_init()
{
	if (!g_inited)
		GDB_GIVEUP("You must call gdb_init before using GDB services.")
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_bitcount
	/*	
	/*	Count the number of bits in a word.  Adapted from 
	/*	K&R.
	/*	
	/*----------------------------------------------------------*/

int
g_bitcount(inp)
unsigned int inp;
{
	register unsigned int x = inp;		/* the word to check */
	register int count;			/* the count to return */

	for (count=0; x !=0; x=x>>1)
		if (x & 01)
			count++;

	return count;
}

	/*----------------------------------------------------------*/
	/*	
	/*			gdb_fstring
	/*	
	/*	Utility routine to conditionally free a null
	/*	terminated string.
	/*	
	/*----------------------------------------------------------*/

int
gdb_fstring(str)
char *str;
{
	if (str != NULL)
		db_free(str, strlen(str)+1);
}

	/*----------------------------------------------------------*/
	/*	
	/*			g_givup
	/*	
	/*	Called when a fatal error occurs.
	/*	
	/*----------------------------------------------------------*/

int
g_givup(errormsg)
char *errormsg;
{
	fprintf(gdb_log,"\n\nFATAL GDB ERROR:\n\n\t");
	fprintf(gdb_log,errormsg);
	fprintf(gdb_log,"\n\n");
	exit(100);
}

	/*----------------------------------------------------------*/
	/*	
	/*			connection_perror
	/*	
	/*	Acts like gdb_perror, but takes its errno from 
	/*	the connection.  Resets errno as a side effect.
	/*	THIS ROUTINE IS A NO_OP IF connection_status(con)!=CON_STOPPING
	/*	
	/*----------------------------------------------------------*/

int
connection_perror(con, msg)
CONNECTION con;
char *msg;
{
	if (con == NULL || connection_status(con) != CON_STOPPING)
		return;
	errno = connection_errno(con);
	gdb_perror(msg);
}

	/*----------------------------------------------------------*/
	/*	
	/*			gdb_perror
	/*	
	/*	Performs same function as system perror, but does
	/*	it on gdb_log instead of stderr.
	/*	
	/*----------------------------------------------------------*/

int
gdb_perror(msg)
char *msg;
{
	if(msg != NULL)
		fprintf(gdb_log, "%s: ", msg);
	if(errno < sys_nerr)
		fprintf(gdb_log, "%s.\n", sys_errlist[errno]);
	else
	        fprintf(gdb_log, "errno %d is out of range of message table.\n"
			);
}

