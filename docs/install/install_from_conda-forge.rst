.. _conda-forge:

Install from conda-forge
************************

The latest ecflow is available for Linux and macOS from conda-forge: https://anaconda.org/conda-forge/ecflow

It is simple to setup a local conda environment for ecflow python API:

.. code-block:: bash
  :linenos:

  conda create -n ecflow_env python=3.11 -y
  conda activate ecflow_env
  conda install -c conda-forge ecflow
  python3 -c "import ecflow; help(ecflow)"
