PROG=	propdump
SRCS=	propdump.c
LDADD+=	-lprop


MKMAN=	no

.include <bsd.prog.mk>
