$(function(){
	$('.article_flex_wrapper#img_article_1').hover( function(){
		$('img#img_article_1').animate({'opacity': 1}, 500);
	}, function(){
		$('img#img_article_1').animate({'opacity': 0}, 500);
	});

	$('.article_flex_wrapper#img_article_2').hover( function(){
		$('img#img_article_2').animate({'opacity': 1}, 500);
	}, function(){
		$('img#img_article_2').animate({'opacity': 0}, 500);
	});

	$('.article_flex_wrapper#img_article_3').hover( function(){
		$('img#img_article_3').animate({'opacity': 1}, 500);
	}, function(){
		$('img#img_article_3').animate({'opacity': 0}, 500);
	});
});
