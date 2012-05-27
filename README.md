XCache
===========================

XCache is an advanced flv/mp4 caching/streaming system. 
Filter GET /*.(flv|mp4) using libpcap+python, 
using mod_h264_streaming for mp4 streaming, and flvlib+fcgi for flv streaming.

Managment
----------
Web Interface: http://127.0.0.1/cache
`/etc/init.d/xcache start|stop|restart` controls the daemon

BUILD: 

`sudo apt-get install python-flup libpcap-dev lighttpd && make`