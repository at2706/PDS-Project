$(document).ready(function(){
	//Home Page post counter
    $('#postInput').on('keyup focus', function() {
    	$('#charCounter').html(this.value.length);
    });

    //Registration Form Validation
    $('#password2').on('keyup focus', function() {
    	var p1 = $('#password1');
    	if(p1.value != this.value){
    		this.setCustomValidity("Passwords do not match!");
    	}
    	else
    		this.setCustomValidity("");
    });
});