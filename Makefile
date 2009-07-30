.PHONY : clean install

prefix = /usr/local

PREFIX = $(prefix)

all : gptgen

clean :
	-rm -f *.o gptgen

install : all
	install -d $(PREFIX)/sbin
	install -m 755 gptgen $(PREFIX)/sbin
