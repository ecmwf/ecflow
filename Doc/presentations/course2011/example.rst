.. include:: <s5defs.txt>

============================================
 ecFlow course - migration - September 2011
============================================

:Authors: John Hodkinson, Avi Bahra, Axel Bonet
:Date: September 26-28 2011

AIMS
========

.. class:: incremental

* step by step example to migrate a suite to ecFlow:

  * transcoding a definition file

  * preparing task wrappers

  * preparing task headers

SMS suite: Play
===============

.. class:: incremental

* a template suite to test jobs push on multiple platforms, 

  used as an example to start new projects: skull.

* the play-expand-translate-load method:

* **play** the suite definition file into sms::
  
  $ l="set SMS_PROG $((900000 +$(id -u))); login $HOST $USER 1"
  $ cdp -c "$l; play -r /skull skull.def"
  $# MSG:play:/skull replaced.

SMS suite: display with XCdp
=============================

.. container:: handout

  XCdp is the graphical application to display SMS server content.


.. container:: hidden slide-display
 
   .. container:: animation center

     .. image:: img11/skull_sms1.png     
       :scale: 90
       :class: hidden slide-display


     .. class:: incremental center hidden slide-display
        :hidden:

        .. image:: img11/skull_sms4.png       
          :scale: 90
        .. image:: img11/skull_sms5.png       
          :scale: 90 
        .. image:: img11/skull_sms6.png       
          :scale: 90 
        .. image:: img11/skull_sms7.png       
          :scale: 90 
        .. image:: img11/skull_sms8.png       
          :scale: 90 
        .. image:: img11/skull_sms9.png       
          :scale: 90 

     .. image:: img11/skull_sms10.png       
       :scale: 90 
       :class: incremental center hidden slide-diplay
   

SMS suite: CDP definition file
===============================

.. container::  left tiny slide-display

  .. raw :: html 
     :file: src/skull.def.html

SMS suite: stepping out
===========================

.. class:: incremental

* **expand** the suite:

  ``cdp -c "ode; status; get; show /skull > expanded.sms"``

* alternatively, it can be played and expanded **locally**:

  ``$ cdp -c "play -l skull.def; show /skull > expanded.sms"``

* apply **filter** (grep out or change variables)::

   $ filter=~map/bin/sms2ecf/sms2ecf-min.sed; name=expanded
   $ sed -f $filter $name.sms > $name.ecf

* **load** into ecFlow server:``$ ecflow_client --load expanded.ecf``

* or **replace**:``$ ecflow_client --replace=/skull expanded.ecf``

  $# Add Suite failed: A Suite of name 'skull' already exist

ecFlow: first step
===================

.. class:: incremental

* the server shall be capable to **find tasks wrappers** (ECF_HOME)

  * edit ECF_EXTN .sms 
  * copy, move, link the files ?

* the server shall be capable to **find header files** (ECF_INCLUDE)

  * %SMS to be replaced with relevant %ECF_ variable

* multiple situations:
  
  * same location for SMSFILES and ECF_FILES ?
  * same location for SMSINCLUDE and ECF_INCLUDE ?
  * different directories
  * links ? different path names, same location

* *Script-Edit-Preprocess-Submit* ?

ecFlow suite: display with ecflowview
======================================

.. container:: animation hidden slide-display center

  .. image:: img11/skull_ecf1.png
     :class: hidden slide-display
     :scale: 90 

  .. class:: incremental hidden slide-display center
 
    .. image:: img11/skull_ecf1.png
       :scale: 90 
    .. image:: img11/skull_ecf2.png
       :scale: 90 
    .. image:: img11/skull_ecf3.png
       :scale: 90 
    .. image:: img11/skull_ecf4.png
       :scale: 90 
    .. image:: img11/skull_ecf5.png
       :scale: 90 
    .. image:: img11/skull_ecf6.png
       :scale: 90 
    .. image:: img11/skull_ecf7.png
       :scale: 90 
    .. image:: img11/skull_ecf8.png
       :scale: 90 

ecFlow: second step
===================

.. class:: incremental

* one-off upgrade ?

* updating scripts to gain **compatibility** with both SMS and ecFlow ?

  * replace child commands calls with generic name, or use aliases

  * define a header file to enable the relevant child command to be called

From CDP expanded suite to ecFlow 
================================================

.. sidebar:: ecFlow: ready to load 
   :class: tiny slide-display

   .. raw :: html
     :file: src/expanded.ecf.html

.. class:: handout

  Expanded definition files can be rather large files ... 

  We expect to find the relevant matching ``ECF_`` variable matching the ``SMS`` system variable.

.. class:: hidden slide-display

  **CDP: ready to play**

  .. raw :: html
     :file: src/expanded.sms.html

Definition file: SMS, ecFlow, or both
======================================

.. _ including the two sets of system variables into the suite, 

.. _ it will be compatible for any system the suite is played/loaded into

.. class:: tiny
 
.. raw :: html
       :file: src/compatible.def.html

Including one additional header may be enough
=========================================================

.. class:: tiny

.. raw :: html
    :file: src/inc_ecf.h.html

Exotic cases, native python tasks
================================================

.. sidebar:: py_tail.h, task wrapper 
  :class: tiny 

  .. raw :: html
    :file: src/python_endt.h.html

  .. raw :: html
    :file: src/python.sms.html

.. class:: tiny

.. raw:: html
    :file: src/python_header.h.html

Exotic cases, native perl tasks
=============================================

.. sidebar:: perl_tail.h, task wrapper
  :class: tiny

  .. raw :: html
      :file: src/perl_endt.h.html

  .. raw :: html
      :file: src/perl.sms.html

.. class:: tiny
  :tiny:

.. raw:: html
  :file: src/perl_header.h.html

Text definition file (Ksh)
============================

.. class:: tiny slide-display

  .. raw :: html 
      :file: src/skull.sh.html

Python definition file
================================

.. class:: tiny slide-display

    .. raw :: html 
      :file: src/skull.py.html

Comparison: CDP vs text
========================

.. class:: tiny slide-display

.. sidebar:: Text suite

  .. raw:: html
     :file: src/ex.sh.html

.. class:: tiny slide-display

.. raw:: html
   :file: src/ex.cdp.html
 
Comparison: CDP vs Python
============================

.. class:: slide-display tiny

.. sidebar:: Python suite

    .. raw:: html
       :file: src/ex.py.html

.. class:: slide-display tiny

.. raw:: html
     :file: src/ex.cdp.html 

Going further
=================

.. class:: incremental

* in the interim, compatible definition files can be used

* most transition issues are in the suite designer hands

* jobs script can be aware what is the server to contact: SMS_PROG, ECF_PORT defined ?

* system side: 

  * child binaries can be replaced with homonym shell scripts: intercept-redirect

  * submission script shall be upgraded to be server independent
  
    to deal with specific queuing system directives, e.g. qsub.h:

    ``# QSUB -o %LOGDIR%%SMSNAME%.%SMSTRYNO%``

Questions ?
===========

* Online Tutorial: http://intra.ecmwf.int/metapps/manuals/ecflow

* Contacts: User Support, or John Hodkinson, Avi Bahra, Axel Bonet 
