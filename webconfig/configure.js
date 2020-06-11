class Configure extends Page{
	name = "configure";
	label = "Configure";
	moduleConfig = {};
	settings = {};
	data = {};
	panels = {};
	constructor(){
		super('body');
		// console.log('Configure', this);

		this.addNavBarItem();

		this.ipBlock = this.appendOnce(Block, 'ip', 'Device IP Address');

		this.ipBlock.form = this.ipBlock.appendOnce(Form, 'form');

		this.ipBlock.form.ipInput = this.ipBlock.form.appendOnce(IpInput, 'ip', 'IP Address');

		this.ipBlock.form.configureButton = this.ipBlock.form.appendOnce(Button, 'config', 'Configure', 'primary');
		this.ipBlock.form.configureButton.click = function(event){
			this.page.getDeviceModules();
		}
	}

	set ip(value){
		this.ipBlock.form.ipInput.setIp(value);
	}

	get ip(){
		return this.ipBlock.form.ipInput.getIp();
	}

	getDeviceModules(){

		this.ipBlock.form.configureButton.disable();

		var url = 'http://' + this.ip + '/rest/modules';

		deviceGet(url, this, function(self, response){
			self.gotDeviceModules(response);
		});
	}

	gotDeviceModules(response){

		this.ipBlock.form.configureButton.enable();

		// if (this.modules){
		// 	this.modules.remove();
		// }

		this.modules = this.appendOnce(Block, 'Modules', 'Modules');
		

		var index;
		for (index in response){

			var moduleName = response[index];

			var panel = this.modules.appendOnce(ModulePanel, moduleName);

			this.panels[moduleName] = panel;

			if (this.settings[moduleName]){
				this.settings[moduleName].constructor(this, panel, moduleName);
			}
			else{
				this.settings[moduleName] = new ConfigureSetting(this, panel, moduleName);
			}
			
			if (this.data[moduleName]){
				this.data[moduleName].constructor(this, panel, moduleName);
			}
			else{
				this.data[moduleName] = new ConfigureData(this, panel, moduleName);
			}
			

			this.modules.append(HorizontalRule);

			this.getModuleOptions(moduleName);
		}
	}

	getModuleOptions(moduleName){
		
		var url = 'http://' + this.ip + '/rest/modules/' + encodeURIComponent(moduleName);

		deviceGet(url, this, function(self, response){

			self.moduleConfig[moduleName] = response;

			self.panels[moduleName].name		= response.name;
			self.panels[moduleName].description	= response.description;

			self.settings[moduleName].init(response.settings);

			self.data[moduleName].init(response.data);
		});

	}
}

class ConfigureData{
	configure = null;
	panel = null;
	dataItems = {};
	moduleName = null;
	constructor(configure, panel, moduleName){
		this.configure = configure;
		this.panel = panel;
		this.moduleName = moduleName;
	}

	init(data){

		var name;
		for (name in data){
			if (this.dataItems[name]){
				this.dataItems[name].constructor(this, name, data[name], this.panel);
			}
			else{
				this.dataItems[name] = new ConfigureDataItem(this, name, data[name], this.panel);
			}
			
		}
	}
}

class ConfigureDataItem{
	dataItem = null;
	panel = null;
	dataItemView = null;
	name = null;
	configureData = null;
	updateTimer = null;
	constructor(configureData, name, dataItem, panel){

		this.configureData = configureData;
		this.name = name;
		this.dataItem = dataItem;
		this.panel = panel;

		var constructor = eval(this.dataItem.class || 'Preformatted');

		this.dataItemView = this.panel.appendOnce(constructor, this.dataItem.name);

		copyProperties(this.dataItem, this.dataItemView);

		if (this.dataItem.updateInterval){
			console.log('constructor', this.name);
			this.updateValue();
		}
		
	}

	updateValue(){

		var url = 'http://' + this.configureData.configure.ip + '/rest/modules/' + encodeURIComponent(this.configureData.moduleName) + '/data/' + encodeURIComponent(this.name) + '/value';

		deviceGet(url, this, function(self, response, event){
			self.updateValueCallback(response, event);
		});

	}

	updateValueCallback(response, event) {

		if (response !== null){
			this.dataItemView.value = response;
		}

		var self = this;
		this.updateTimer = setTimeout(function(){

			if (self.panel.visible){
				self.updateValue();
			}
			else{
				self.updateValueCallback(null, null);
			}

		}, this.dataItem.updateInterval);
	}

}

class ConfigureSetting{
	configure = null;
	panel = null;
	name = null;
	settings = {};
	constructor(configure, panel, name){
		this.configure = configure;
		this.panel = panel;
		this.name = name;

		this.form = this.panel.appendOnce(Form, 'form');
	}

	init(settings){

		// this.form.empty();
	
		var index;
		for (index in settings){

			var setting = settings[index];

			if (this.settings[setting.name]){
				this.settings[setting.name].load(setting);
				// this.settings[setting.name].reConstructor(this, setting);
			}
			else{
				this.settings[setting.name] = new ConfigureSettingInput(this, setting);
			}
			
		}

		if (!this.form.saveButton){
			this.form.saveButton = this.form.appendOnce(Button, 'save', 'Save', 'primary');
			this.form.saveButton.setting = this;
			this.form.saveButton.click = function(event){
				this.setting.save(event);
			}
		}
	}

	append(...args){
		return this.form.append(...args);
	}

	appendOnce(...args){
		return this.form.appendOnce(...args);
	}

	save(event){

		var request = [];

		var settingName;
		for (settingName in this.settings) {

			var newValue = {
				name : settingName,
				value : this.settings[settingName].value
			}

			request.push(newValue);
		}

		console.log("save", request);

		var url = 'http://' + this.configure.ip + '/rest/modules/' + encodeURIComponent(this.name);

		devicePost(url, request, this, function(self, response){
			self.init(response.settings);
		});
	}
}

class ConfigureSettingInput{
	setting = null;
	inputOptions = null;
	input = null;
	constructor(setting, inputOptions){
		this.setting = setting;
		this.inputOptions = inputOptions;

		this.input = this.getInput(inputOptions);

		this.setInputOptions(inputOptions);
	}

	getType(inputOptions){
		var defaultType = inputOptions.options ? 'select' : 'text';
		return inputOptions.inputType || defaultType;
	}

	getInput(inputOptions){

		if (inputOptions.inputClass){
			var constructor = eval(inputOptions.inputClass);
			return this.setting.appendOnce(constructor, inputOptions.name);
		}

		switch (this.getType(inputOptions)){

			case 'text':
				return this.setting.appendOnce(TextInput, inputOptions.name);

			case 'select':
				return this.setting.appendOnce(SelectInput, inputOptions.name);

			case 'checkbox':
				var constructor = inputOptions.options ? MultiCheckboxInput : SingleCheckboxInput;
				return this.setting.appendOnce(constructor, inputOptions.name);

			default:
				console.error('Type ' + type + ' not handled');
			break;
		}
	}

	setInputOptions(inputOptions){

		var optionIndex;
		for (optionIndex in inputOptions){
			this.input[optionIndex] = inputOptions[optionIndex];
		}
	}

	get value(){
		var typedValue = this.inputOptions.value || this.inputOptions.default
		return convertToTypeOf(typedValue, this.input.value);
	}

	load(values){
		this.input.load(values);
	}
}