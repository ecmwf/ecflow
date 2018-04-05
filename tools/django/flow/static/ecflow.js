var statuses= [ "queued", "submitted", "aborted", "active", "complete",
		"suspended", "unknown", "shutdown", "halted", "completed",];

var status_colours = [ "#add9e7", "#04e1d1", "#ff0000", "#00ff00", "yellow",
		       "#ffa600", "#bfbfbf", "#ffc1cc", "#ef83ef", "yellow", ];

function colour(d) {
  if (d._status) {
    for (num in statuses) { 
      if (statuses[num] == d._status) { 
        return status_colours[num]; 
      }
    }
    return status_colours[0]; 
  }
  if (d.children) {
    // There is a maximum of two children!
    var colours = d.children.map(colour),
        a = d3.hsl(colours[0]),
        b = d3.hsl(colours[1]);
    // L*a*b* might be better here...
    return d3.hsl((a.h + b.h) / 2, a.s * 1.2, a.l / 1.2);
  }
  return d.colour || "#fff";
}

/* thanks
http://stackoverflow.com/questions/4909167/how-to-add-a-custom-right-click-menu-to-a-webpage 
*/

var mouseX = 0;
var mouseY = 0;
var mousepos = function mousePos (e) {
    mouseX = e.clientX + document.body.scrollLeft;
    mouseY = e.clientY + document.body.scrollTop;    
    // document.show.mouseXField.value = mouseX;
    // document.show.mouseYField.value = mouseY;
    return true;
}

window.onload = function () {
     document.onmousemove = mousepos; 
     // document.onmousedown = function (e) {mouseClicked();};
 };
