
.. index::
   single: migration-intro
   
.. _migration-intro:
   

Introduction
============
 
This document describes the main steps in moving from as SMS system to a suite
managed by an ecFlow server. Related documentation for each system can be found here:

* `SMS <http://www.ecmwf.int/publications/manuals/sms/>`_.

* `ecFlow <http://intra.ecmwf.int/metapps/manuals/ecflow/>`_.

Multiple scenarios are available:

* the best case is perhaps when starting a new project, with no prior
  historical constraint nor inherited scripts. A suite designer can
  choose their preferred language for the suite definition, either 
  python, or using another any language to produce a text definition file such as 
  simple ksh scripting, other scripting languages, an interpreted or a compiled language. 
  The best language is the one you are fluent with, pleased to work with, 
  alone and as a team. Its purpose is to produce an expanded definition file, 
  containing the description of the tasks to run, their time, date and trigger 
  dependencies, their specific configuration (variables). For SMS, the play environment 
  keywords available are (CDP>man -s play)::
  
    EOF          abort        action       autocancel   automigrate  autorestore  
    clock        complete     cron         date         day          defstatus    
    edit         endfamily    endsuite     endtask      event        extern       
    family       inlimit      label        late         limit        meter        
    owner        repeat       suite        task         text         time         
    today        trigger      

  For ecFlow this list is slightly smaller::

                                           autocancel  
    clock        complete     cron         date         day          defstatus    
    edit         endfamily    endsuite     endtask      event        extern       
    family       inlimit      label        late         limit        meter        
                 repeat       suite        task                      time         
    today        trigger      

  Next comes the task wrappers definition. Again, there is no
  right way to do this. It is simple to design a task
  whose language is pure python or pure perl. We tend to use use Ksh scripting for task
  templates for the following reasons:

    * trap ERROR 0: to prevent early exit from the script and call the ERROR if exited

    * set -e: to raise an error if a command exit status is not 0

    * set -u: to prevent undefined variable usage

    * set -x: to display each command before execution

    * PS4 variable: to allow time stamping and evaluate each lines runtime

    * trap: to redirect internal/external signal reception to an ERROR function

  Task headers can be used to make common what can be shared among multiple
  tasks (head.h, tail.h, trap.h, rcp.h, qsub.h).

* another migration strategy is a one step migration: this document can be used as a 
  check-list. However, it may not be exhaustive and may be enriched with your contribution, 
  reporting a case not mentioned.

* a frequent migration strategy may be to evolve to a situation where it is possible to
  run a suite under both SMS or ecFlow.

Milestone are:

* upgrading header files.

* modifying task wrapper files.

* writing a definition file that describes the expected suite.

* third party software may have migration concerns: 

  * the IFS model is known to call smsevent, smsmeter,

  * our archive system MARS transmits information to sms using smslabel, smsevent.

  * hopefully, these calls are through a system commands. However SMS extensions, linked with the
    sms library, are obsolete with ecFlow and cannot be simply migrated.

* when both SMS/ecFlow systems have to run in parellel, we have to
  decide what is part of the suite configuration (user side), and what
  is part of the system responsibility.

* on the system side: 

  * smssubmit, smskill, smsstatus: have to decide if the same script
    can be used, if their name has to be changed or if we take this opportunity
    to modify their behaviour.

  * child command calls (smsinit, smscomplete, smsabort ...) can be intercepted by
    changing the PATH variable in order to use the same shell scripts that can
    call an SMS child command if SMS_PROG is part of the environment or an 
    ecFlow child command when ECF_PORT is defined and not null.

  * When the link between a job and its parent server cannot be
    established, the file $HOME/.smshostfile is used by child
    commands to identify potential backup servers to contact (network
    zombie). It has to be decided if the ecFlow servers deployment needs to
    match exactly what was chosen for SMS.
