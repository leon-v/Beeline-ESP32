/*
*
* Scan IPs
*
*/
var ipsScanned = null;

function scanIPs(ipPrefix, suffixFrom, suffixto, endpoint, callback, completeCallback){

	suffixFrom = parseInt(suffixFrom);
	suffixto = parseInt(suffixto);

	var ipsToScan = suffixto - suffixFrom + 1;
	var ipsScanned = 0;

	var scanIPsCallback = function(response, ipAddress){

		ipsScanned++;
		var scannedRatio =  ipsScanned / ipsToScan;

		callback(response, ipAddress, scannedRatio);

		if (scannedRatio < 1) {
			return;
		}

		completeCallback();
	}

	for (var ipSuffix = suffixFrom; ipSuffix <= suffixto; ipSuffix++) {
		scanIP(ipPrefix, ipSuffix, endpoint, scanIPsCallback);
	}
}

function postToDevice(ipAddress, endpoint, data, callback) {

	var url = 'http://' + ipAddress + '/rest' + endpoint;

	$.ajax({
		url: url,
		timeout: 120000,
		type: 'POST',
		data: JSON.stringify(data),
		success: function(response){
			callback(response, ipAddress);
		},
		error: function(response) {
			callback(null, ipAddress);
		}
	});
}

function getFromDevice(ipAddress, endpoint, callback, timeout) {

	var url = 'http://' + ipAddress + '/rest' + endpoint;

	timeout = timeout || 30000;

	$.ajax({
		url: url,
		timeout: timeout,
		type: 'GET',
		success: function(response){
			callback(response, ipAddress);
		},
		error: function(response) {
			callback(null, ipAddress);
		}
	});
}
function scanIP(ipPrefix, ipSuffix, endpoint, callback) {

	setTimeout(function(){

		var ipAddress = ipPrefix + '.' + ipSuffix;

		getFromDevice(ipAddress, endpoint, callback);

	}, 100 * ipSuffix);
}

function getIpPrefixFrom(className) {

	var ipPrefixArray = $(className).serializeArray();

	var ipPrefix = '';

	for (index in ipPrefixArray) {

		var item = ipPrefixArray[index];

		if (item.name.substring(0, 2) != 'ip') {
			continue;
		}

		if (ipPrefix) {
			ipPrefix+= '.';
		}

		ipPrefix+= item.value;

	}

	return ipPrefix;
}

function setPreset(settings, parent){

	for (index in settings) {

		var value = settings[index];

		$(parent).find('[name=' + index + ']').val(value);
	}
}

function setIP(ipAddress, selector) {

	var ipAddressArray = ipAddress.split('.');

	for (index in ipAddressArray) {

		var ipAddressSegment = ipAddressArray[index];

		elementIndex = parseInt(index) + 1;

		var element = $(selector).find('[name=ip' + elementIndex + ']');

		element.val(ipAddressSegment);
	}
}

function getPresetFromNames(namesCSV, parent){

	var names = namesCSV.split(",");

	var settings = {};

	for (index in names){

		var name = names[index];

		settings[name] = $(parent).find('[name=' + name + ']').val();
	}

	return settings;
}

function getPresetFromElement(element) {
	return  $(element).data();
}

var presetStorageKey = 'ipPreset';

function getPresetFromLocalStorage(){
	return JSON.parse(window.localStorage.getItem(presetStorageKey));
}

function setPresetToLocalStorage(preset){
	window.localStorage.setItem(presetStorageKey, JSON.stringify(preset));
}


function addLoadingTo(element){

	element.find('.alert').remove();

	$(element).prepend($('\
		<div class="alert alert-dark loading" role="alert">\
			Loading...\
		</div>\
	'));
}

function removeLoadingFrom(element){
	$(element).find('.loading').remove();
}

function addErrorTo(element, message) {
	element.prepend($('\
		<div class="alert alert-danger" role="alert">\
			' + message + '\
		</div>\
	'));
}

function removeErrorFrom(element){
	$(element).find('.alert-danger').remove();
}

function getUrlParameter(parameter){
	var url = window.location.href;
    parameter = parameter.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
    var regex = new RegExp('[\\?|&]' + parameter.toLowerCase() + '=([^&#]*)');
    var results = regex.exec('?' + url.toLowerCase().split('?')[1]);
    return results === null ? '' : decodeURIComponent(results[1].replace(/\+/g,''));
}


function setUrlParameter(url, key, value) {
    var key = encodeURIComponent(key),
        value = encodeURIComponent(value);
	
    var baseUrl = url.split('?')[0],
        newParam = key + '=' + value,
        params = '?' + newParam;

    if (url.split('?')[1] === undefined){ // if there are no query strings, make urlQueryString empty
        urlQueryString = '';
    } else {
        urlQueryString = '?' + url.split('?')[1];
    }

    // If the "search" string exists, then build params from it
    if (urlQueryString) {
        var updateRegex = new RegExp('([\?&])' + key + '=[^&]*');
        var removeRegex = new RegExp('([\?&])' + key + '=[^&;]+[&;]?');

        if (value === undefined || value === null || value === '') { // Remove param if value is empty
            params = urlQueryString.replace(removeRegex, "$1");
            params = params.replace(/[&;]$/, "");
    
        } else if (urlQueryString.match(updateRegex) !== null) { // If param exists already, update it
            params = urlQueryString.replace(updateRegex, "$1" + newParam);
    
        } else if (urlQueryString == '') { // If there are no query strings
            params = '?' + newParam;
        } else { // Otherwise, add it to end of query string
            params = urlQueryString + '&' + newParam;
        }
    }

    // no parameter was set so we don't need the question mark
    params = params === '?' ? '' : params;

    return baseUrl + params;
}

function setUrlParameters(paramters, hash) {

	if (hash) {
		hash = '#' + hash;
	}

	var url = location.protocol + '//' + location.host + location.pathname;
	var hash = hash || window.location.hash;

	for (key in paramters) {
		url = setUrlParameter(url, key, paramters[key]);
	}

	if (hash) {
		url+= hash;
	}

	window.location.href = url;
}

function setPage(container, paramters) {

	setUrlParameters(paramters, container);
	showPage();
}

var initFunctions = {};

function addInitFunction(container, initFunction) {
	initFunctions[container] = initFunction;
}

function showPage(){

	var hash = window.location.hash;

	var container = hash.substring(1);

	if (!container) {
		return;
	}

	var pageClass = container + '-container';

	var allPages = $('body main');

	var page = allPages.filter('.' + pageClass);

	if (!page.length) {
		console.log('!page', '.' + pageClass, main);
		return;
	}

	allPages.removeClass('current');

	page.addClass('current');

	if (initFunctions[container] != undefined) {
		initFunctions[container](page);
	}
}

function startStatusPoll(ipAddress, panel, name, delay) {

	if (!panel.is(':visible')) {
		return;
	}

	var statusBlock = panel.find('.status');

	statusBlock.show();

	var callback = function(status){

		status = status || {
			message: 'Error getting status',
			alert: 'danger'
		};

		statusBlock.text(status.message);
		statusBlock.attr('class', 'status alert alert-' + status.alert);

		if (panel.lastStatus != status.message) {
			
			if (panel.statusChange) {
				panel.statusChange(status);
			}

			panel.lastStatus = status.message;
		}

		setTimeout(function(){
			startStatusPoll(ipAddress, panel, name, delay);
		}, delay);
	}

	getFromDevice(ipAddress, '/component/' + encodeURIComponent(name) + '/status', callback);
}

$(document).on('show.bs.modal', '#modal', function (event) {
			
	var modal = $(this);
	var options = modal.get(0).options;
	
	modal.find('.modal-title').text(options.title);
	modal.find('.modal-body').text(options.body);

	var modalFooter = modal.find('.modal-footer');

	modalFooter.children().remove();

	var buttons = options.buttons || [];

	for (index in buttons){

		var buttonOptions = buttons[index];

		var button = $('<button type="button" class="btn"></button>');

		buttonOptions.type = buttonOptions.type || "primary";
		if (buttonOptions.type) {
			button.addClass('btn-' + buttonOptions.type);
		}

		if (buttonOptions.text) {
			button.text(buttonOptions.text);
		}


		if (buttonOptions.onclick) {
			button.on('click', buttonOptions.onclick);
		}

		modalFooter.append(button);
	}

	var closeButton = $('<button type="button" class="btn btn-secondary" data-dismiss="modal">Close</button>');
	modalFooter.append(closeButton);
});

function showModal(options) {
	var modal = $('#modal');
	modal.get(0).options = options;
	modal.modal('show');
}