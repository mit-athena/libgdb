###########################################################################
#
#				Makefile
#
#
#	Makefile for the GDB Database Server
#
#	Copyright 1987 by the Massachusetts Institute of Technology.
#	For copying and distribution information, see the file
#	"mit-copyright.h".
#
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/Makefile,v $
#	$Author: epeisach $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/Makefile,v 1.5 1989-10-18 13:12:56 epeisach Exp $
#
###########################################################################

INSTDIR= /usr/athena

SRCS=	  dbserv.qc
INCLIB= ../include
CFLAGS=  ${OCFLAGS} -I${INCLIB}
LINTFLAGS= ${OLINTFLAGS} -I${INCLIB}
INGRESBIN= /usr/rtingres/bin
INGRESLIB= /usr/rtingres/lib
GDBLIB= ../lib/libgdb.a

.c.o:
	-${CC} -c ${CFLAGS} $*.c


all: dbserv

dbserv.c: dbserv.qc
	-${INGRESBIN}/eqc -d -fdbserv.c dbserv.qc

dbserv: dbserv.o ${GDBLIB} ${INGRESLIB}/libqlib ${INGRESLIB}/compatlib 
	-${CC} ${CFLAGS} -o dbserv dbserv.o ${GDBLIB} ${INGRESLIB}/libqlib ${INGRESLIB}/compatlib -lm

lint:   dbserv.c
	echo "The following should produce lint errors for"
	echo " send_object, receive_object, and the value of the"
	echo " time() library function"
	lint -huv ${LINTFLAGS} dbserv.c | egrep -v "warning: possible pointer alignment problem" | egrep -v "index redefinition hides earlier one"

clean:
	rm -f *~ *.o dbserv.c dbserv

install: dbserv
	install -c -s -m 755 dbserv ${DESTDIR}${INSTDIR}/dbserv

depend: dbserv.c
	makedepend -I${INCLIB} ${CFLAGS} dbserv.c

