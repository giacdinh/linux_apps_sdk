
CFLAGS=-I../inc
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

OBJS=controller.o

controller.a: $(OBJS)
	ar rc ctrl.a *.o

