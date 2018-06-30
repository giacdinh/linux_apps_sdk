
CFLAGS=-I../inc
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

OBJS=dbase.o

dbase.a: $(OBJS)
	ar rc dbase.a *.o

