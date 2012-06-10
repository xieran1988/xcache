
all: pcap

pcap.o: CFLAGS += $(shell pkg-config python --cflags)

pcap: pcap.o
	gcc -o $@ $< $(shell pcap-config --libs) $(shell pkg-config python --libs)

cp: all
	cp xcache /etc/init.d
	cp util.py /usr/lib/python2.7/xcache.py
	ln -sf /var/lib/xcache /var/www/
	ln -sf /usr/lib/xcache /var/www/xcache-lib
	cp 10-xcache.conf /etc/lighttpd/conf-enabled
	cp jmp.py cap.py cut.py dump.py web.sh /usr/lib/xcache
	cp pcap /usr/bin/xcache-pcap
	cp dump.py /usr/bin/xcache-proc
	cp xcache-list /usr/bin/
	cp xcache-clear /usr/bin/
	cp xcache-stat /usr/bin
	cp mod_h264_streaming.so /usr/lib/lighttpd

mkdirs:
	rm -rf /var/lib/xcache*
	mkdir -p /usr/lib/xcache
	mkdir -p /var/lib/xcache
	mkdir -p /var/lib/xcache-log

install: mkdirs
	update-rc.d xcache defaults
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache start
	/etc/init.d/lighttpd restart

replay: mkdirs
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


