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
#	$Author: shanzer $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/lib/Makefile,v 1.2 1987-10-29 12:01:38 shanzer Exp $
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
	rm -f makedep eddep makedep.p sed.file
	echo 's/\.o/.p/' > sed.file
	for i in ${LIBCSRCS} ; do \
		(${CC} -I${INCLIB} -M $$i | \
		 tee -a makedep | \
		 sed -f sed.file >> makedep.p); \
	 done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo '$$r makedep.p' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep makedep.p sed.file
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

TAGS: ${LIBCSRCS} 
	etags ${LIBCSRCS} ${INCLIB}/*.h


# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
gdb_struct.o: gdb_struct.c
gdb_struct.o: ../include/mit-copyright.h
gdb_struct.o: /usr/include/stdio.h
gdb_struct.o: /usr/include/strings.h
gdb_struct.o: ../include/gdb.h
gdb_struct.o: /usr/include/sys/types.h
gdb_struct.o: /usr/include/sys/time.h
gdb_struct.o: /usr/include/time.h
gdb_stype.o: gdb_stype.c
gdb_stype.o: ../include/mit-copyright.h
gdb_stype.o: /usr/include/stdio.h
gdb_stype.o: /usr/include/strings.h
gdb_stype.o: ../include/gdb.h
gdb_stype.o: /usr/include/sys/types.h
gdb_stype.o: /usr/include/sys/time.h
gdb_stype.o: /usr/include/time.h
gdb_stype.o: /usr/include/netinet/in.h
gdb.o: gdb.c
gdb.o: ../include/mit-copyright.h
gdb.o: /usr/include/stdio.h
gdb.o: /usr/include/strings.h
gdb.o: /usr/include/signal.h
gdb.o: /usr/include/pwd.h
gdb.o: ../include/gdb.h
gdb.o: /usr/include/sys/types.h
gdb.o: /usr/include/sys/time.h
gdb.o: /usr/include/time.h
gdb.o: ../include/gdb_lib.h
gdb.o: /usr/include/errno.h
gdb_trans.o: gdb_trans.c
gdb_trans.o: ../include/mit-copyright.h
gdb_trans.o: /usr/include/sys/types.h
gdb_trans.o: /usr/include/strings.h
gdb_trans.o: /usr/include/stdio.h
gdb_trans.o: /usr/include/varargs.h
gdb_trans.o: /usr/include/errno.h
gdb_trans.o: ../include/gdb.h
gdb_trans.o: /usr/include/sys/types.h
gdb_trans.o: /usr/include/sys/time.h
gdb_trans.o: /usr/include/time.h
gdb_trans2.o: gdb_trans2.c
gdb_trans2.o: ../include/mit-copyright.h
gdb_trans2.o: /usr/include/sys/types.h
gdb_trans2.o: /usr/include/errno.h
gdb_trans2.o: /usr/include/stdio.h
gdb_trans2.o: ../include/gdb.h
gdb_trans2.o: /usr/include/sys/types.h
gdb_trans2.o: /usr/include/sys/time.h
gdb_trans2.o: /usr/include/time.h
gdb_trans2.o: /usr/include/sys/uio.h
gdb_trans2.o: /usr/include/sys/socket.h
gdb_conn.o: gdb_conn.c
gdb_conn.o: ../include/mit-copyright.h
gdb_conn.o: /usr/include/stdio.h
gdb_conn.o: /usr/include/strings.h
gdb_conn.o: ../include/gdb.h
gdb_conn.o: /usr/include/sys/types.h
gdb_conn.o: /usr/include/sys/time.h
gdb_conn.o: /usr/include/time.h
gdb_conn.o: /usr/include/sys/types.h
gdb_conn.o: /usr/include/sys/uio.h
gdb_conn.o: /usr/include/sys/socket.h
gdb_conn.o: /usr/include/sys/ioctl.h
gdb_conn.o: /usr/include/sys/ttychars.h
gdb_conn.o: /usr/include/sys/ttydev.h
gdb_conn.o: /usr/include/netinet/in.h
gdb_conn.o: /usr/include/netdb.h
gdb_conn.o: /usr/include/errno.h
gdb_serv.o: gdb_serv.c
gdb_serv.o: /usr/include/stdio.h
gdb_serv.o: /usr/include/strings.h
gdb_serv.o: ../include/gdb.h
gdb_serv.o: /usr/include/sys/types.h
gdb_serv.o: /usr/include/sys/time.h
gdb_serv.o: /usr/include/time.h
gdb_serv.o: /usr/include/sys/ioctl.h
gdb_serv.o: /usr/include/sys/ttychars.h
gdb_serv.o: /usr/include/sys/ttydev.h
gdb_fserv.o: gdb_fserv.c
gdb_fserv.o: ../include/mit-copyright.h
gdb_fserv.o: /usr/include/stdio.h
gdb_fserv.o: /usr/include/sys/types.h
gdb_fserv.o: /usr/include/sys/uio.h
gdb_fserv.o: /usr/include/sys/socket.h
gdb_fserv.o: /usr/include/sys/wait.h
gdb_fserv.o: /usr/include/sys/signal.h
gdb_fserv.o: ../include/gdb.h
gdb_fserv.o: /usr/include/sys/types.h
gdb_fserv.o: /usr/include/sys/time.h
gdb_fserv.o: /usr/include/time.h
gdb_fserv.o: /usr/include/sys/resource.h
gdb_db.o: gdb_db.c
gdb_db.o: ../include/mit-copyright.h
gdb_db.o: /usr/include/stdio.h
gdb_db.o: /usr/include/strings.h
gdb_db.o: ../include/gdb.h
gdb_db.o: /usr/include/sys/types.h
gdb_db.o: /usr/include/sys/time.h
gdb_db.o: /usr/include/time.h
gdb_debug.o: gdb_debug.c
gdb_debug.o: ../include/mit-copyright.h
gdb_debug.o: /usr/include/stdio.h
gdb_debug.o: ../include/gdb.h
gdb_debug.o: /usr/include/sys/types.h
gdb_debug.o: /usr/include/sys/time.h
gdb_debug.o: /usr/include/time.h
gdb_ops.o: gdb_ops.c
gdb_ops.o: ../include/mit-copyright.h
gdb_ops.o: /usr/include/stdio.h
gdb_ops.o: ../include/gdb.h
gdb_ops.o: /usr/include/sys/types.h
gdb_ops.o: /usr/include/sys/time.h
gdb_ops.o: /usr/include/time.h
gdb_ops.o: /usr/include/netinet/in.h
gdb_ops.o: /usr/include/sys/ioctl.h
gdb_ops.o: /usr/include/sys/ttychars.h
gdb_ops.o: /usr/include/sys/ttydev.h
gdb_struct.p: gdb_struct.c
gdb_struct.p: ../include/mit-copyright.h
gdb_struct.p: /usr/include/stdio.h
gdb_struct.p: /usr/include/strings.h
gdb_struct.p: ../include/gdb.h
gdb_struct.p: /usr/include/sys/types.h
gdb_struct.p: /usr/include/sys/time.h
gdb_struct.p: /usr/include/time.h
gdb_stype.p: gdb_stype.c
gdb_stype.p: ../include/mit-copyright.h
gdb_stype.p: /usr/include/stdio.h
gdb_stype.p: /usr/include/strings.h
gdb_stype.p: ../include/gdb.h
gdb_stype.p: /usr/include/sys/types.h
gdb_stype.p: /usr/include/sys/time.h
gdb_stype.p: /usr/include/time.h
gdb_stype.p: /usr/include/netinet/in.h
gdb.p: gdb.c
gdb.p: ../include/mit-copyright.h
gdb.p: /usr/include/stdio.h
gdb.p: /usr/include/strings.h
gdb.p: /usr/include/signal.h
gdb.p: /usr/include/pwd.h
gdb.p: ../include/gdb.h
gdb.p: /usr/include/sys/types.h
gdb.p: /usr/include/sys/time.h
gdb.p: /usr/include/time.h
gdb.p: ../include/gdb_lib.h
gdb.p: /usr/include/errno.h
gdb_trans.p: gdb_trans.c
gdb_trans.p: ../include/mit-copyright.h
gdb_trans.p: /usr/include/sys/types.h
gdb_trans.p: /usr/include/strings.h
gdb_trans.p: /usr/include/stdio.h
gdb_trans.p: /usr/include/varargs.h
gdb_trans.p: /usr/include/errno.h
gdb_trans.p: ../include/gdb.h
gdb_trans.p: /usr/include/sys/types.h
gdb_trans.p: /usr/include/sys/time.h
gdb_trans.p: /usr/include/time.h
gdb_trans2.p: gdb_trans2.c
gdb_trans2.p: ../include/mit-copyright.h
gdb_trans2.p: /usr/include/sys/types.h
gdb_trans2.p: /usr/include/errno.h
gdb_trans2.p: /usr/include/stdio.h
gdb_trans2.p: ../include/gdb.h
gdb_trans2.p: /usr/include/sys/types.h
gdb_trans2.p: /usr/include/sys/time.h
gdb_trans2.p: /usr/include/time.h
gdb_trans2.p: /usr/include/sys/uio.h
gdb_trans2.p: /usr/include/sys/socket.h
gdb_conn.p: gdb_conn.c
gdb_conn.p: ../include/mit-copyright.h
gdb_conn.p: /usr/include/stdio.h
gdb_conn.p: /usr/include/strings.h
gdb_conn.p: ../include/gdb.h
gdb_conn.p: /usr/include/sys/types.h
gdb_conn.p: /usr/include/sys/time.h
gdb_conn.p: /usr/include/time.h
gdb_conn.p: /usr/include/sys/types.h
gdb_conn.p: /usr/include/sys/uio.h
gdb_conn.p: /usr/include/sys/socket.h
gdb_conn.p: /usr/include/sys/ioctl.h
gdb_conn.p: /usr/include/sys/ttychars.h
gdb_conn.p: /usr/include/sys/ttydev.h
gdb_conn.p: /usr/include/netinet/in.h
gdb_conn.p: /usr/include/netdb.h
gdb_conn.p: /usr/include/errno.h
gdb_serv.p: gdb_serv.c
gdb_serv.p: /usr/include/stdio.h
gdb_serv.p: /usr/include/strings.h
gdb_serv.p: ../include/gdb.h
gdb_serv.p: /usr/include/sys/types.h
gdb_serv.p: /usr/include/sys/time.h
gdb_serv.p: /usr/include/time.h
gdb_serv.p: /usr/include/sys/ioctl.h
gdb_serv.p: /usr/include/sys/ttychars.h
gdb_serv.p: /usr/include/sys/ttydev.h
gdb_fserv.p: gdb_fserv.c
gdb_fserv.p: ../include/mit-copyright.h
gdb_fserv.p: /usr/include/stdio.h
gdb_fserv.p: /usr/include/sys/types.h
gdb_fserv.p: /usr/include/sys/uio.h
gdb_fserv.p: /usr/include/sys/socket.h
gdb_fserv.p: /usr/include/sys/wait.h
gdb_fserv.p: /usr/include/sys/signal.h
gdb_fserv.p: ../include/gdb.h
gdb_fserv.p: /usr/include/sys/types.h
gdb_fserv.p: /usr/include/sys/time.h
gdb_fserv.p: /usr/include/time.h
gdb_fserv.p: /usr/include/sys/resource.h
gdb_db.p: gdb_db.c
gdb_db.p: ../include/mit-copyright.h
gdb_db.p: /usr/include/stdio.h
gdb_db.p: /usr/include/strings.h
gdb_db.p: ../include/gdb.h
gdb_db.p: /usr/include/sys/types.h
gdb_db.p: /usr/include/sys/time.h
gdb_db.p: /usr/include/time.h
gdb_debug.p: gdb_debug.c
gdb_debug.p: ../include/mit-copyright.h
gdb_debug.p: /usr/include/stdio.h
gdb_debug.p: ../include/gdb.h
gdb_debug.p: /usr/include/sys/types.h
gdb_debug.p: /usr/include/sys/time.h
gdb_debug.p: /usr/include/time.h
gdb_ops.p: gdb_ops.c
gdb_ops.p: ../include/mit-copyright.h
gdb_ops.p: /usr/include/stdio.h
gdb_ops.p: ../include/gdb.h
gdb_ops.p: /usr/include/sys/types.h
gdb_ops.p: /usr/include/sys/time.h
gdb_ops.p: /usr/include/time.h
gdb_ops.p: /usr/include/netinet/in.h
gdb_ops.p: /usr/include/sys/ioctl.h
gdb_ops.p: /usr/include/sys/ttychars.h
gdb_ops.p: /usr/include/sys/ttydev.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
