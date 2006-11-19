CC = gcc
AR = ar
CFLAGS = -O2 -Wall
#CFLAGS = /O2 /GL /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /EHsc /MD /W3 /nologo /c /Zi /TP  
SRCS = bgi.c server.c client.c ipc.c graphics.c
OBJS = bgi.o server.o client.o ipc.o graphics.o

openbgi.a: $(OBJS)
	$(AR) rvu $@ $(OBJS)
	ranlib $@

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)
clean:
	del *.o



