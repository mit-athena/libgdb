###########################################################################
#
#				Makefile
#
#
#	Makefile for the GDB Libraries
#
#	Copyright 1987 by the Massachusetts Institute of Technology.
#	For copying and distribution information, see the file
#	"mit-copyright.h".
#
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/Makefile,v $
#	$Author: epeisach $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/Makefile,v 1.3 1989-10-18 13:13:06 epeisach Exp $
#
#	Note: the profiled versions of the .o files are made in the directory
#	named profiled.  For the benefit of make, a parallel set of 
#	.p files are made in this directory.  They are always newer than
#	the corresponding profiled .o files.  The profiled directory
#	is NEVER rmdir'd.
#
###########################################################################

INSTDIR= /usr/athena/lib

LIBCSRCS=	 gdb_struct.c gdb_stype.c gdb.c gdb_trans.c \
gdb_trans2.c gdb_conn.c gdb_serv.c gdb_fserv.c gdb_db.c gdb_debug.c gdb_ops.c

LIBOBJS = gdb.o gdb_struct.o gdb_stype.o  gdb_trans.o gdb_trans2.o gdb_conn.o gdb_ops.o gdb_serv.o gdb_fserv.o gdb_db.o  gdb_debug.o

LIBOBJS_P = gdb.p gdb_struct.p gdb_stype.p  gdb_trans.p gdb_trans2.p gdb_conn.p gdb_ops.p gdb_serv.p gdb_fserv.p gdb_db.p  gdb_debug.p

INCLIB= ../include
CFLAGS=  ${OCFLAGS} -I${INCLIB}
LINTFLAGS =${OLINTFLAGS} -I${INCLIB}

SUBDIR= 
SCRIPT= 
STD=	
NSTD=	

.c.o:
	${CC} -c ${CFLAGS} $*.c

# see comments at head of makefile for explanation of this
.c.p:	
	mv -f $*.o $*.o.hold
	${CC} -c -pg ${CFLAGS} $*.c
	mv $*.o profiled/$*.o
	mv -f $*.o.hold $*.o
	touch $*.p

OPTS=


all: libgdb.a libgdb_p.a

libgdb.a: ${LIBOBJS}
	ar uv libgdb.a ${LIBOBJS}; ranlib libgdb.a

libgdb_p.a: ${LIBOBJS_P}
	(cd profiled; ar uv ../libgdb_p.a ${LIBOBJS}; ranlib ../libgdb_p.a)

install: libgdb.a libgdb_p.a
	install -c -m 644 libgdb.a ${DESTDIR}${INSTDIR}/libgdb.a
	ranlib ${DESTDIR}${INSTDIR}/libgdb.a
	install -c -m 644  libgdb_p.a ${DESTDIR}${INSTDIR}/libgdb_p.a
	ranlib ${DESTDIR}${INSTDIR}/libgdb_p.a

clean:
	rm -f a.out core *.s *.o *~ *.p profiled/* makedep TAGS .*~ *.a
	rm -f eddep makedep.p sed.file
#	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean; cd ..); done

lint:
	lint -huv ${LINTFLAGS} ${LIBCSRCS} | egrep -v "warning: possible pointer alignment problem"


depend:
	makedepend -I${INCLIB} ${CFLAGS} ${LIBCSRCS}


