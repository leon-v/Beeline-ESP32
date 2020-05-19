class Index{
	pages = {};
	page = null;
	constructor() {
		
		window.hashchangeIndex = this;
		window.addEventListener('hashchange', function(event){
			this.hashchangeIndex.hashChange(event);
		}, false);
	}

	getPageData(){

		var data = location.hash.split('?');

		return {
			name: (data[0] || '').substr(1),
			params: new URLSearchParams('?' + (data[1] || ''))
		};
	}

	hashChange(event){

		var pageData = this.getPageData();

		var page = this.pages[pageData.name] || null;

		if (!page) {
			alert('Page ' + pageData.name + ' not found');
			return;
		}

		if (this.page){
			this.page.hide();
		}

		this.page = page;

		this.page.show();

		for(var entry of pageData.params.entries()) {
			var name = entry[0];
			var value = entry[1];
			
			this.page[name] = value;			
		}
	}


	setPage(page, paramaters){

		var searchParams = new URLSearchParams;

		var key;
		for (key in paramaters){
			searchParams.append(key, paramaters[key]);
		}
		
		location.hash = '#' + page.name + '?' + searchParams.toString();
	}

	add(page){

		this.pages[page.name] = page;

		var pageData = this.getPageData();

		if (pageData.name == page.name) {
			this.hashChange();
		}
	}
}