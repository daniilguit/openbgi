CC = gcc
AR = ar
CFLAGS = -O 
LDFLAGS = -s
SRCS = bgi.c server.c client.c ipc.c graphics.c
OBJS = bgi.o server.o client.o ipc.o graphics.o

openbgi.a: $(OBJS)
	$(AR) rvu $@ $(OBJS)
	ranlib $@

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)
clean:
	del *.o



