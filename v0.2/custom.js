$( document ).ready(function() {
    var menu = document.getElementById ("menu-bar");
    var right_buttons = menu.querySelector (".right-buttons");

    var print_button = right_buttons.firstElementChild;
    var version_button = document.createElement ("i");
    version_button.setAttribute ("id", "version-toggle");
    version_button.classList.add ("fa")

    var version_name = document.createElement ("span");
    version_name.textContent = "v0.2";

    version_button.appendChild (version_name);
    
    version_button.setAttribute ("title", "change version");

    right_buttons.removeChild (print_button);
    right_buttons.appendChild (version_button);
    right_buttons.appendChild (print_button);
    
    $("#version-toggle").click(function(){
        if($('.version-popup').length) {
            $('.version-popup').remove();
        } else {
            var popup = $('<div class="version-popup"></div>')
            .append($('<div class="version" id="v1.3">v1.3</div>'))
	        .append($('<div class="version" id="v1.2">v1.2</div>'))
	    	.append($('<div class="version" id="v1.0">v1.0</div>'))
                .append($('<div class="version" id="v0.2">v0.2</div>'));


            popup.insertAfter(this);

            $('.version').click(function(){
		var version = $(this).attr('id');
		set_version (version);
            });
        }
    });


    function set_version (version) {
	var url = window.location.href;
	var domain = window.location.host;
	var pos = url.search (domain);
	
	var page = url.substring (pos + domain.length + 1);
	var protocol = url.substring (0, pos);

	n_page = protocol + domain + "/vjoule/" + version + "/";
	console.log (n_page);
	window.location.href = n_page;
    }

});
