$(document).ready(function(){
	//Home Page post counter
    $('#postInput').on('keyup focus', function() {
    	$('#charCounter').html(this.value.length);
    });

    //Form Validation
    $('#password2').on('keyup focus', function() {
    	var p1 = $('#password1').get(0);
    	if(p1.value != this.value){
    		this.setCustomValidity("Passwords do not match!");
    	}
    	else
    		this.setCustomValidity("");
    });

    $('#email').on('keyup focus', function() {

        //EMAIL_CHAR_LIMIT
        if(this.value.length > 60){
            this.setCustomValidity("Email is too long!");
        }
        else{
            this.setCustomValidity("");
        }
    });

    $('#first_name').on('keyup focus', function() {
        var fn = $('#first_name').get(0);
        var ln = $('#last_name').get(0);
        var length = fn.value.length + ln.value.length;

        //NAME_CHAR_LIMIT
        if(length > 60){
            fn.setCustomValidity("Full Name is too long!");
        }
        else{
            fn.setCustomValidity("");
        }
    });
});