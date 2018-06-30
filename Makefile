

all: subdirs rfrain
CFLAGS=-I./inc
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

SUBDIRS = database reader remotem system ctrl

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

rfrain: main.o
		gcc -o rfrain main.o reader/reader.a database/dbase.a system/system.a \
				remotem/remotem.a ctrl/ctrl.a -lpthread -lmysqlclient 


clean:
	rm *.o rfrain

depclean:
	rm rfrain main.o reader/reader.a database/dbase.a remotem/remotem.a system/system.a \
			ctrl/ctrl.a ctrl/*.o reader/*.o remotem/*.o system/*.o database/*.o
