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
#	$Header: /afs/dev.mit.edu/source/repository/athena/lib/gdb/samps/Makefile,v 1.2 1989-10-18 13:13:16 epeisach Exp $
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
	mkdep ${CFLAGS} -I${INCLIB} `for i in ${STD}; do \
		(echo $$i: $$i.o >>makedep; done`

