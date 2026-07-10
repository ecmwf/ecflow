
from docutils import nodes
from docutils.parsers.rst.directives.admonitions import BaseAdmonition

class Implementation(BaseAdmonition):
    node_class = nodes.admonition
    required_arguments = 0

    def run(self):
        self.arguments = ['Implementation Detail']
        self.options.setdefault('classes', []).append('implementation')
        return super().run()

def setup(app):
    app.add_directive('implementation', Implementation)
    return {'parallel_read_safe': True, 'parallel_write_safe': True}
