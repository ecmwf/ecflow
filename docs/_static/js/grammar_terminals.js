// Colourise terminal (quoted) symbols in grammar ``productionlist`` blocks.
//
// Sphinx renders a ``.. productionlist::`` as a bare <pre> in which the
// non-terminals are <strong>/<a> elements but the terminal literals (e.g.
// ":", "(", "and") are plain text with no markup to style. This script wraps
// each double-quoted literal in a <span class="grammar-terminal"> so it can be
// coloured via CSS, making terminals stand out from non-terminals.
(function () {
    function colouriseTerminals(root) {
        var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null);
        var textNodes = [];
        var node;
        while ((node = walker.nextNode())) {
            textNodes.push(node);
        }
        textNodes.forEach(function (textNode) {
            var text = textNode.nodeValue;
            if (text.indexOf('"') === -1) {
                return;
            }
            var frag = document.createDocumentFragment();
            var re = /"[^"]*"/g;
            var last = 0;
            var match;
            while ((match = re.exec(text)) !== null) {
                if (match.index > last) {
                    frag.appendChild(document.createTextNode(text.slice(last, match.index)));
                }
                var span = document.createElement('span');
                span.className = 'grammar-terminal';
                span.textContent = match[0];
                frag.appendChild(span);
                last = re.lastIndex;
            }
            if (last < text.length) {
                frag.appendChild(document.createTextNode(text.slice(last)));
            }
            textNode.parentNode.replaceChild(frag, textNode);
        });
    }

    function run() {
        var pres = document.querySelectorAll('.rst-content pre');
        Array.prototype.forEach.call(pres, function (pre) {
            // Only touch productionlist blocks (they contain grammar-token anchors).
            if (pre.querySelector('strong[id^="grammar-token"]')) {
                // Tag the block so the scoped CSS (see custom_style.css) applies to
                // grammar blocks only, not to every <pre> on the page.
                pre.classList.add('grammar-pre');
                colouriseTerminals(pre);
            }
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', run);
    } else {
        run();
    }
})();
