
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
	@cd netsniff-ng/src/build && make && \
		./netsniff-ng/netsniff-ng --in eth1 -s #-f /root/port80-2.bpf 

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
	@rm -rf /var/www/xcache*
	@cp xcache-jmp.pl xcache-charts.html /var/www
	@ln -sf /c /var/www/xcache
	@cp 10-xcache.conf /etc/lighttpd/conf-enabled
	@rm -rf /usr/lib/xcache/*
	@cp urlsha.py /usr/lib/xcache/
	@cp xcache-* /usr/bin/
	@cp mod_h264_streaming.so /usr/lib/lighttpd

clear:
	@rm -rf /c/* /d/* /l/*

install: init cp clear
	@/etc/init.d/lighttpd restart

dep:
	cd /tmp && \
	wget "http://pypi.python.org/packages/source/f/flvlib/flvlib-0.1.12.tar.bz2#md5=9792adde8179516c5bbd474cd89e52dd" && \
	tar -xvf flvlib-0.1.12.tar.bz2 && \
	cd flv* && \
	python setup.py install
	apt-get install python-flup libpcap-dev lighttpd python-dev

dep2:
	apt-get build-dep netsniff-ng
	apt-get install -y --force-yes cmake libnl-dev flex bison \
		libgeoip-dev ethtool
	git clone git://github.com/gnumaniacs/netsniff-ng.git
	cd netsniff-ng/src && \
		ln -sv ../../cap.c xcache_cap.c && \
		ln -sv ../../queue.c xcache_queue.c && \
		ln -svf ../../netsniff-ng-patch/CMakeLists.txt \
			netsniff-ng/CMakeLists.txt && \
		ln -svf ../../netsniff-ng-patch/netsniff-ng.c netsniff-ng.c && \
		mkdir build && cd build && \
		make
	mkdir /c /d /l

clean:
	rm -rf *.o xcache-cap

ctest:
	gcc /root/t.c -o /tmp/c
	/tmp/c
