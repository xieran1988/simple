

$.fn.btnbar = function() {

	var tmpl = $.heredoc(function () {
		/*
		<div class=btnbar>
		<div class=btn-toolbar>
		<div class=btn-group>
			<a class=btn op=cut>
				<img height=10px src=/img/cut.png>
				分割 </a>
			<a class=btn op=merge>
				<img src=/img/merge.png>
				合并 </a>
			<a class=btn op=del>
				<img src=/img/cancel.png>
				删除 </a>
			<a class=btn op=add>
				<img src=/img/add.png>
				添加 </a>
		</div>
		</div>

		<div class=btn-toolbar>
		<div class=btn-group>
			<a class=btn op=undo>
				<img src=/img/undo.png>
				撤销</a>
			<a class=btn op=redo>
				<img src=/img/redo.png>
				重做</a>
		</div>
		</div>

		<div class=btn-toolbar>
		<div class=btn-group>
			<a class=btn op=text>
				<img src=/img/font.png>
				文字</a>
			<a class=btn op=music>
				<img src=/img/music.png>
				音乐</a>
			<a class=btn op=effect>
				<img src=/img/eff.png>
				过场效果</a>
		</div>
		</div>
		</div>
		*/
	});


	var args = Array.apply(null, arguments);
	var bar = $(this);

	if (!args.length) {
		bar.html(tmpl);
		return ;
	}
	if (typeof args[1] == 'function') {
		$('[op="'+ args[0] +'"]', bar).click(args[1]);
		console.log('bind');
	}
};

