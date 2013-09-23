.. index::
   single: header_files
   
.. _header_files:
   
Header files migration
----------------------

In our main operational suite, 9 files, out of 115, need modification, and
118 out of 932, amongst all suites.

Many header files are a snapshot of old versions
of the main header files, used to setup stand alone projects. 
This migration can be an opportunity to converge toward a common configuration
file set.

We also found that similar code was replicated amongst multiple files. This 
leads to "code cleaning" and factorisation.

* We have the main configuration variables defined in our trap.h header file::

   SMSNODE=%SMSNODE%
   SMS_PROG=%SMS_PROG%
   SMSNAME=%SMSNAME%
   SMSPASS=%SMSPASS%
   SMSTRYNO=%SMSTRYNO%
   SMSRID=$(echo $QSUB_REQID | cut -f1 -d.)
   SMSHOSTFILE=$HOME/.smshostfile
   SMSJOBOUT=%SMSJOBOUT%

* synchronisation commands existed in the rcp.h header::

   rcp %LOGDIR%%SMSNAME%.%SMSTRYNO%  emos@%SMSNODE%:%SMSOUT%%SMSNAME%.%SMSTRYNO%

* queuing system directives had embedded SMS vairiable (qsub.h)::

  # QSUB -o %LOGDIR%%SMSNAME%.%SMSTRYNO%

* SMS child commands were called in few header files: trap.h
  (smsabort, smsinit) and endt.h (smscomplete)

* All SMS variable must be used with a default value (%SMSX:0%) to
  prevent ecFlow to complain when generating job file.

* we can then proceed to variables translation or include specific ecFlow header
  files.

.. literalinclude:: src/ecf.h
