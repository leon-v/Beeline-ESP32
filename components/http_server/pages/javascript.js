function checkboxChange(element){
	var id = element.getAttribute('for');
	document.getElementById(id).value = element.checked ? 1 : 0;
}