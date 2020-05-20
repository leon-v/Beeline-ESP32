function int2ip (ipInt) {
    return ( (ipInt>>>24) +'.' + (ipInt>>16 & 255) +'.' + (ipInt>>8 & 255) +'.' + (ipInt & 255) );
}

function ip2int(ip) {
    return ip.split('.').reduce(function(ipInt, octet) { return (ipInt<<8) + parseInt(octet, 10)}, 0) >>> 0;
}

function deviceGet(url, requestContext, callback, timeout) {
	
	timeout = timeout || 30000;
	
	$.ajax({
		url: url,
		timeout: timeout,
		type: 'GET',
		success: function(response, event){
			callback(requestContext, response, event);
		},
		error: function(response, event) {
			callback(requestContext, null, event);
		}
	});
}

function devicePost(url, data, requestContext, callback, timeout) {
	
	timeout = timeout || 30000;
	
	$.ajax({
		url: url,
		timeout: timeout,
		type: 'POST',
		data: JSON.stringify(data),
		success: function(response, event){
			callback(requestContext, response, event);
		},
		error: function(response, event) {
			callback(requestContext, null, event);
		}
	});
}

function convertToTypeOf(typedVar, input) {

	if (typeof(typedVar) == typeof(input)) {
		return input;
	}

	console.log(typeof(typedVar), typeof(input));

	return {
		'boolean': (v) => (v == 'true') || (v == 1), // eslint-disable-line eqeqeq
		'number': Number,
		'string': String,
	}[typeof typedVar](input);
};

function copyProperties(fromObject, toObject){

	var index;
	for (index in fromObject){
		toObject[index] = fromObject[index];
	}
}
