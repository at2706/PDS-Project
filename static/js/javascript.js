$(document).ready(function(){
    $('#postInput').on('keyup focus', function() {
    	$('#charCounter').html(this.value.length);
    });
});