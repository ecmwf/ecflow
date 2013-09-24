.. index::
   single: example
   
.. _example:
   
Example
--------------------------

A suite template is provided to demonstrate basic SMS features which
can be used as a starting point for suite design. 

* Download the tar-file :download:`skull suite <src/skull.tgz>`

* untar it::

  > tar -xzvf skull.tgz

* eventually, start a SMS server::

  > sms_start

* convention at ECMWF is to attribute a program number
  according to the formula::

  > prog_num=$((900000 + $(id -u))); echo $prog_num

* open xcdp, File->Login to: "localhost <prog_num>"
  
* play the skull suite into sms::

  > export SMS_PROG=$((900000+$(id -u))) 
  > LOGGER="setenv -i $SMS_PROG; login localhost $USER 1;"
  > /usr/local/apps/sms/bin/cdp -c "$LOGGER play -r /skull skull.def"

* skull suite families are

  * make: to open a xterm window from jobs run on local workstation, ecgate, HPC, linux cluter
  * consumer: to demonstrate the producer-consumer pattern
  * dinner: a resource sharing example through the dinning philosophers example
  * shop: to practice dynamic tasks generation
  * perl, Python: as task template examples that do not use ksh

  .. image:: img/skull.png

* conversion to a text based definition file written by ksh (skull.sh) is rather straightfoward:: 

  > make sh

  libgen.sh is generated to provide cdp:play commands in ksh environment::

  > bin/genlibdef.sh sh libgen.sh

  .. literalinclude:: src/libgen.sh

  At this stage, libgen.sh can be manually updated, to replace EOF
  with exit 0, inhibit the action, automigrate, autorestore, owner commands
  (which are unavailable in ecFlow). Change the limit, date, time naming, as it
  interferes with ksh intrinsics.

  It is possible to transcode most CDP commands into ksh, defining
  aliases that will print the cdp:play command and redirect their output in a
  dedicated file descriptor::

  > bin/def2ecf.sh sh gen   

  sh/skull.sh contains the ksh command to create the file descriptor 3::

  > exec 3> expanded.tmp

  SMS **.def** definition files may be updated to facilitate such
  transcoding (#CONV tags into \*.def files):

    - commands can be split into multiline commands to simplify
      mathematical expressions
      incompatible commands (owner, autorestore, automigrate, action)
      can be removed or set as a comment

    - multiline triggers changed into one line commands

    - the heavy use of aliases can lead to confused transcoding

    - CDP is known to be "tolerant": for example, an unboumded endin
      command is transparent to CDP, but it would mislead the
      transcoder.

    - for CDP, commands accept minimum significant string for options;
      they should be replace with their unambiguous version::

      > # CDP: repeat int iter 1 $life_expectency
      > repeat integer iter 1 $life_expectency

    - through its simplicity, the CDP language was a way to keep away from
      complex code. Once left behind, it is each suite designers
      responsability to stick to simple manageable, portable and
      maintainable code.

    - beware homonyms: such as date, time, limit

  This demonstrates the feasibility, for a simple suite. Past
  experience shows it is not as easy when considering a more complex suite design.

  The suite definition is then expanded, as a file sh/expanded.tmp::

  > bin/def2ecf.sh sh expand

  or::
  
  > cd sh; ksh skull.sh

* the suite can be played locally and expanded for comparison::

  > /usr/local/apps/sms/bin/cdp -c "play -l skull.def; show > expand.cdp"
  > kompare expand.cdp sh/expand.tmp
  > kompare skull.def  sh/skull.sh

* it can then be loaded into SMS:: 

  > /usr/local/apps/sms/bin/cdp -c "LOGGER play -r /skull sh/expanded.tmp"

* apply the command begin and resume with XCdp or CDP 

* We can then start ecFlow server, if not already done::

  > use ecflow
  > export ECF_PORT=$((20000+$(id -u))) 
  > mkdir ecf && cd ecf
  > ecflow_server
  
* expanded suite can be loaded into ecFlow::

  > cd ..
  > ecflow_client --load sh/expanded.tmp

* once loaded, it can be replaced with::

  > ecflow_client --replace /skull sh/expanded.tmp

* commands begin and resume can be applied on the console or with ecflowview::  

  > ecflow_client --begin /skull
  > ecflow_client --resume /skull
  > ecflow_client --resume /skull/skull /skull/perl /skull/python

* in this example, it is enough to **link task wrappers** from the
  original name *.sms* to the expected name *.ecf*. 

  Alternatively, ECF_EXTN variable may be defined, as **.sms** on the
  top node.

* **smsfiles/passby.sms** task wrapper has been updated to be compatible in both modes.

* **smsfiles/perl.sms** and **smsfiles/python.sms** call an intermediate
  function to access the meter child command: call_meter

* in the directory include: trap.h, perl_header.h, python_header.h,
  endt.h, were modified to enable ecFlow mode.

* **include/inc_ecf.h** was added and included from trap.h to add new expected variables, when ECF_PORT variable is not 0.

* it may be an interesting exercise to transform into Python code the file skull.def and the related families defined in 
  their dedicated definition files. The script ./bin/def2ecf.pl can be used to produce a transcoded file. Careful attention 
  is required to fix the transcoder mistakes and obtain a script that can be parsed with Python.  Then a script that 
  can be loaded as a proper suite into SMS, or ecFlow.
  
  * pydef_example/inc_hostc.py is a Python library used to maintain a definition file syntax close to CDP's. 
  * the transcoded file has to be checked for all dollar variables
  * all CDP functions parameters are transcoded incorrectly: $1, $2 ... They can be transformed into named arguments, with a 
    default value when needed.
  * the definition tree is created adding the lines
  
::
    s = suite(SELECTION)
    DEFS = Defs()
    DEFS.add_suite(s)

  * choice must be made whether to use "import library" or "from library import \*". 
  * to maintain code readability, new functions can be created (create_suite)
