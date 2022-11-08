# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#

import datetime
import os
import sys

# Adds path to the folder _ext, where extensions are stored
sys.path.insert(0, os.path.abspath("."))
sys.path.append(os.path.abspath("./_ext"))

build_path = "/Users/cgr/build/ecflow/debug/Pyext/python3"
sys.path.insert(0, build_path)

# -- Project information -----------------------------------------------------

project = "ecflow"
author = "ECMWF"


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["sphinx_rtd_theme", "sphinx.ext.viewcode", "sphinx.ext.autodoc"]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# The suffix of source filenames.
source_suffix = ".rst"

# The encoding of source files.
# source_encoding = 'utf-8-sig'

# The master toctree document.
master_doc = "index"

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

highlight_language = "none"


import sphinx.ext.autodoc


class TargetFile:
    LAST_NAME = "ZombieVec"

    def __init__(self):
        self.name = ""
        self.fp = None

    def write(self, source, line):
        _, _, s = source.rpartition("docstring of ecflow.")
        if "zombievec" in s.lower():
            print(s)
        s = s.split(".")
        if s:
            name = s[0]
            if name and name[0].isupper():
                self._open(name)
                if self.fp is not None:
                    self.fp.write(line + "\n")
                    if self.name == self.LAST_NAME:
                        self.fp.close()
                        self.fp = None

    def _open(self, name):
        if self.name == name and self.name != "":
            if self.fp is None:
                self.fp = open(f"rst/{name}.rst", "a")
        else:
            self.name = name
            if self.fp is not None:
                self.fp.close()
                self.fp = None
            if self.name != "" and self.name[0].isupper():
                self.fp = open(f"rst/{self.name}.rst", "w")
                title = f"ecflow.{name}"
                self.fp.write(f"{title}\n")
                self.fp.write("/" * len(title) + "\n\n")


targetFile = TargetFile()


def add_line(self, line, source, *lineno):
    """Append one line of generated reST to the output."""
    # _RST_FP.write(line + "\n")
    # print(f"source={source}")
    targetFile.write(source, line)
    self.directive.result.append(self.indent + line, source, *lineno)


sphinx.ext.autodoc.Documenter.add_line = add_line

#
