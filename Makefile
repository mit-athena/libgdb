#	Makefile for the entire GDB Package
#
#	Noah Mendelsohn
#	Created: 8/24/87
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/Makefile,v $
#	$Author: shanzer $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/Makefile,v 1.1 1987-10-28 18:38:10 shanzer Exp $

DESTDIR=
INSTDIR=/usr/unsupported
CFLAGS=	 "-O -DHESIOD"
LINTFLAGS= "-DHESIOD"

SUBDIR= lib   include doc



all:	${SUBDIR} db 

${SUBDIR} samps: FRC
	(cd $@; make ${MFLAGS} OCFLAGS=${CFLAGS} all)

db: FRC
	(cd $@; make -k ${MFLAGS} OCFLAGS=${CFLAGS} all;echo "The make (above) for dbserv may blow up if Ingres is not installed.";echo "The rest of GDB should still build properly.")

FRC:

# ${STD}:
# 	${CC} ${CFLAGS} -o $@ $@.c

install: ${SUBDIR} db
	for i in ${SUBDIR} ; do\
	    (cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	for i in db; do  \
	    (cd $$i; make -k ${MFLAGS} DESTDIR=${DESTDIR} install); done
	echo "GDB Installation Attempt Complete"

clean:
	rm -f a.out core *.s *.o *~ .*~
	for i in ${SUBDIR} samps db; do (cd $$i; make ${MFLAGS} clean; cd ..); done

depend:
	for i in ${SUBDIR} samps; do \
	    (cd $$i; make ${MFLAGS} depend;cd ..); done
	for i in db; do \
	    (cd $$i; make -k ${MFLAGS} depend;cd ..); done
	echo "GDB Makefile dependencies have been rebuilt"


lint:
	for i in ${SUBDIR}  db; do \
	    (cd $$i; make ${MFLAGS} OLINTFLAGS=${LINTFLAGS} lint;cd ..); done


# DO NOT DELETE THIS LINE -- make depend may use it at some future time
# DEPENDENCIES MUST END AT END OF FILE
