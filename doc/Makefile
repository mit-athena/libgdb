###########################################################################
#
#				Makefile
#
#
#	Makefile for the GDB Database documentation files.
#
#	Copyright 1987 by the Massachusetts Institute of Technology.
#	For copying and distribution information, see the file
#	"mit-copyright.h".
#
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/doc/Makefile,v $
#	$Author: don $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/doc/Makefile,v 1.1 1988-06-09 16:32:36 don Exp $
#
###########################################################################

.SUFFIXES: .1 .2 .3 .4 .5 .6 .7 .8 .man .mss .PS 
DESTDIR=
INSTDIR=/usr/doc/gdb
MANINSTDIR=/usr/man
MAN1FILES=dbserv.1
MAN3FILES=gdb.3
SCRIBEFILES=gdb_ugd.mss gdb_library.mss

.mss.PS:
	scribe $*.mss
	scribe $*.mss

.1.man:
	nroff -man $*.1 > $*.man
.3.man:
	nroff -man $*.3 > $*.man


SUBDIR= 

all:	

install:
#	for i in ${SUBDIR}; do \
#	    (cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in ${SCRIBEFILES}; do \
		(install -c $$i ${DESTDIR}${INSTDIR}/$$i); \
	done
	for i in ${MAN1FILES}; do \
		(install -c $$i ${DESTDIR}${MANINSTDIR}/man1/$$i); \
	done
	for i in ${MAN3FILES}; do \
		(install -c $$i ${DESTDIR}${MANINSTDIR}/man3/$$i); \
	done

clean:
	rm -f a.out core *.s *.o *~ *.PS *.err *.man *.otl *.aux
#	for i in ${SUBDIR} ; do (cd $$i; make ${MFLAGS} clean; cd ..); done

depend:
#	for i in ${SUBDIR}; do \
#	    (cd $$i; make ${MFLAGS} depend;cd ..); done

lint:
#	for i in ${SUBDIR}; do \
#	    (cd $$i; make ${MFLAGS} OLINTFLAGS=${LINTFLAGS} depend;cd ..); done


# DO NOT DELETE THIS LINE -- make depend may use it at some future time
# DEPENDENCIES MUST END AT END OF FILE


