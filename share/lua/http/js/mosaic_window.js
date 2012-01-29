$(function(){
	$('#window_mosaic').dialog({
		autoOpen: false,
		width: 800,
		maxWidth: 1000,
		minWidth: 800,
		minHeight: 500,
		modal: true,
		buttons: {
			"<?vlc gettext("Create") ?>": function() {
				$(this).dialog("close");
			},
			"<?vlc gettext("Cancel") ?>" : function(){
				$(this).dialog("close")
			}
		}
	});
	$('#mosaic_bg').resizable({
		maxWidth: 780,
		ghost: true
	});
	$('#mosaic_tiles').draggable({
		maxWidth: 780,
		handle: 'h3',
		containment: [13,98,99999999,99999999],
		drag:function(event,ui){
			var xoff	=	ui.offset.left - $('#mosaic_bg').offset().left;
			var yoff	=	ui.offset.top - $('#mosaic_bg').offset().top-17;
			$('#mosaic_xoff').val(xoff);
			$('#mosaic_yoff').val(yoff);
		}
	});
	$('input','#mosaic_options').change(setMosaic);
	setMosaic();
});
function setMosaic(){
	var rows	=	Number($('#mosaic_rows').val());
	var cols	=	Number($('#mosaic_cols').val());
	var n		=	0;
	$('#mosaic_tiles').empty()
	$('#mosaic_tiles').append('<tr><td colspan="99"><h3 style="margin:0px;cursor:move; font-weight:normal" class="ui-widget-header"><?vlc gettext("Mosaic Tiles") ?></h3></td></tr>');
	for(var i=0;i<rows;i++){
		$('#mosaic_tiles').append('<tr>');
		for(var j=0;j<cols;j++){
			$('tr:last','#mosaic_tiles').append('<td class="mosaic">');
			$('td:last','#mosaic_tiles').append('<div id="mosaic_open__'+n+'" class="button icon ui-widget ui-state-default" title="Open Media" style="margin-top:49%"><span class="ui-icon ui-icon-eject"></span></div>');
			n++;
		}
	}
	$('.mosaic').resizable({
		alsoResize: '.mosaic',
		resize:function(event,ui){
			$('#mosaic_width').val(ui.size.width);
			$('#mosaic_height').val(ui.size.height);
			$('[id^=mosaic_open]').css({
				'margin-top': Number($('#mosaic_height').val()/2)
			});
		}
	});
	$('.mosaic').css({
		'background': '#33FF33',
		'width': Number($('#mosaic_width').val()),
		'height':Number($('#mosaic_height').val()),
		'text-align': 'center',
		'float' : 'left',
		'border' : '1px solid #990000',
		'margin-left': Number($('#mosaic_rbord').val()),
		'margin-right': Number($('#mosaic_rbord').val()),
		'margin-top': Number($('#mosaic_cbord').val()),
		'margin-bottom': Number($('#mosaic_cbord').val())
	});
	$('[id^=mosaic_open_]').each(function(){
		$(this).css({
			'margin-top': Number($('#mosaic_height').val()/2)
		});
		$(this).click(function(){
			browse_target	=	'#'+$(this).attr('id');
			browse();
			$('#window_browse').dialog('open');
		});
	});

	$('.button').hover(
		function() { $(this).addClass('ui-state-hover'); },
		function() { $(this).removeClass('ui-state-hover'); }
	);
}
