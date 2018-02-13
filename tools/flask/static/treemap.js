var margin = {top: 40, right: 10, bottom: 10, left: 10},
    width = 960 - margin.left - margin.right,
    height = 500 - margin.top - margin.bottom;

var color = d3.scale.category20c();

var treemap = d3.layout.treemap()
    .size([width, height])
    .sticky(true)
    .value(function(d) { return d.size; });

var div = d3.select("body").append("div")
    .on("contextmenu", function() { return false; })
    .style("position", "relative")
    .style("width", (width + margin.left + margin.right) + "px")
    .style("height", (height + margin.top + margin.bottom) + "px")
    .style("left", margin.left + "px")
    .style("top", margin.top + "px");

var root;
// http://bl.ocks.org/tgk/6044254
var mousemove = function(d) {
  var xPosition = d3.event.pageX + 2;
  var yPosition = d3.event.pageY + 2;

  d3.select("#tooltip")
    .style("left", xPosition + "px")
    .style("top", yPosition + "px");
  d3.select("#tooltip #fullname")
    .text(d["name"]);
  d3.select("#tooltip").classed("hidden", false);
};

var mouseout = function() {
  d3.select("#tooltip").classed("hidden", true);
};

var mouseover = function(d) {
  var tok = d;
  var fullname = [tok.name];
  while (typeof tok.parent === 'object') {
    tok = tok.parent;
    fullname.unshift(tok.name);
  }
  fullname = fullname.join('/');
  $('#tooltip').text(fullname);
}

d3.json(jsondata, function(error, data) {
	root = data;
  var node = div.datum(root).selectAll(".node")
      .data(treemap.nodes)
    .enter().append("div")
      .attr("class", "node")
      .on("contextmenu", function() { return false; })
      .call(position)
      .style("background", function(d) { 
	      if (d._status) return colour(d);
	      return d.children ? color(d.size) : color(Math.random()); });

    node.on('mouseover', mouseover)
      .on("mousemove", mousemove)
      .on("mouseout", mouseout);

    node.text(function(d) { return "\n" + d.children ? null : (d.name + "\n");})
	.append("a")
	.attr("href", function(d) { return d.url; })
	.html(function(d) { return d.children ? null : (d.name + "\n") ; });

    node.each(function(d) {
	  if (! d.meta) return ""; // /static/q.png";
	node.append("img").attr("src", function(d) {
	    if (d.meta.search("ing_ok")) 
	      return '/static/ok.png';
	  else if (d.meta.search("ing_nok"))
	      return '/static/nok.png';
	return "/static/q.png";
    })});

  d3.selectAll("input").on("change", function change() {
    var value = this.value === "count"
        ? function()  { return 1; }
        : function(d) { return d.size; };

    node
        .data(treemap.value(value).nodes)
      .transition()
        .duration(1500)
        .call(position);
  });
});

function position() {
  this.style("left", function(d) { return d.x + "px"; })
      .style("top",  function(d) { return d.y + "px"; })
      .style("width",  function(d) { return Math.max(0, d.dx - 1) + "px"; })
      .style("height", function(d) { return Math.max(0, d.dy - 1) + "px"; });
}

// http://bl.ocks.org/tgk/6044254
/*
Group,Group description,Type,Type description,Percentage
A,The Rich,A1,Privileged,3
A,The Rich,A2,Entrepaneurs,7
A,The Rich,A3,Empty Nesters,10
B,Upper Class Families,B4,Well-established Volvo,23
B,Upper Class Families,B5,Optimist,7
C,Suburban,C6,Small Business,5
C,Suburban,C7,Never Left Home,15
E,Students,D8,Students,12
F,Troubled,E9,Borderline Beneficiaries,8
F,Welfare,F10,Families on Benefits,10

 */

// http://stackoverflow.com/questions/4909167/how-to-add-a-custom-right-click-menu-to-a-webpage
/*
if (document.addEventListener) {
        document.addEventListener('contextmenu', function(e) {
            alert("You've tried to open context menu"); //here you draw your own menu
            e.preventDefault();
        }, false);
    } else {
        document.attachEvent('oncontextmenu', function() {
            alert("You've tried to open context menu");
            window.event.returnValue = false;
        });
    }
*/
