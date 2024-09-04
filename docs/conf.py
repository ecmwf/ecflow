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

# build_path = "/Users/cgr/build/ecflow/debug/Pyext/python3"
# sys.path.insert(0, build_path)

# -- Project information -----------------------------------------------------

project = "ecflow"
author = "ECMWF"

year = datetime.datetime.now().year
years = "2009-%s" % (year,)
copyright = "%s, European Centre for Medium-Range Weather Forecasts (ECMWF)" % (years,)


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["sphinx_rtd_theme"]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "python_api_old",
    "build_python_api",
]

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

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

html_css_files = ["css/custom_style.css"]

html_logo = ""

highlight_language = "none"

numfig = True

rst_prolog = """
.. role:: mval
"""

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#


def get_ecflow_version():
    version = "5.13.4"
    ecflow_version = version.split(".")
    print("Extracted ecflow version '" + str(ecflow_version))
    return ecflow_version


# import sphinx.ext.autodoc

# _RST_FP = open("MY_PY.rst", "w")


# def add_line(self, line, source, *lineno):
#     """Append one line of generated reST to the output."""
#     _RST_FP.write(line + "\n")
#     # print(f"source={source}")
#     self.directive.result.append(self.indent + line, source, *lineno)


# sphinx.ext.autodoc.Documenter.add_line = add_line

# The full version, including alpha/beta/rc tags.
ecflow_version_list = get_ecflow_version()
assert len(ecflow_version_list) == 3, "Expected version to have release, major,minor"
version = ".".join(ecflow_version_list[:2])
release = ".".join(ecflow_version_list)
