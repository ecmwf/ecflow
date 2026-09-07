"""The ``.. implementation::`` admonition.

Usage without an ``.. extension::`` marker renders a plain, always-expanded
admonition (unchanged from the original behaviour)::

    .. implementation::

        Some implementation detail, always shown.

Usage with an ``.. extension::`` marker on its own line splits the content in
two, and renders as a collapsible HTML ``<details>`` block: the part before
the marker is always visible (used as the clickable summary/label), the part
after the marker is hidden until the block is expanded::

    .. implementation::

        This part is always visible.

        .. extension::

        This part is only shown once the block is expanded.

The marker line itself is not rendered; it is only recognised by this
directive and is not a real, separately registered directive.
"""

import re

from docutils import nodes
from docutils.parsers.rst.directives.admonitions import BaseAdmonition

_EXTENSION_MARKER_RE = re.compile(r'^\.\.\s*extension::\s*$')


class implementation_node(nodes.Admonition, nodes.Element):
    pass


class implementation_summary_node(nodes.Admonition, nodes.Element):
    pass


class implementation_extension_node(nodes.General, nodes.Element):
    pass


def _extension_marker_index(content):
    for index, line in enumerate(content):
        if _EXTENSION_MARKER_RE.match(line.strip()):
            return index
    return None


class Implementation(BaseAdmonition):
    node_class = implementation_node
    required_arguments = 0

    def run(self):
        self.assert_has_content()

        options = dict(self.options)
        options.setdefault('classes', []).append('implementation')

        node = self.node_class(**options)
        self.add_name(node)
        node.source, node.line = self.state_machine.get_source_and_line(self.lineno)

        marker = _extension_marker_index(self.content)
        node['collapsible'] = marker is not None

        title_text = 'Implementation Detail'
        textnodes, messages = self.state.inline_text(title_text, self.lineno)
        title = nodes.title(title_text, '', *textnodes)
        title.source, title.line = self.state_machine.get_source_and_line(self.lineno)

        if marker is None:
            # No marker: render exactly as a plain admonition (no collapsing).
            node += title
            node += messages
            self.state.nested_parse(self.content, self.content_offset, node)
            return [node]

        summary_content = self.content[:marker]
        extension_content = self.content[marker + 1:]
        extension_offset = self.content_offset + marker + 1

        summary = implementation_summary_node()
        summary += title
        summary += messages
        self.state.nested_parse(summary_content, self.content_offset, summary)
        node += summary

        extension = implementation_extension_node(classes=['implementation-extension'])
        self.state.nested_parse(extension_content, extension_offset, extension)
        node += extension

        return [node]


def visit_implementation_html(self, node):
    tagname = 'details' if node.get('collapsible') else 'div'
    node['_html_tagname'] = tagname
    self.body.append(self.starttag(node, tagname, CLASS='admonition'))


def depart_implementation_html(self, node):
    self.body.append('</%s>\n' % node['_html_tagname'])


def visit_implementation_summary_html(self, node):
    self.body.append(self.starttag(node, 'summary', CLASS='admonition-summary'))


def depart_implementation_summary_html(self, node):
    self.body.append('</summary>\n')


def visit_implementation_extension_html(self, node):
    self.body.append(self.starttag(node, 'div', CLASS='admonition-body'))


def depart_implementation_extension_html(self, node):
    self.body.append('</div>\n')


def setup(app):
    app.add_directive('implementation', Implementation)
    app.add_node(
        implementation_node,
        html=(visit_implementation_html, depart_implementation_html),
    )
    app.add_node(
        implementation_summary_node,
        html=(visit_implementation_summary_html, depart_implementation_summary_html),
    )
    app.add_node(
        implementation_extension_node,
        html=(visit_implementation_extension_html, depart_implementation_extension_html),
    )
    return {'parallel_read_safe': True, 'parallel_write_safe': True}
