class Template{

	selector = null;
	template = null;
	element = null;
	domElement = null;
	parent = null;
	page = null;
	appendedElements = {};

	constructor(selector, parent) {

		this.parent = parent;
		this.selector = selector;
		this.template = $(this.selector);

		this.element = $(this.template.html());

		this.element.template = this;

		this.domElement = this.element.get(0);
		this.domElement.template = this;

		// Cascade page though to children
		if (this.parent == 'body') {
			this.page = this;
		}
		else{
			if (this.parent && this.parent.template) {
				this.page = this.parent.template.page;
			}	
		}

		if (this.parent) {
			$(parent).append(this.element);
		}
		else{
			console.log('No parent for ', this.element);
		}
		
	}

	find(selector){

		var element = this.element.find(selector);

		if (element.length){
			return element;
		}
		
		var element = this.element.filter(selector);

		if (element.length){
			return element;
		}

		return null;
	}

	append(className, ...args){
		return new className(this.element, ...args);
	}

	appendOnce(className, key, ...args) {
		this.appendedElements[key] = this.appendedElements[key] || this.append(className, ...args);
		return this.appendedElements[key];
	}

	remove(){
		this.element.remove();
	}

	set page(page){
		this.page = page;
		this.element.page = this.page;
	}

	get page(){
		return this.page;
	}

	set click(callback){

		this.clickCallback = callback;

		this.element.click(function(event){
			this.template.clickCallback(event);
		});
	}

	hide(){
		this.element.hide();
	}

	show(){
		this.element.show();
	}
}