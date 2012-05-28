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
* Web view at: `http://localhost/cache` 
* `/etc/init.d/xcache start|stop|restart` controls the daemon.
* `xcache-list` list the entries.
* `xcache-clear` clears all entries.
* receive `GET /host/path/to/video.mp4` and serve cached file or redirect to `http://host/path/to/video.mp4`
