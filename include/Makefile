###########################################################################
#
#				Makefile
#
#
#	Makefile for the GDB Database Include files.
#
#	Copyright 1987 by the Massachusetts Institute of Technology.
#	For copying and distribution information, see the file
#	"mit-copyright.h".
#
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/include/Makefile,v $
#	$Author: epeisach $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/include/Makefile,v 1.1 1991-02-21 11:06:55 epeisach Exp $
#
###########################################################################

DESTDIR=
INSTDIR=/usr/include
INCLUDEFILES=gdb.h gdb_lib.h mit-copyright.h
INSTALLFILES=gdb.h 
CFLAGS=	 "-DVERIFY_PROTOCOL -DHESIOD"
LINTFLAGS= "-DVERIFY_PROTOCOL -DHESIOD"

SUBDIR= 

all:	

install:
	for i in ${INSTALLFILES}; do  \
		(install -c $$i ${DESTDIR}${INSTDIR}/$$i); \
	done

clean:
	rm -f a.out core *.s *.o *~
#	for i in ${SUBDIR} ; do (cd $$i; make ${MFLAGS} clean; cd ..); done

depend:
#	for i in ${SUBDIR}; do \
#	    (cd $$i; make ${MFLAGS} depend;cd ..); done

lint:
#	for i in ${SUBDIR}; do \
#	    (cd $$i; make ${MFLAGS} OLINTFLAGS=${LINTFLAGS} depend;cd ..); done


# DO NOT DELETE THIS LINE -- make depend may use it at some future time
# DEPENDENCIES MUST END AT END OF FILE


