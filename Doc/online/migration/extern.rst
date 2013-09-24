.. index::
   single: extern
   
.. _extern:
   
SMS triggers are checked from the client side. It is simple to reach a
situation where triggers are not valid from the server side, and
trigger related task may remain queued (deadlock).

To prevent that to happen, ecFlow triggers check is more robust:

* triggers must be consistent from the client side.

* 'extern' shall only refer to an external suite's node

* server may be requested to confirm that all triggers are referring
  to an existing node, in already loaded suites.

As it is with the SMS, a ecFlow suite can be shared for team work. It is
suite design and management skill to decide how to split the suite for
a good collaboration within team members.

At ECMWF, operational suites are defined in files describing the full content.
* the definition file is split into different python modules for

    * observations retrieval
    * analysis and forecast
    * products generation and dissemination
    * plots generation

* then a python script is in charge to put all things together and to create the suite

* the user decides to load the full suite, only a subt-ree from the suite or to load it as an e-suite (test suite)

* a SCM is used to identify the modules edited by different users, merge their updates, resolve the conflicts or allow them to work in an branched version or the suite for the next release.</li></ul>

.. 
  loop path (/eda/main /o/main /o/lag /o/pop /mc/main /mofc /law/main ); do 
    echo $path; ls -RVvd $path/ | grep task | wc -l; 
  endloop
  /eda/main 1214
  /o/main   6045
  /o/lag    1645
  /o/pop     308
  /mc/main  5073
  /mofc     19453
  /law/main   779

  set suites "limits admin sync eda o mc ocean4 law lbc mofc mars efas pp1 pp2 pp3"; 
  echo "" > tmp.tmp
  loop path ($suites) do ls -RVvd /$path | grep task >> tmp.tmp ; endloop
  cat tmp.tmp | awk 'BEGIN{sum=0}{sum++}END{print sum}' # 43349

The following example shows that with ecflow

* a master definition file may provide the suite structure (multi1.def), with the right setting of inlimits, limits, triggers,

* a developer in charge of adding test tasks to the suite may update it with a task test2, below an agreed family (/multi/main/00/test),

* another tester may add some other tests as a different suite, triggered from 'multi' suite.

.. image:: ecflowviewmap.png

client commands to load and replace
===================================

.. literalinclude:: load_and_replace.sh
  :language: bash

======================================
multi1.def: suite structure definition
======================================
.. literalinclude:: multi1.def
   :language: bash
   :linenos:


=========================================================
multi2.def: task and family addition for intra-suite test
=========================================================
.. literalinclude:: multi2.def
   :language: bash
   :linenos:


======================================================
multi3.def: standalone test triggered by 'multi' suite
======================================================
.. literalinclude:: multi3.def
  :language: bash
  :linenos:
