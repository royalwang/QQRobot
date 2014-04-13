# Makefile for MyQQ

CC=		gcc
CFLAGS=		-c -Wall -I/usr/include/mysql  -DBIG_JOINS=1 -fPIC -fno-strict-aliasing -Wl,-Bsymbolic-functions -rdynamic -L/usr/lib/mysql -lmysqlclient
LDFLAGS=	-lpthread -s
LD=		gcc  -I/usr/include/mysql  -DBIG_JOINS=1 -fPIC -fno-strict-aliasing -Wl,-Bsymbolic-functions -rdynamic -L/usr/lib/mysql -lmysqlclient

OBJS=		qqsocket.o qqcrypt.o md5.o debug.o qqclient.o memory.o config.o packetmgr.o qqpacket.o \
		prot_login.o protocol.o prot_misc.o prot_im.o prot_user.o list.o buddy.o group.o qun.o \
		prot_group.o prot_qun.o prot_buddy.o loop.o utf8.o myqq.o util.o crc32.o qqconn.o qq_robot.o db.o

TARGET=	../myqq

all: $(TARGET)
	@echo done.

$(TARGET): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -g -o $@

.c.o:
	$(CC) $(CFLAGS) -g -o $@ $<

.PHONY: clean cleanobj
clean:
	rm -f *.o
	rm -f $(TARGET)

cleanobj:
	rm -f *.o
