$(document).on('click', '.mainMenuItem, .navbar-brand', function(){

	var data = $(this).data();

	setPage(data.container);
});



function ipScanAddRow(response, ipAddress){

	if (!response){
		return;
	}

	if (response.firmwareName == undefined) {
		return;
	}

	if (response.firmwareName != 'Beeline ESP32') {
		return;
	}

	var tr = $('<tr></tr>');
	tr.addClass('scanItem');

	var td = $('<td></td>');
	tr.append(td);

	var link = $('<a></a>');
	link.text(ipAddress || null);
	link.attr('href', 'javascript:;');
	link.on('click', function(){
		setPage('configure', {
			ipAddress: ipAddress
		});
		// setIP(ipAddress, '.configure-container .ipAddress');
	});
	td.append(link);

	var td = $('<td></td>');
	td.text(response.firmwareVersion || null);
	tr.append(td);

	var td = $('<td></td>');
	td.text(response.deviceName || null);
	tr.append(td);

	$('.devices-container .scanResults .table tbody').append(tr);
}

function ipScanComplete(){
	$('.devices-container .startScan').attr('disabled', false);
}

function ipScanCallback(response, ipAddress, scannedRatio) {

	var percent = Math.round(scannedRatio * 100);

	$('.devices-container .scanResults .scanProgress').html(percent + '%');

	ipScanAddRow(response, ipAddress);
}

$(document).on('click', '.devices-container .startScan', function(){

	$('.devices-container .startScan').attr('disabled', true);

	$('.devices-container .scanResults .scanProgress').html('0%');

	$('.devices-container .scanResults table tbody tr.scanItem').remove();

	$('.devices-container .scanResults').removeClass('hidden');

	var ipPrefix = getIpPrefixFrom('.devices-container .ipScanForm');

	var suffixFrom = $('.devices-container input[name=rangeMin]').val();
	var suffixTo = $('.devices-container input[name=rangeMax]').val();

	scanIPs(ipPrefix, suffixFrom, suffixTo, '/version', ipScanCallback, ipScanComplete); // Don't forget the trailing slash
});


var presetParent = '.devices-container';

$(document).on('click', '.devices-container .ipPreset button.isPreset', function(){

	var preset = getPresetFromElement(this);

	setPreset(preset, presetParent);
});

function loadPreset() {

	var preset = getPresetFromLocalStorage();

	if (!preset) {
		return;
	}

	setPreset(preset, presetParent);
}

$(document).on('click', '.devices-container .ipPreset button.isLoad', function(){
	loadPreset();
});



$(document).on('click', '.devices-container .ipPreset button.isSave', function(){

	var namesCSV = $(this).data('names');

	var preset = getPresetFromNames(namesCSV, presetParent);

	setPresetToLocalStorage(preset);

	setPreset(preset, presetParent);

});

function configureRenderComponentInput(parent, setting) {

	var input = $('<input class="form-control" aria-describedby="basic-addon2">');
	parent.append(input);


	input.val(setting.value);
	input.attr('name', setting.name);
	input.attr('type', setting.inputType || 'text');
}

function configureRenderComponentSelect(parent, settings) {

	var select = $('<select class="form-control" aria-describedby="basic-addon2"></select>');
	select.attr('name', settings.name);

	var option = $('<option></option>');
	option.text('Select ' + settings.name);
	option.attr('disabled', 'disabled');
	select.append(option);

	var options = settings.options || {};

	for (optionKey in options) {

		var optionValue = options[optionKey];

		optionValue = new DOMParser().parseFromString(optionValue, "text/html").documentElement.textContent;

		optionValue = optionValue.replace(/\s/g, '&nbsp;');

		var option = $('<option></option>');
		option.attr('value', optionKey);
		option.html(optionValue);

		if (optionKey == settings.value) {
			option.attr('selected', 'selected');
		}

		select.append(option);
	}

	parent.append(select);
}

function configureRenderComponentMultiCheckbox(parent, variable) {

	var options = variable.settings.options || {};

	for (optionKey in options) {

		var optionValue = options[optionKey];

		var htmlName = new DOMParser().parseFromString(variable.name, "text/html").documentElement.textContent;
		var htmlOptionKey = new DOMParser().parseFromString(optionKey, "text/html").documentElement.textContent;

		var formCheck = $('\
			<div class="form-check">\
				<input type="checkbox" class="form-check-input" id="' + htmlOptionKey + '" name="' + htmlName + '[' + htmlOptionKey + ']">\
				<label class="form-check-label" for="' + htmlOptionKey + '">' + htmlOptionKey + '</label>\
			</div>\
		');

		var checkbox = formCheck.find('input');

		var checked = variable.settings.value || [];

		var index = checked.indexOf(optionKey);

		if (index >= 0) {
			checkbox.prop('checked', true);
		}

		parent.append(formCheck);
	}
}

function configureRenderComponentCheckbox(parent, variable) {

	var htmlName = new DOMParser().parseFromString(variable.name, "text/html").documentElement.textContent;

	var formCheck = $('\
		<div class="form-check">\
			<input type="checkbox" class="form-check-input" name="' + htmlName + '" value="1">\
		</div>\
	');

	var checkbox = formCheck.find('input');

	if (variable.settings.value) {
		checkbox.prop('checked', true);
	}
	
	parent.append(formCheck);
}

function configureRenderComponentVariable(table, setting) {

	var row = $('<tr></tr>');
	table.append(row);

	var label = $('<th></th>');
	row.append(label);
	label.text(setting.label || setting.name);
	label.css('min-width', '100px');

	var cell = $('<td></td>');

	row.append(cell);

	var defaultType = 'text';

	if (setting.options || null) {
		defaultType = 'select';
	}

	var type = setting.inputType || defaultType;
	
	switch (type) {

		case 'select':
			configureRenderComponentSelect(cell, setting);
		break;

		case 'multiCheckbox':
			configureRenderComponentMultiCheckbox(cell, setting);
		break;

		case 'checkbox':
			configureRenderComponentCheckbox(cell, setting);
		break;

		default:
		case 'text':
		case 'password':
		case 'string':
			configureRenderComponentInput(cell, setting);
		break;
	}
}

function configureRenderComponentSettings(panel, settingsResult, ipAddress){

	var panelTable = $('<table></table>');

	panel.content.append(panelTable);

	panel.description = panel.find('.description');
	panel.description.text(settingsResult.description);

	var settings = settingsResult.settings;

	panel.form.data('settings', settings);

	for (index in settings) {

		var setting = settings[index];

		configureRenderComponentVariable(panelTable, setting);
	}

	if (settings.statusPoll) {
		// startStatusPoll(ipAddress, panel, settings.name, settings.statusPoll);
		console.log('!startStatusPoll');
	}
}

function configureRenderComponent(name, container, ipAddress) {

	var key = 'panel' + index;

	var panel = $(container).find('.template').clone();
	panel.attr('class', 'panel');

	container.append(panel);

	panel.title = panel.find('.title');
	panel.title.text(name);
	panel.title.attr('href', '#' + key);

	panel.collapse = panel.find('.panel-collapse');
	panel.collapse.attr('id', key);

	panel.content = panel.collapse.find('.content');

	panel.saveButton = panel.collapse.find('button.save');
	panel.saveButton.data('name', name);

	panel.form = panel.collapse.find('form');

	panel.statusChange = function(status) {

		var extra = status.extra || null;

		if (!extra) {
			return;
		}
		var newIpAddress = extra.ipAddress || null;
		
		if (!newIpAddress) {
			return;
		}

		/*
		if (ipAddress != newIpAddress) {

			showModal({
				title: "Device IP address changed",
				body:	"The connected device has aquired the new IP address " + newIpAddress + ".\r\n" +
						"It is highly recomended to use \"WiFi Client\" mode for non-degraded performance.\r\n" +
						"Click \"Set Client Mode and Swap\" to set client mode and use the new IP address. This will likely cause your WiFI to reconnect so please be patient.\r\n",
				buttons: [
					{
						text: "Set Client Mode and Swap",
						onclick: function(event){

							var post = {vales: { Mode: 2 } };

							postToDevice(ipAddress, '/modules/WiFi', post, function(result) {

								if (!result) {
									alert("Failed to set Wifi Client mode");
									return;
								}

								setPage('configure', {
									ipAddress: newIpAddress
								});
							});
						}
					}
				]
			});
		}
		*/
	}

	addLoadingTo(panel.content);

	var callback = function(settingsResult){

		removeLoadingFrom(panel.content);

		if (!settingsResult) {

			panel.content.append($('\
				<div class="alert alert-danger" role="alert">\
					Failed to get settings for ' + name + '<br>\
					Please try again.\
				</div>\
			'));

			panel.description = panel.find('.description');
			panel.description.text('Failed to load description.');
			return;
		}

		panel.saveButton.show();

		configureRenderComponentSettings(panel, settingsResult, ipAddress);
	}


	getFromDevice(ipAddress, '/modules/' + encodeURIComponent(name), callback);

	panel.show();
}

function configureRenderComponents(ipAddress, modules) {

	if (!modules) {
		return;
	}

	var container = $('.configure-container .components');

	for (index in modules) {

		var name = modules[index];

		configureRenderComponent(name, container, ipAddress);
	}
}

addInitFunction('configure', function(page){

	var ipAddress = getUrlParameter('ipAddress');

	if (!ipAddress) {
		return;
	}

	setIP(ipAddress, $(page).find('.ipAddress'));

	$(page).find('.connect').click();
	
});

$(document).on('click', '.configure-container .connect', function() {

	$(this).prop('disabled', true);

	var ipAddress = getIpPrefixFrom('.configure-container .ipAddressForm');

	var components = $('.configure-container .components');

	components.find('.panel').remove();

	addLoadingTo(components);

	removeErrorFrom(components);

	var button = this;

	getFromDevice(ipAddress, '/modules', function(componentsResult) {

		removeLoadingFrom(components);

		$(button).prop('disabled', false);

		if (!componentsResult) {
			addErrorTo(components, 'Failed to load settings.<br>Please try again');
			return;
		}

		configureRenderComponents(ipAddress, componentsResult);
	});
});


function configureSaveComponent(form, result){

	form.find('.alert').remove();

	if (!result) {

		form.prepend($('\
			<div class="alert alert-danger" role="alert">\
				Failed to get response after saving.\
			</div>\
		'));

		return;
	}

	if (result.name) {

		form.prepend($('\
			<div class="alert alert-success" role="alert">\
				Settings saved.\
			</div>\
		'));
	}

	var variables = result.variables;

	for (variablesName in variables) {

		var variable = variables[variablesName];

		form.find('[name=' + variablesName + ']').val(variable.value);
	}
}

function buildFormValuesArray(values) {

	var outValues = {};

	for (index in values) {

		var value = values[index];

		var open = value.name.indexOf('[');
		var close = value.name.indexOf(']');

		if ( (open >= 0) && (close >= 0) && (close > open) ) {

			var name = value.name.substring(0, open);

			if (outValues[name] == undefined) {
				outValues[name] = Array();
			}

			var outValue = value.name.substring(open + 1, close);

			outValues[name][outValues[name].length] = outValue;
		}
		else{
			outValues[value.name] = value.value;
		}
	}

	return outValues;
}


$(document).on('click', '.configure-container .save', function(){

	var ipAddress = getIpPrefixFrom('.configure-container .ipAddressForm');

	var name = $(this).data('name');

	var form = $(this.form);

	addLoadingTo(form);

	var settings = form.data('settings');

	var values = buildFormValuesArray(form.serializeArray());

	console.log(settings, values);

	for (index in settings) {

		var setting = settings[index];

		var value = values[setting.name];

		// Save the value as the same type as the current value to preserve types
		setting.value = convertToTypeOf(setting.value, value);
	}

	postToDevice(ipAddress, '/modules/' + encodeURIComponent(name), settings, function(result) {

		configureSaveComponent(form, result);
	});

});

function convertToTypeOf(typedVar, input) {
	return {
		'boolean': (v) => (v == 'true') || (v == 1), // eslint-disable-line eqeqeq
		'number': Number,
		'string': String,
	}[typeof typedVar](input);
};
