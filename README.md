XCache
===========================

XCache is an advanced flv/mp4 caching/streaming system. 
Filter GET /*.(flv|mp4) using libpcap+python, 
using mod_h264_streaming for mp4 streaming, and flvlib+fcgi for flv streaming.

Build
----------
Install flvlib: http://pypi.python.org/pypi/flvlib/
<pre>
sudo apt-get install python-flup libpcap-dev lighttpd
sudo make install
</pre>

Managment
----------
* Web view at: http://127.0.0.1/cache. 
* `/etc/init.d/xcache start|stop|restart` controls the daemon.
* `xcache-list` list the entries.
* `xcache-clear` clears all entries.

