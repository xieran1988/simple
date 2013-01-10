

$(document).ready(function() {

	var dragops = {};

	dragops.start = function(bar, col) {
		col.addClass('holder');
		var arr = [];
		bar.find('.col').each(function (_i, v) {
			v = $(v);
			if (v.hasClass('ui-draggable-dragging'))
				return ;
			if (v.is(col))
				return ;
			arr.push({
				m:v.offset().left + v.width()/2, 
				v:v,
			});
		});
		this.arr = arr;
		this.start_l = col.offset().left;
	};

	dragops.drag = function (col, ui) {
		var l = ui.offset.left;
		var r = ui.offset.left + col.width();
		if (l > this.start_l) {
			for (var i = this.arr.length - 1; i >= 0; i--) {
				var v = this.arr[i];
				if (r > v.m) {
					col.insertAfter(v.v);
					return ;
				}
			}
		} else {
			for (var i = 0; i < this.arr.length; i++) {
				var v = this.arr[i];
				if (l < v.m) {
					col.insertBefore(v.v);
					return ;
				}
			}
		}
	};

	dragops.stop = function (d) {
		d.removeClass('holder');
	};

	dragops.bind = function (bar, col) {
		col.draggable({
			revert: 'invalid', helper: 'clone', opacity: 0.5,
			revertDuration: 0, distance: 40,
			start: function(event, ui) { dragops.start(bar, col); },
			drag: function(event, ui) { dragops.drag(col, ui); },
			stop: function(event, ui) { dragops.stop(col); },
		});
	};

	var barops = {};

	barops.draw_cursor = function (cursor) {
		var cw = cursor.width();
		var ch = cursor.height();
		var c = cursor[0].getContext("2d");
		var i;
		var g;

		c.shadowOffsetX = 0;
		c.shadowOffsetY = 0;
		c.shadowBlur = 3;
		c.shadowColor = '#F31B2A';
		c.fillStyle = '#F31B2A';
		c.fillRect(cw/2-1, 0, 2, ch);
		this.hello = 'hi';
	};

	barops.stat = function (bar, cursor) {
		var s = {};
		var cols = [];
		var multisel = [];
		var cursels = [];

		$('.col', bar).each(function (_i, v) {
			v = $(v);
			if (v.hasClass('ui-draggable-dragging'))
				return ;
			var issel = v.hasClass('col-sel');
			cols.push({
				l: v.offset().left,
				r: v.offset().left + v.width(),
				v: v,
				per: colops.getper(v),
				issel: issel,
			});
			if (!issel) {
				if (cursels.length > 0) {
					multisel.push(cursels);
					cursels = [];
				}
			} else {
				cursels.push(v);
			}
		});
		
		if (cursels.length > 0)
			multisel.push(cursels);
		s.multisel = multisel;

		s.cols = cols;

		var per = 0;
		var x = cursor.offset().left;
		for (var i = 0; i < cols.length; i++) {
			var c = cols[i];
			if (x >= c.l && x <= c.r) {
				s.cursor_at = c.v;
				s.cursor_off = c.per*(x-c.l)/(c.r-c.l);
			}
			per += c.per;
		}
		s.per = per;

		return s;
	};

	barops.per2w = function (bar, per) {
		var w = bar.width() - 20;
		return w*per;
	};

	barops.newcol = function (bar, per) {
		var w = barops.per2w(bar, per);
		var r = $('<div class=col>').width(w).attr('per', per);
		colops.bind(bar, r);
		return r;
	}

	barops.append = function (bar, per) {
		var r = barops.newcol(bar, per);
		bar.append(r);
		return r;
	};

	barops.bind = function (bar, cursor) {
		bar.click(function (e) {
			barops.click(bar, cursor, e);
		});
	};

	barops.click = function (bar, cursor, e) {
		var x = e.clientX;
		cursor.css('left', x);
		var s = barops.stat(bar, cursor);
		if (!s.cursor_at) {
			$('.col-sel', bar).removeClass('col-sel');
		}
	};

	barops.cut = function (bar, cursor) {
		var s = barops.stat(bar, cursor);
		var col = s.cursor_at;
		if (!col)
			return ;
		var per = colops.getper(col);
		var newper = per - s.cursor_off;
		if (newper*bar.width() < 10) 
			return ;
		var r = barops.newcol(bar, newper);
		colops.setper(bar, col, s.cursor_off);
		r.insertAfter(col);
	};

	barops.del = function (bar, cursor) {
		var s = barops.stat(bar, cursor);
		for (var i in s.multisel) {
			var sel = s.multisel[i];
			for (var j in  sel) {
				sel[j].remove();
			}
		}
	};

	barops.glue = function (bar, cursor) {
		var s = barops.stat(bar, cursor);
		for (var i in s.multisel) {
			var sel = s.multisel[i];
			if (sel.length == 1)
				continue;
			var per = 0;
			for (var j in sel) {
				var col = sel[j];
				per += colops.getper(col);
			}
			for (var j = 1; j < sel.length; j++) 
				sel[j].remove();
			colops.setper(bar, sel[0], per);
			console.log(per);
		}
	};

	var colops = {};

	colops.getper = function (col) {
		return parseFloat(col.attr('per'));
	};

	colops.setper = function (bar, col, per) {
		var w = barops.per2w(bar, per);
		col.attr('per', per);
		col.width(w);
	};

	colops.click = function (bar, col, e) {
		if (e.shiftKey) {
			col.toggleClass('col-sel');
		} else {
			bar.find('.col-sel').removeClass('col-sel');
			col.addClass('col-sel');
		}
	};

	colops.bind = function (bar, col) {
		dragops.bind(bar, col);
		col.click(function (e) {
			colops.click(bar, col, e);
		});
	};

	$.fn.bar = function () {
		var args = Array.apply(null, arguments);
		var bar = $(this);
		var cursor = $('.cursor', bar);

		if (args.length == 0) {
			bar.css('width', 800);
			barops.draw_cursor(cursor);
			barops.bind(bar, cursor);
			return ;
		} 
		if (args[0] == 'width') {
			bar.css('width', args[1]);
		}
		if (args[0] == 'add') {
			var per = args[1];
			var col = barops.append(bar, per);
		}
		if (args[0] == 'cursor') {
			var per = args[1];
			var w = barops.per2w(bar, per);
			cursor.css('left', w);
		}
		if (args[0] == 'cut') {
			barops.cut(bar, cursor);
		}
		if (args[0] == 'glue') {
			barops.glue(bar, cursor);
		}
		if (args[0] == 'del') {
			barops.del(bar, cursor);
		}
		if (args[0] == 'new') {
			barops.append(bar, 0.3);
		}
	};

	$.btnclick = function (s) {
		if (s == 'glue') {
			$('.bar').bar('glue');
		}
		if (s == 'cut') {
			$('.bar').bar('cut');
		}
		if (s == 'del') {
			$('.bar').bar('del');
		}
		if (s == 'new') {
			$('.bar').bar('new');
		}
	}

	$('.bar').bar();
	$('.bar').bar('add', 0.1);
	$('.bar').bar('add', 0.1);
	$('.bar').bar('add', 0.2);
	$('.bar').bar('cursor', 0.4);

});

