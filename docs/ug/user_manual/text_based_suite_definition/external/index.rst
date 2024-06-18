.. _external_dependencies:

External Dependencies
/////////////////////

In addition to attributes that allow defining the dependencies mentioned in
:ref:`dependencies`, ecFlow is able to specify dependencies based on external
entities, and thus allow "local" nodes to be triggered by remote events:

   - notifications disseminated by an Aviso server
   - status changes on remote ecFlow

The following sections present in detail the different external dependencies
currently supported.

.. toctree::
   :maxdepth: 1

   aviso
   mirror
