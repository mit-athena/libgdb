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
#	$Author: shanzer $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/db/Makefile,v 1.1 1987-10-29 11:58:20 shanzer Exp $
#
###########################################################################

INSTDIR= /usr/unsupported

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
	rm -f makedep eddep 
	for i in dbserv.c; do \
		(${CC} -I${INCLIB} -M $$i  >> makedep); \
	 done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep 
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
dbserv.o: dbserv.c
dbserv.o: ../include/mit-copyright.h
dbserv.o: /usr/include/stdio.h
dbserv.o: /usr/include/strings.h
dbserv.o: ../include/gdb.h
dbserv.o: /usr/include/sys/types.h
dbserv.o: /usr/include/sys/time.h
dbserv.o: /usr/include/time.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
