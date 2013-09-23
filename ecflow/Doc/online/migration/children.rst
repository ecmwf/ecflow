.. index::
   single: child_commands
   
.. _child_commands:

Child commands comparison 
--------------------------

SMS child commands are not binary compatible with ecFlow. The
following table relates a sms command, provided with SMS package, and
the matching ecflow client call.

Some sms child commands (meter, event) have a query option. ecFlow
relies on the job using 'ecflow_client --get <path>' to obtain the
same information:

============================ ============================== ======================================
SMS commands                 ecflow equivalent              comment
============================ ============================== ======================================
sms                          ecflow_server                  sms -b: detach the server from session
                                                            nohup ecflow_server > out 2>&1 &:
---------------------------- ------------------------------ --------------------------------------
smsabort                     ecflow_client --abort <reason> 
---------------------------- ------------------------------ --------------------------------------
smsalive                     <none> 	                    task heart-bit, 
                                                            not used at the centre
---------------------------- ------------------------------ --------------------------------------
smscompletre                 ecflow_client --complete
---------------------------- ------------------------------ --------------------------------------
smsdate	                     <none> 	                    not a child command - 
                                                            date manipulation utility, 
                                                            not part of ecflow
---------------------------- ------------------------------ --------------------------------------
smsdie	                     <none>	                    clean RPC port 
			                                    not needed with ecflow
---------------------------- ------------------------------ --------------------------------------
smsevent <name>	             ecflow_client --event <name>	 
---------------------------- ------------------------------ --------------------------------------
smsinit $PID                 ecflow_client --init <rid>	    restricted to integer for SMS 
                                                            (use smsrid for string)
---------------------------- ------------------------------ --------------------------------------
smslabel <name> <string>     ecflow_client --label <n> <s>	 
---------------------------- ------------------------------ --------------------------------------
smsmeter <name> <step>	     ecflow_client --meter <n> <s>	 
---------------------------- ------------------------------ --------------------------------------
smsmsg <string tokens>	     ecflow_client --msg <string>	 
---------------------------- ------------------------------ --------------------------------------
smsping	                     ecflow_client --ping	 
---------------------------- ------------------------------ --------------------------------------
smsrand	                     <none>	                    not a child command, used to 
                                                            introduce random task runtime, 
                                                            in test mode, for sms
---------------------------- ------------------------------ --------------------------------------
smsrid <string>	             <none>	                    ecflow_client --init 
                                                            is used with string job id
---------------------------- ------------------------------ --------------------------------------
smsstatus <node path>	     <none>	                    ecflow_client --get <path> 
                                                            can be used to get node state
---------------------------- ------------------------------ --------------------------------------
smsvariable <string> <value>	<none>	                    variable is only updated by SMS, 
                                                            not created, and it must belong 
                                                            to the node
                                                            ecflow_client --get <path> can be 
                                                            used to get variable value, 
                                                            create or update any variable
---------------------------- ------------------------------ --------------------------------------
smswait <trigger expression> ecflow_client --wait <string>	
============================ ============================== ======================================

sms expects string tokens, builds up the related tree, checks its value on the server,

ecflow expect one string containing the trigger expression to check

In order to migrate tasks, and keep the ability for them to
communicate with the server in charge, SMS or ecflow, child commands
were replaced with ksh functions,

to call the right child according to the variable ECF_PORT set to 0
(sms case) or not (originated from ecflow server).
