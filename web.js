
var a = {hole:0, complete:0, rst:0};
a['d']++;
console.log(a);

var rl = require('readline').createInterface(process.stdin, process.stdout);
//var io = require('socket.io').listen(8080);
var cached = {};
rl.on("line", function(s) {
	a = s.split(' ');
/*	if (a[0] == 's_cached') {
		cached[a[5]]++;
		console.log(cached);
		*/
	}
//	io.sockets.emit('news', new Date());
);
//setInterval(function() {
//	io.sockets.emit('news', new Date());
//}, 1000);

