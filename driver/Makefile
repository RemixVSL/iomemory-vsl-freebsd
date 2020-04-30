KMOD=	 iomemory-vsl

CFLAGS+= -I${.CURDIR}/include
CFLAGS+= -D__KERNEL__

SRCS = cdev.c	\
	errno.c	\
	state.c \
	kblock.c	\
	kcache.c	\
	kcondvar.c	\
	kcsr.c	\
	kfio.c	\
	kinfo.c	\
	ktr-os.c \
	kmem.c	\
	kmisc.c	\
	kscatter.c	\
	ktime.c	\
	main.c	\
	pci.c	\
	sched.c	\
	module_param.c	\

SRCS += bus_if.h    \
	device_if.h \
	pci_if.h

FIOARCH != uname -m
CCMAJOR != echo __GNUC__ | cc -E -xc - | tail -n 1
CCMINOR != echo __GNUC_MINOR__ | cc -E -xc - | tail -n 1
CCVER = ${CCMAJOR}${CCMINOR}
TARGET = ${FIOARCH}_cc${CCMAJOR}${CCMINOR}

KFIO_LIB ?= kfio/${TARGET}_libkfio.o_shipped

libkfio.o: ${KFIO_LIB}
	ln -sf ${.CURDIR}/${KFIO_LIB} ${.TARGET}

OBJS += libkfio.o
KMODDIR ?= /boot/modules

.include <bsd.kmod.mk>

CFLAGS+= -Wno-redundant-decls
