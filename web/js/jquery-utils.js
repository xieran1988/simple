
$.heredoc = function (f) {
		var s = new String(f);
		return s.substring(s.indexOf("/*") + 2, s.lastIndexOf("*/"));  
};

