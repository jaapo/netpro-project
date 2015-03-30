function createTocEntry(header, id) {
	var level = parseInt(header.tagName[1]);
	var link = document.createElement('a');
	link.href = '#' + id;
	header.id = id;
	link.innerHTML = header.innerHTML;
	
	var entry = document.createElement('li');
	entry.className = 'tocentry';
	entry.appendChild(link);
	return entry;
}

function addLevel(e) {
	var ol = document.createElement('ol');
	ol.className = 'tocentry';
	e.appendChild(ol);
	return ol;
}

function createToc() {
	var headers = [];
	for(var i=0,c=document.body.children;i<c.length;i++){
		if (c[i].tagName.match(/H[0-9]/))
			headers.push(c[i]);
	}

	var toc = addLevel(document.getElementById('toc'));
	var ol = toc;
	var stack = [];
	var lastlevel = 2;
	var n = Array(9);
	n.fill(0);
	for(var i=1;i<headers.length;i++) {
		var h = headers[i];
		var level = parseInt(h.tagName[1]);
		n[level]++;
		
		var ltext = n.filter((_,i)=>(i>1&&i<=level)).join('.')
		h.innerHTML = ltext + ' ' + h.innerHTML;
		if (level == lastlevel) {
		} else if (level > lastlevel) {
			stack.push(ol);
			ol = addLevel(ol);
		} else if (level < lastlevel) {
			for(var j=0;j<lastlevel-level;j++) {
				n[level+j+1] = 0;
				ol = stack.pop();
			}
		}
		ol.appendChild(createTocEntry(h, ltext.replace(/\./g,'-')));
		lastlevel = level;
	}
}
