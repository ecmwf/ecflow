var width = 1024,
    height = width,
    padding = 5, 
    duration = 1000,
    radius = Math.min(width, height) / 3;
var x = d3.scale.linear()
    .range([0, 2 * Math.PI]);
var y = d3.scale.sqrt()
    .range([0, radius]);
var color = d3.scale.category20c();
var svg = d3.select("body").append("svg")
    .attr("width", width)
    .attr("height", height)
    .append("g")
    .attr("transform", "translate(" + width / 2 + "," + (height / 2 + 10) + ")");
var partition = d3.layout.partition()
    .sort(null)
    .value(function(d) { return 5.8 - d.depth; });
//    .value(function(d) { return 1; });
var arc = d3.svg.arc()
    .startAngle(function(d) { return Math.max(0, Math.min(2 * Math.PI, x(d.x))); })
    .endAngle(function(d) { return Math.max(0, Math.min(2 * Math.PI, x(d.x + d.dx))); })
    .innerRadius(function(d) { return Math.max(0, y(d.y)); })
    .outerRadius(function(d) { return Math.max(0, y(d.y + d.dy)); });

var nodes;
// Keep track of the node that is currently being displayed as the root.
var node;
d3.json(jsondata, function(error, root) {
	node = root;
	nodes = partition.nodes(root); // json
	var path = svg.datum(root).selectAll("path")
	    .data(nodes // partition.nodes)
		  )
	    .enter().append("path")
	    .attr("d", arc)
	    .style("fill", function(d) { 
		    if (1) return colour(d);
		    return color((d.children ? d.name : d.size)); })
	    .on("click", click)
	    .each(stash);
	d3.selectAll("input").on("change", function change() {
		var value = this.value === "count"
		    ? function() { return 1; }
		: function(d) { return d.size; };
		path
		    .data(partition.value(value).nodes)
		    .transition()
		    .duration(1000)
		    .attrTween("d", arcTweenData);
	    });

// from sunburst
  var text = svg.selectAll("text").data(nodes);
  var textEnter = text.enter().append("text")
      .style("fill-opacity", 1)
      .style("fill", function(d) {
        return brightness(d3.rgb(colour(d))) < 125 ? "#eee" : "#000";
      })
      .attr("text-anchor", function(d) {
        return x(d.x + d.dx / 2) > Math.PI ? "end" : "start";
      })
      .attr("dy", ".2em")
      .attr("transform", function(d) {
	      var name = d.name || "";
        var multiline = (name).split(" ").length > 1,
            angle = x(d.x + d.dx / 2) * 180 / Math.PI - 90,
            rotate = angle + (multiline ? -.5 : 0);
        return "rotate(" + rotate + ")translate(" + (y(d.y) + padding) + ")rotate(" + (angle > 90 ? -180 : 0) + ")";
      })
      .on("mouseover", mouseover)
      .on("click", click);
  textEnter.append("tspan")
      .attr("x", 0)
      .text(function(d) { 
	      var name = d.name || "";
	      return d.depth ? name.split(" ")[0] : ""; });
  textEnter.append("tspan")
      .attr("x", 0)
      .attr("dy", "1em")
      .text(function(d) { 
	      var name = d.name || "";
	      return d.depth ? name.split(" ")[1] || "" : ""; });

// end

function click(d) {
    node = d;
    path.transition()
	.duration(1000)
	.attrTween("d", arcTweenZoom(d));

    // from sunburst zoom

    // Somewhat of a hack as we rely on arcTween updating the scales.
    text.style("visibility", function(e) {
          return isParentOf(d, e) ? null : d3.select(this).style("visibility");
        })
      .transition()
        .duration(duration)
        .attrTween("text-anchor", function(d) {
          return function() {
            return x(d.x + d.dx / 2) > Math.PI ? "end" : "start";
          };
        })
        .attrTween("transform", function(d) {
          var multiline = (d.name || "").split(" ").length > 1;
          return function() {
            var angle = x(d.x + d.dx / 2) * 180 / Math.PI - 90,
                rotate = angle + (multiline ? -.5 : 0);
            return "rotate(" + rotate + ")translate(" + (y(d.y) + padding) + ")rotate(" + (angle > 90 ? -180 : 0) + ")";
          };
        })
        .style("fill-opacity", function(e) { return isParentOf(d, e) ? 1 : 1e-6; })
        .each("end", function(e) {
          d3.select(this).style("visibility", isParentOf(d, e) ? null : "hidden");
        });
}
    });
d3.select(self.frameElement).style("height", height + "px");
// Setup for switching data: stash the old values for transition.
function stash(d) {
    d.x0 = d.x;
    d.dx0 = d.dx;
}
// When switching data: interpolate the arcs in data space.
function arcTweenData(a, i) {
    var oi = d3.interpolate({x: a.x0, dx: a.dx0}, a);
    function tween(t) {
	var b = oi(t);
	a.x0 = b.x;
	a.dx0 = b.dx;
	return arc(b);
    }
if (i == 0) {
// If we are on the first arc, adjust the x domain to match the root node
// at the current zoom level. (We only need to do this once.)
var xd = d3.interpolate(x.domain(), [node.x, node.x + node.dx]);
return function(t) {
    x.domain(xd(t));
    return tween(t);
};
} else {
    return tween;
}
}
// When zooming: interpolate the scales.
function arcTweenZoom(d) {
    var xd = d3.interpolate(x.domain(), [d.x, d.x + d.dx]),
	yd = d3.interpolate(y.domain(), [d.y, 1]),
	yr = d3.interpolate(y.range(), [d.y ? 20 : 0, radius]);
    return function(d, i) {
	return i
	    ? function(t) { return arc(d); }
	: function(t) { x.domain(xd(t)); y.domain(yd(t)).range(yr(t)); return arc(d); };
    };
}

// http://www.w3.org/WAI/ER/WD-AERT/#color-contrast
function brightness(rgb) {
  return rgb.r * .299 + rgb.g * .587 + rgb.b * .114;
}
function isParentOf(p, c) {
  if (p === c) return true;
  if (p.children) {
    return p.children.some(function(d) {
      return isParentOf(d, c);
    });
  }
  return false;
}

  function mouseover(d) {

      var percentage = 0; // (100 * d.value / totalSize).toPrecision(3);
  var percentageString = percentage + "%";
  if (percentage < 0.1) {
    percentageString = "< 0.1%";
  }

  d3.select("#percentage")
      .text(percentageString);

  d3.select("#explanation")
      .text(d.name)
      .style("visibility", "");

  var sequenceArray = getAncestors(d);
  // updateBreadcrumbs(sequenceArray);
  }