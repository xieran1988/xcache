
all: install

cap.o: CFLAGS += $(shell pkg-config python glib-2.0 --cflags)

xcache-cap: cap.o queue.o main.o raw.o
	gcc -o $@ $^ \
		-lpthread \
		$(shell pcap-config --libs) \
		$(shell pkg-config python glib-2.0 --libs)

seq: seq.c
	@gcc seq.c -o seq

sum-seq:
	./sumseq | grep tot

run-netsniff: install 
	@cd /root/netsniff-ng/src && make && \
		netsniff-ng/netsniff-ng --in eth1 -s #-f /root/port80-2.bpf 

queue-netsniff:
	mode=2 make run-netsniff

direct-netsniff:
	make run-netsniff

weblog:
	make direct-netsniff | ./parse.pl | node data.js

save-netsniff:
	mode=1 make run-netsniff

mem-netsniff: save-netsniff replay-scap

replay: install
	seq=all xcache-cap $f 

replay-pcap:
	make replay f=/root/out.pcap 

replay-scap: 
	make replay f=/root/out.scap 

save-tcpdump:
	tcpdump -i eth1 -w /root/out.pcap "port 80"

replay-tcpdump: install replay-pcap

save-raw: install
	mode=1 cap=1 xcache-cap

queue-raw: install
	mode=2 cap=1 xcache-cap

mem-raw: save-raw replay-scap

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

cp: 
	@ln -sf /tmp /var/www/xcache
	@ln -sf /usr/lib/xcache /var/www/xcache-lib
	@cp 10-xcache.conf /etc/lighttpd/conf-enabled
	@rm -rf /usr/lib/xcache/*
	@cp urlsha.py /usr/lib/xcache/
	-@cp xcache-* /usr/bin/
	@cp -R xcache-web xcache-jmp.pl /var/www
	@cp mod_h264_streaming.so /usr/lib/lighttpd

install: init cp
	@/etc/init.d/lighttpd restart

dep:
	cd /tmp && \
	wget "http://pypi.python.org/packages/source/f/flvlib/flvlib-0.1.12.tar.bz2#md5=9792adde8179516c5bbd474cd89e52dd" && \
	tar -xvf flvlib-0.1.12.tar.bz2 && \
	cd flv* && \
	python setup.py install
	apt-get install python-flup libpcap-dev lighttpd python-dev

clean:
	rm -rf *.o xcache-cap

ctest:
	gcc /root/t.c -o /tmp/c
	/tmp/c
