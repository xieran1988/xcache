
all: pcap

pcap.o: CFLAGS += $(shell pkg-config python glib-2.0 --cflags)

pcap: pcap.o
	gcc -o $@ $< $(shell pcap-config --libs) $(shell pkg-config python glib-2.0 --libs)

raw: raw.o
	gcc -o $@ $< 

cp: all
	rm -rf /usr/lib/python2.7/xcache.py
	cp util.py /usr/lib/python2.7/xcache.py
	ln -sf /var/lib/xcache /var/www/
	ln -sf /usr/lib/xcache /var/www/xcache-lib
	cp 10-xcache.conf /etc/lighttpd/conf-enabled
	rm -rf /usr/lib/xcache/*
	cp jmp.py cap.py dump.py web.sh /usr/lib/xcache
	cp pcap /usr/bin/xcache-pcap
	cp dump.py /usr/bin/xcache-proc
	cp cut.py /usr/bin/xcache-cutflv
	cp xcache-list /usr/bin/
	cp xcache-clear /usr/bin/
	cp xcache-stat /usr/bin
	cp mod_h264_streaming.so /usr/lib/lighttpd

init:
	rm -rf /var/lib/xcache*
	mkdir -p /usr/lib/xcache
	mkdir -p /var/lib/xcache
	mkdir -p /var/lib/xcache-log
	cp xcache /etc/init.d
	update-rc.d xcache defaults

dep:
	cd /tmp && \
	wget "http://pypi.python.org/packages/source/f/flvlib/flvlib-0.1.12.tar.bz2#md5=9792adde8179516c5bbd474cd89e52dd" && \
	tar -xvf flvlib-0.1.12.tar.bz2 && \
	cd flv* && \
	python setup.py install
	apt-get install python-flup libpcap-dev lighttpd python-dev

update:
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache start
	/etc/init.d/lighttpd restart

install: init update

replay: init
	/etc/init.d/xcache stop
	make cp
	/etc/init.d/lighttpd restart
	sudo xcache-pcap tests/2.pcap
	-wget -O /dev/null http://127.0.0.1/shot/dfdsf.cf
	sudo pkill xcache-proc

test:
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache stop
	/etc/init.d/lighttpd restart
	xcache-pcap
	
clean:
	rm -rf *.o pcap


