
all: pcap

pcap.o: CFLAGS += $(shell pkg-config python --cflags)

pcap: pcap.o
	gcc -o $@ $< $(shell pcap-config --libs) $(shell pkg-config python --libs)

cp: all
	cp xcache /etc/init.d
	update-rc.d xcache defaults
	mkdir -p /usr/lib/xcache
	mkdir -p /var/lib/xcache
	ln -sf /var/lib/xcache /var/www/
	ln -sf /usr/lib/xcache /var/www/xcache-lib
	cp 10-xcache.conf /etc/lighttpd/conf-enabled
	cp cap.py cut.py dump.py web.sh /usr/lib/xcache
	cp pcap /usr/bin/xcache-pcap
	cp dump.py /usr/bin/xcache-proc
	cp xcache-list /usr/bin/
	cp xcache-clear /usr/bin/
	cp mod_h264_streaming.so /usr/lib/lighttpd

install: 
	[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache start
	/etc/init.d/lighttpd restart

test:
	[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache stop
	xcache-pcap
	
clean:
	rm -rf *.o pcap


