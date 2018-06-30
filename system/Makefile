
CFLAGS=-I../inc
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

OBJS=sys_msg.o watchdog.o logging.o

system.a: $(OBJS)
	ar rc system.a *.o

