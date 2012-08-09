
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

init:
	@mkdir -p /usr/lib/xcache
	#@update-rc.d xcache defaults

build-netsniff:
	cd netsniff-ng/src/build && make

stop-all:
	xcache-stop
	make stop-lighttpd

clear-and-restart-all:
	make clear
	make restart-all

restart-all:
	make stop-all
	rm -rf /c/*
	make install
	make build-lighttpd
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
	@rm -rf /usr/lib/xcache/*
	@cp netsniff-ng/src/build/netsniff-ng/netsniff-ng /usr/bin/xcache-cap
	@cp -dp xcache-* /usr/bin/
	@cp -dp data.js /usr/lib/xcache
	make update-lighttpd

clear:
	@rm -rf /c/* /d/* /tl/*
	> /l/L
	> /l/cap
	> /l/E
	> /l/cgi2

touch-log:
	umask 000 && touch /l/cgi2

install: init cp turn-off-gso
#	@/etc/init.d/lighttpd restart
#

turn-off-gso:
	. /root/xcache/rc.local

apt-get:
	apt-get install -y --force-yes ethtool lighttpd screen curl
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

build-lighttpd:
	cd lighttpd-1.4.31 && make install

autogen-lighttpd:
	cd lighttpd-1.4.31/ && ./autogen.sh && ./configure && make install

clean:
	rm -rf *.o xcache-cap

test-reap:
	make install
	cp sample-cap /tmp/kk
	do=1 xcache-reap-one /tmp/kk

test-mod-range:
	make update-restart-lighttpd
	make test-pyjmp
	curl -v -r 1-9 "http://localhost/www.youku.com/aaa.mp4?y=yjwt08"
	wget -O - "http://localhost/www.youku.com/aaa.mp4?y=yjwt08"
#	wget -O - "http://localhost/dddd?302"

test-cap:
	./test-cap.pl

test-lighttpd:
	make update-restart-lighttpd
	echo 12346678 > /var/www/xcache-testfile
	echo 123 > /tmp/ha
	echo 789 > /tmp/hb
	#curl "http://localhost/xcache-testfile?fufkcyousadf&ahha"
	#curl "http://localhost/dddd?302"
	#wget -O - "http://localhost/dddd?302"
	#curl "http://localhost/dddd?chunk"
	#curl -r 0-333 "http://localhost/dddd?py"
	make tail-lighttpd-log

tail-lighttpd-log:
	tail /var/log/lighttpd/error.log

update-lighttpd:
	mkdir -p /var/www/xcache
	cp 10-xcache.conf /etc/lighttpd/conf-enabled
	cp lighttpd.conf /etc/lighttpd/
	cp -dp xcache-cgi.pl /var/www/xcache/cgi.pl
	cp -dp util.py /usr/lib/xcache
	ln -f /root/xcache/index.html /var/www/xcache/index.html
	cp *.js /var/www/xcache/
	ln -sf /var/log/lighttpd/error.log /le

update-restart-lighttpd:
	make stop-lighttpd
	make update-lighttpd
	make build-lighttpd
	make start-lighttpd

update-xcache:
	xcache-stop
	make install
	xcache-start

wget-js:
	wget -O /var/www/xcache/jquery.js http://code.jquery.com/jquery-1.7.2.min.js
	wget -O /var/www/xcache/highcharts.js http://code.highcharts.com/highcharts.js
	wget -O /var/www/xcache/socket.js https://raw.github.com/LearnBoost/socket.io-client/master/dist/socket.io.min.js

test-wget-listc:
	echo ddd > /c/A.sss
	echo ccc > /c/A.ss
	wget -O - "http://localhost/xcache-cgi.pl?listc"

test-wget-mgr:
	wget -O /tmp/wget "http://localhost/QQPCMgr_Setup_Basic_68_2393.exe?yjwt"

test-wget-minggame:
	wget -O /tmp/wget "http://localhost/MingGame.swf?yjwt"

test-wget-qq:
	wget -O /tmp/wget "http://localhost/QQBrowserSetup.exe?yjwt"

test-pyjmp:
	make test-combine
	./util.py

test-combine: clear
	./gen-combine-test 3da812f /d


