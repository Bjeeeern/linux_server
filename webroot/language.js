$(function(){
	var days_until_cookie_expires = 7;

	function show_lang(lang)
	{
		$('#lang-switch option[value="'+ lang +'"]').prop('selected', true);
		$('[lang]').hide();
		$('[lang="'+ lang +'"]').show();
		$.cookie('lang', lang, { expires: days_until_cookie_expires });
	}

	var lang = $.cookie('lang');
	if(lang && lang.match(/^(en|jp)$/))
	{
		show_lang(lang);
	} 
	else 
	{
		//TODO(bjorn): Debug this.
		if("geolocation" in navigator)
		{
			navigator.geolocation.getCurrentPosition(function(position){
				var lat = position.coords.latitude;
				var lng = position.coords.longitude;
				$.getJSON('http://maps.googleapis.com/maps/api/geocode/json?latlng='+ 
									lat +','+ lng +'&sensor=true', null, 
									function(response){
										var country = 
											response.results[response.results.length-1].formatted_address;
										switch(country)
										{
											//case 'China':
											//case 'Taiwan':
											//case 'Hong Kong':
											//	show_lang('zh');
											//	break;
											case 'Japan':
												show_lang('jp');
												break;
											default:
												show_lang('en');
										}
									}
				).fail(function(error){
					// console.log('error: '+ error);
					show_lang('en');
				});
			}, function(error){ show_lang('en'); });
		} 
		else 
		{
			show_lang('en');
		}
	}

	$('#lang-switch').change(function(){
		var lang = $(this).val();

		if(lang && lang.match(/^(en|jp)$/))
		{
			show_lang(lang);
		}
		else
		{
			show_lang('en');
		}
	});
});
