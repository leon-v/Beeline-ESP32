class Page extends Container{
	name = "page";
	label = "Page";
	navBarItem = null;

	constructor(parent) {
		super(parent);
		// console.log('Page', this, parent);

	}

	addNavBarItem(label, name) {
		this.navBarItem = new NavBarItem(label || this.label, name || this.name);
		
		this.navBarItem.page = this;
		this.navBarItem.click = function(event){
			this.page.navBarItemClicked(event);
		}
	}

	navBarItemClicked(event){
		index.setPage(this);
	}
}