
var io = require('socket.io').listen(8889);
require('readline')
	.createInterface(process.stdin, process.stdout)
	.on("line", function(s) { io.sockets.emit('news', s); });

