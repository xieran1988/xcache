
all: 
	xcache-stop
	make install
	xcache-start

cap.o: CFLAGS += $(shell pkg-config python glib-2.0 --cflags)

xcache-cap: cap.o queue.o main.o raw.o
	gcc -o $@ $^ \
		-lpthread \
		$(shell pcap-config --libs) \
		$(shell pkg-config python glib-2.0 --libs)

all-start: install
	/etc/init.d/lighttpd start
	screen -dmS cap xcache-cap --in eth1 -s
	screen -dmS reap xcache-reap
	screen -dmS logeth xcache-logeth

all-stop:
	/etc/init.d/lighttpd stop
	-pkill xcache-cap
	-screen -S cap -X quit
	-screen -S reap -X quit
	-screen -S logeth -X quit

run-netsniff: install
	xcache-cap --in eth1 -s #-f /root/port80-2.bpf 

umount-ssd:
	umount /dev/sdd

ext4-sdd:
	mkfs.ext4 /dev/sdd
	tune2fs -O ^has_journal /dev/sdd
	mount -o data=writeback,noatime /dev/sdd /ssd

umount-hd:
	umount /dev/sdc

ext4-hd:
	mkfs.ext4 /dev/sdc
	tune2fs -O ^has_journal /dev/sdc
	mount -o data=writeback,noatime /dev/sdc /hd

mkdir-hd:
	mkdir /hd/l
	mkdir /hd/d
	ln -sf /hd/l /l
	ln -sf /hd/d /d

weblog:
	./xcache-taillog | node data.js

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
	@update-rc.d xcache defaults

build-netsniff:
	cd netsniff-ng/src/build && make

stop-all:
	xcache-stop
	make stop-lighttpd

restart-all:
	make stop-all
	make start-all

start-all:
	xcache-start
	make start-lighttpd

stop-lighttpd:
	/etc/init.d/lighttpd stop

start-lighttpd:
	/etc/init.d/lighttpd start

restart-lighttpd:
	/etc/init.d/lighttpd restart

cp: build-netsniff
	@rm -rf /var/www/xcache*
	@cp xcache-jmp.pl xcache-charts.html /var/www
	@ln -sf /c /var/www/xcache-c
	@ln -sf /d /var/www/xcache-d
	@ln -sf /l /var/www/xcache-l
	@ln -sf /root/xcache/ /var/www/xcache-dev
	@cp 10-xcache.conf /etc/lighttpd/conf-enabled
	@rm -rf /usr/lib/xcache/*
	@cp mod_h264_streaming.so /usr/lib/lighttpd
	@cp netsniff-ng/src/build/netsniff-ng/netsniff-ng /usr/bin/xcache-cap
	@cp -dp xcache-* /usr/bin/

clear:
	@rm -rf /c/* /d/* /l/*

install: init cp 
#	@/etc/init.d/lighttpd restart

dep:
	cd /tmp && \
	wget "http://pypi.python.org/packages/source/f/flvlib/flvlib-0.1.12.tar.bz2#md5=9792adde8179516c5bbd474cd89e52dd" && \
	tar -xvf flvlib-0.1.12.tar.bz2 && \
	cd flv* && \
	python setup.py install
	apt-get install python-flup libpcap-dev lighttpd python-dev

apt-get:
	apt-get install -y --force-yes ethtool lighttpd
	apt-get build-dep -y --force-yes netsniff-ng lighttpd 

download-netsniff:
	git clone git://github.com/gnumaniacs/netsniff-ng.git

patch-netsniff:
	cd netsniff-ng/src && \
		ln -sf ../../cap.c xcache_cap.c && \
		ln -sf ../../queue.c xcache_queue.c && \
		ln -sf ../../../netsniff-ng-patch/CMakeLists.txt \
			netsniff-ng/CMakeLists.txt && \
		ln -sf ../../netsniff-ng-patch/netsniff-ng.c netsniff-ng.c

cmake-netsniff:
	cd netsniff-ng/src && mkdir -p build && cd build && cmake ..

mkdir-cache:
	mkdir -p /c /d /l /tl
	chmod 777 /c /d /l

download-lighttpd:
	wget http://download.lighttpd.net/lighttpd/releases-1.4.x/lighttpd-1.4.31.tar.bz2
	tar -xf lighttpd-1.4.31.tar.bz2 
	rm lighttpd-1.4.31.tar.bz2

patch-lighttpd:
	cd lighttpd-1.4.31/src && \
		ln -sf ../../lighttpd-patch/Makefile.am Makefile.am && \
		ln -sf ../../lighttpd-patch/mod_range.c mod_range.c
	ln -svf /usr/local/sbin/lighttpd /usr/sbin/lighttpd

autogen-lighttpd:
	cd lighttpd-1.4.31/ && ./autogen.sh && ./configure && make install

wget-fcgi-perl:
	wget http://search.cpan.org/CPAN/authors/id/F/FL/FLORA/FCGI-0.74.tar.gz

build-fcgi-perl:
	./configure
	perl Makefile.PL
	make install

range-test:
	echo 12345678 > /var/www/xcache-a
	wget -O - "http://localhost/xcache-a?rs=2&rl=3"

jmp-test:
	@echo "12345678" > /d/CF.3da812f
	@echo "12345678" > /d/CH.3da812f
	@echo "0-4/5" > /d/R.3da812f
	wget -O - http://localhost/com/aaa.mp4


uri := http://localhost/com/aaa.mp4 
q := 2> /dev/null
inrange-test: install
	@echo "12345678" > /d/CF.3da812f
	@echo "12345678" > /d/CH.3da812f
	echo "100 2-9" > /d/R.3da812f
	@wget -O - --header="Range: bytes=4-8" ${uri} ${q}
	@echo "==34567"
	@wget -O - --header="Range: bytes=2-9" ${uri} ${q}
	@echo "==12345678"
	@wget -O - --header="Range: bytes=2-" ${uri} ${q} || echo "error"
	@wget -O - ${uri} ${q} || echo "error"
	echo "11 4-" > /d/R.3da812f
	@wget -O - --header="Range: bytes=4-8" ${uri} ${q}
	@echo "==12345"
	@wget -O - --header="Range: bytes=5-" ${uri} ${q}
	@echo "==234567"
	@wget -O - --header="Range: bytes=11-" ${uri} ${q} && echo 'len0'
	@wget -O - ${uri} ${q} || echo "error"
	echo "8" > /d/R.3da812f
	@wget -O - --header="Range: bytes=4-7" ${uri} ${q}
	@echo "==5678"
	@wget -O - --header="Range: bytes=4-" ${uri} ${q}
	@echo "==5678"
	@wget -O - ${uri} ${q}
	@echo "==12345678"

clean:
	rm -rf *.o xcache-cap

ctest:
	gcc /root/t.c -o /tmp/c
	/tmp/c

update-lighttpd:
	cp 10-xcache.conf /etc/lighttpd/conf-enabled
	cp -dp xcache-jmp.pl /var/www/
	cp -dp xcache-fastjmp.py /usr/bin
	ln -sf /root/xcache /var/www/xcache-dev

update-xcache:
	xcache-stop
	make install
	xcache-start

test-fastjmp:
	make install && wget --header="Range: bytes=1-100" -O /tmp/o http://localhost/xcache-fastjmp/
	cat /tmp/o

test-fastjmp-py:
	echo "0-8/9" > /d/R.3da812f
	./xcache-fastjmp.py "http://localhost/www.youku.com/aaa.mp4?dddd&y=yjwt08" $c
	./xcache-fastjmp.py "http://localhost/www.youku.com/bbb.mp4?dddd&y=yjwt08" 
test-fastjmp-py2: update-lighttpd
	wget -d -O - http://localhost/youku.com/bbb.mp4

