function createTocEntry(heading, id) {
	var level = parseInt(heading.tagName[1]);
	var link = document.createElement('a');
	link.href = '#' + id;
	heading.id = id;
	link.innerHTML = heading.innerHTML;
	
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
	//get all headings as ordered in html
	var headings = [];
	for(var i=0,c=document.body.children;i<c.length;i++){
		if (c[i].tagName.match(/H[1-6]/))
			headings.push(c[i]);
	}

	//top list element
	var toc = addLevel(document.getElementById('toc'));
	var ol = toc;
	var stack = []; //store ol-elements
	var lastlevel = 2; //start from h2
	var n = Array(9); //level depths
	n.fill(0);

	for(var i=1;i<headings.length;i++) {
		var h = headings[i];
		var level = parseInt(h.tagName[1]);
		n[level]++;
	
		//create 2.4.1 style text for the heading, add to heading content
		var ltext = n.filter((_,i)=>(i>1&&i<=level)).join('.')
		h.innerHTML = ltext + ' ' + h.innerHTML;

		if (level == lastlevel) {
			//no need for special things
		} else if (level > lastlevel) {
			//store higher level (supports only 1 level jumps)
			stack.push(ol);
			ol = addLevel(ol);
		} else if (level < lastlevel) {
			//reset counters for each popped level
			for(var j=0;j<lastlevel-level;j++) {
				n[level+j+1] = 0;
				ol = stack.pop();
			}
		}
		ol.appendChild(createTocEntry(h, ltext.replace(/\./g,'-')));
		lastlevel = level;
	}
}
