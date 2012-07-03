var chart1; // globally available

var options = {
	chart: {
					 renderTo: 'container',
					 defaultSeriesType: 'scatter'
				 },
	title: {
					 text: 'Fruit Consumption'
				 },
	xAxis: {
					 categories: []
				 },
	yAxis: {
					 title: {
										text: 'Units'
									}
				 },
	series: []
};

$(document).ready(function() {

	var socket = io.connect('http://114.112.46.58:8080/');
	socket.on('news', function (data) {
		console.log(data);
	});

	$.get('/xcache-web/a.csv', function(data) {
		// Split the lines
		var lines = data.split('\n');

		// Iterate over the lines and add categories or series
		$.each(lines, function(lineNo, line) {
			var items = line.split(',');

			// header line containes categories
			if (lineNo == 0) {
				$.each(items, function(itemNo, item) {
					if (itemNo > 0) options.xAxis.categories.push(item);
				});
			}

			// the rest of the lines contain data with their name in the first position
			else {
				var series = {
					data: []
				};
				$.each(items, function(itemNo, item) {
					if (itemNo == 0) {
						series.name = item;
					} else {
						series.data.push(parseFloat(item));
					}
				});
				options.series.push(series);
			}
		});

		// Create the chart
		var chart = new Highcharts.Chart(options);
	});
});

