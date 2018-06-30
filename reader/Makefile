
CFLAGS=-I../inc
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

OBJS=reader.o readerAPIs.o hex_bytes.o osdep_posix.o serial_reader.o serial_reader_l3.o \
		serial_transport_posix.o tm_reader_async.o tm_reader.o tmr_param.o \
		tmr_loadsave_configuration.o tmr_strerror.o tmr_utils.o

reader.a: $(OBJS)
	ar rc reader.a *.o

