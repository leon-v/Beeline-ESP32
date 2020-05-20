class Scan extends Page{
	
	name = "scan";
	label = "Scan";
	scanDelay = 50;
	totalIpsToScan = 0;
	totalIpsResponded = 0;
	remainingIpsToScan = 0;
	cancel = false;
	ipBlock = {};

	constructor(){
		super('body');

		this.addNavBarItem();

		this.ipBlock = this.append(Block, 'IP Range');

		this.ipBlock.form = this.ipBlock.append(Form);


		this.ipBlock.form.fromIpInput = this.ipBlock.form.append(IpInput, 'From IP Address', '192.168.178.1');
		this.ipBlock.form.toIpInput = this.ipBlock.form.append(IpInput, 'From IP Address', '192.168.178.255');

		this.ipBlock.form.scanButtonGroup = this.ipBlock.form.append(InputGroup);

		this.ipBlock.form.scanButtonGroup.scanButton = this.ipBlock.form.scanButtonGroup.append(Button, 'Scan', 'primary');
		this.ipBlock.form.scanButtonGroup.scanButton.click = function(event) {
			this.page.startScan(event);
		};

		this.ipBlock.form.scanButtonGroup.cancelButton = this.ipBlock.form.scanButtonGroup.append(Button, 'Cacnel', 'warning');
		this.ipBlock.form.scanButtonGroup.cancelButton.click = function(event) {
			this.page.cancel = true;
		};

		this.ipBlock.form.scanButtonGroup.cancelButton.disable();

		this.tableBlock = this.append(Block, 'Results');

		this.tableBlock.requestProgress = this.tableBlock.append(Progress, "Requested: ");
		this.tableBlock.responseProgress = this.tableBlock.append(Progress, "Responded: ");
		this.tableBlock.totalProgress = this.tableBlock.append(Progress, "Total Progress: ");
		

		this.tableBlock.table = this.tableBlock.append(Table, 'Devices', ['IP Address', 'Version', 'Name']);
	}

	startScan(){

		this.totalIpsToScan = 0;
		this.totalIpsResponded = 0;

		this.tableBlock.table.clear();

		this.ipBlock.form.scanButtonGroup.scanButton.disable();
		this.ipBlock.form.scanButtonGroup.cancelButton.enable();

		this.toIpInt = ip2int(this.ipBlock.form.toIpInput.getIp());

		this.fromIpInt = ip2int(this.ipBlock.form.fromIpInput.getIp());

		this.totalIpsToScan+= (this.toIpInt - this.fromIpInt) + 1;

		this.scanIp(this.fromIpInt);

		self.updateProgress();
	}

	updateProgress(){

		// Request
		var requestRatio = this.remainingIpsToScan / this.totalIpsToScan;

		this.tableBlock.requestProgress.setPercent(requestRatio * 100);

		// Response
		var respondedRatio = this.totalIpsResponded / this.totalIpsToScan;

		this.tableBlock.responseProgress.setPercent(respondedRatio * 100);

		var totalRatio = (requestRatio + respondedRatio) / 2;

		this.tableBlock.totalProgress.setPercent(totalRatio * 100);
	}

	scanIp(ipInt){

		if ( (ipInt > this.toIpInt) || (this.cancel) ) {
			this.ipBlock.form.scanButtonGroup.scanButton.enable();
			this.ipBlock.form.scanButtonGroup.cancelButton.disable();
			this.cancel = false;
			return;
		}

		this.remainingIpsToScan = this.totalIpsToScan - (this.toIpInt - ipInt);

		this.updateProgress();

		var context = {
			ip: int2ip(ipInt),
			self: this
		}

		var url = 'http://' + context.ip + '/rest/version';

		deviceGet(url, context, this.scanIpCallback);
		
		var self = this;
		setTimeout(function(){
			self.scanIp(ipInt + 1);
		}, this.scanDelay);
	}

	scanIpCallback(context, response, event){

		var self = context.self;

		self.totalIpsResponded++;

		self.updateProgress();

		if (!response){
			return;
		}

		var link = new Anchor;
		link.setLabel(context.ip);
		link.value = context.ip;
		link.page = self;
		link.click = function(event){
			this.page.ipSelected(this.value, event);
		}

		self.tableBlock.table.appendRow([
			link,
			response.firmwareName,
			response.deviceName
		]);

		
	}

	ipSelected(value, event){
		index.setPage(index.pages.configure, {
			ip:value
		});
	}
}