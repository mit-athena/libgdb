###########################################################################
#
#				Makefile
#
#
#	Makefile for the GDB Sample Test Programs
#
#	Copyright 1987 by the Massachusetts Institute of Technology.
#	For copying and distribution information, see the file
#	"mit-copyright.h".
#
#
#	$Source: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/Makefile,v $
#	$Author: epeisach $
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/Makefile,v 1.1 1989-09-18 11:00:55 epeisach Exp $
#
#	Note: these programs are provided for the convenience of those
#	maintaining or playing with GDB.  Though they normally accompany
#	the distribution, they are not an offical part of it.  They
#	are not built or installed automatically, and there is no
#	supplied documentation on them.
#
###########################################################################

STD=	  tcl tsr tfsr tst tst2 tst3 tst4 tst5 tst6 samp1a
MISC=	  .saberinit
INCLIB= ../include
CFLAGS=  ${OCFLAGS} -I${INCLIB}
LINTFLAGS= ${OLINTFLAGS} -I${INCLIB}
GDBLIB= ../lib/libgdb.a
HESLIB= -lhesiod

.c.o:
	${CC} ${CFLAGS} -c $*.c

all: ${STD}

${STD}: ${GDBLIB} 
	${CC} ${CFLAGS} -o $@ $@.o ${GDBLIB} ${HESLIB}

lint:   
	echo "The GDB sample programs do not lint at this time"

clean:
	rm -f *~ *.o ${STD} .*~ coded.dat

install: 
	echo "The GDB sample programs do not install in the system at this time"
depend: 
	rm -f makedep eddep 
	for i in ${STD}; do \
		(echo $$i: $$i.o >>makedep; \
		 ${CC} -I${INCLIB} -M $$i.c >> makedep); \
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
tcl: tcl.o
tcl.o: tcl.c
tcl.o: ../include/mit-copyright.h
tcl.o: /usr/include/stdio.h
tcl.o: /usr/include/sgtty.h
tcl.o: /usr/include/sys/ioctl.h
tcl.o: /usr/include/sys/ttychars.h
tcl.o: /usr/include/sys/ttydev.h
tcl.o: ../include/gdb.h
tcl.o: /usr/include/sys/types.h
tcl.o: /usr/include/sys/time.h
tcl.o: /usr/include/time.h
tsr: tsr.o
tsr.o: tsr.c
tsr.o: ../include/mit-copyright.h
tsr.o: /usr/include/stdio.h
tsr.o: /usr/include/signal.h
tsr.o: ../include/gdb.h
tsr.o: /usr/include/sys/types.h
tsr.o: /usr/include/sys/time.h
tsr.o: /usr/include/time.h
tfsr: tfsr.o
tfsr.o: tfsr.c
tfsr.o: ../include/mit-copyright.h
tfsr.o: /usr/include/stdio.h
tfsr.o: ../include/gdb.h
tfsr.o: /usr/include/sys/types.h
tfsr.o: /usr/include/sys/time.h
tfsr.o: /usr/include/time.h
tst: tst.o
tst.o: tst.c
tst.o: ../include/mit-copyright.h
tst.o: /usr/include/stdio.h
tst.o: ../include/gdb.h
tst.o: /usr/include/sys/types.h
tst.o: /usr/include/sys/time.h
tst.o: /usr/include/time.h
tst2: tst2.o
tst2.o: tst2.c
tst2.o: ../include/mit-copyright.h
tst2.o: /usr/include/stdio.h
tst2.o: ../include/gdb.h
tst2.o: /usr/include/sys/types.h
tst2.o: /usr/include/sys/time.h
tst2.o: /usr/include/time.h
tst2.o: /usr/include/sys/file.h
tst3: tst3.o
tst3.o: tst3.c
tst3.o: ../include/mit-copyright.h
tst3.o: /usr/include/stdio.h
tst3.o: ../include/gdb.h
tst3.o: /usr/include/sys/types.h
tst3.o: /usr/include/sys/time.h
tst3.o: /usr/include/time.h
tst3.o: /usr/include/sys/file.h
tst4: tst4.o
tst4.o: tst4.c
tst4.o: ../include/mit-copyright.h
tst4.o: /usr/include/stdio.h
tst4.o: ../include/gdb.h
tst4.o: /usr/include/sys/types.h
tst4.o: /usr/include/sys/time.h
tst4.o: /usr/include/time.h
tst4.o: /usr/include/sys/file.h
tst5: tst5.o
tst5.o: tst5.c
tst5.o: ../include/mit-copyright.h
tst5.o: /usr/include/stdio.h
tst5.o: ../include/gdb.h
tst5.o: /usr/include/sys/types.h
tst5.o: /usr/include/sys/time.h
tst5.o: /usr/include/time.h
tst5.o: /usr/include/sys/file.h
tst6: tst6.o
tst6.o: tst6.c
tst6.o: ../include/mit-copyright.h
tst6.o: /usr/include/stdio.h
tst6.o: ../include/gdb.h
tst6.o: /usr/include/sys/types.h
tst6.o: /usr/include/sys/time.h
tst6.o: /usr/include/time.h
tst6.o: /usr/include/sys/types.h
tst6.o: /usr/include/sys/file.h
tst6.o: /usr/include/sys/socket.h
tst6.o: /usr/include/netinet/in.h
samp1a: samp1a.o
samp1a.o: samp1a.c
samp1a.o: ../include/mit-copyright.h
samp1a.o: /usr/include/stdio.h
samp1a.o: ../include/gdb.h
samp1a.o: /usr/include/sys/types.h
samp1a.o: /usr/include/sys/time.h
samp1a.o: /usr/include/time.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
