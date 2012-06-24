
all: live

pcap.o: CFLAGS += $(shell pkg-config python glib-2.0 --cflags) -DXCACHE

pcap: pcap.o
	gcc -o $@ $< $(shell pcap-config --libs) $(shell pkg-config python glib-2.0 --libs)

test-raw: raw.o
	gcc -o raw $< 
	./raw

save-pcap:
	tcpdump -i eth1 -w /root/out.pcap "port 80"

load: init
	make test c=/root/out.pcap

cp: pcap seq
	@rm -rf /usr/lib/python2.7/xcache.py
	@cp util.py /usr/lib/python2.7/xcache.py
	@ln -sf /var/lib/xcache /var/www/
	@ln -sf /usr/lib/xcache /var/www/xcache-lib
	@cp 10-xcache.conf /etc/lighttpd/conf-enabled
	@rm -rf /usr/lib/xcache/*
	@cp jmp.py cap.py dump.py web.sh /usr/lib/xcache
	@cp pcap /usr/bin/xcache-pcap
	@cp dump.py /usr/bin/xcache-proc
	@cp cut.py /usr/bin/xcache-cutflv
	@cp xcache-list /usr/bin/
	@cp xcache-clear /usr/bin/
	@cp xcache-stat /usr/bin
	@cp mod_h264_streaming.so /usr/lib/lighttpd

netsniff: install 
	@cd /root/netsniff-ng/src && make && \
		netsniff-ng/netsniff-ng --in eth1 -s -f /root/port80.bpf 

save-scap: 
	make netsniff

seq: seq.c
	@gcc seq.c -o seq

live:
	seq=all mode=2 make netsniff

live-pcap: install
	seq=all xcache-pcap

test-scap:
	make save-scap
	make replay-scap

replay: install
	@/etc/init.d/xcache stop
	make cp
	@/etc/init.d/lighttpd restart
	seq=all xcache-pcap $f 

replay-pcap:
	make replay f=/root/out.pcap 

replay-scap: 
	make replay f=/root/out.scap 

test-mutex:
	gcc mutex.c -pthread -o mutex 
	./mutex

test-pipe:
	gcc pipe.c -o pipe
	./pipe

init:
	@rm -rf /var/lib/xcache*
	@mkdir -p /usr/lib/xcache
	@mkdir -p /var/lib/xcache
	@mkdir -p /var/lib/xcache-log
	@cp xcache /etc/init.d
	@update-rc.d xcache defaults

dep:
	cd /tmp && \
	wget "http://pypi.python.org/packages/source/f/flvlib/flvlib-0.1.12.tar.bz2#md5=9792adde8179516c5bbd474cd89e52dd" && \
	tar -xvf flvlib-0.1.12.tar.bz2 && \
	cd flv* && \
	python setup.py install
	apt-get install python-flup libpcap-dev lighttpd python-dev

stop:
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop

update:
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	@/etc/init.d/lighttpd restart

install: init update

test:
	-[ -e /etc/init.d/xcache ] && /etc/init.d/xcache stop
	make cp
	/etc/init.d/xcache stop
	/etc/init.d/lighttpd restart
	xcache-pcap $c
	
clean:
	rm -rf *.o pcap


