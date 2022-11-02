.. _trigger:

trigger
///////

This defines a dependency for a task or family. There can be only one
trigger dependency per node, but that can be a complex boolean
expression of the status of several nodes. Triggers should be avoided on
suites.

A node with a trigger can only be activated when its trigger has
expired. A trigger holds the node as long as the trigger's expression
evaluation returns false. There are a few additional keywords and some
names may point to other nodes with their value acting as the status of
those nodes. Trigger mathematics are computed, using double arithmetic
(with no string comparisons). There should not be any need to use
numerical expressions, instead logical functions (and, or, not, eq, ne)
with node names should be used.

The keywords in trigger mathematics are: **unknown, suspended,        
complete, queued, submitted, active** and **aborted** for task and    
family status; and **clear** and **set** for event status. These keywords are treated as numbers   
starting from 0 (unknown) to 6 (aborted). There is no need to be      
aware of the numerical values as long as you do not use a trigger in the form:               

.. code-block:: shell

  trigger plain_name # WARNING! DO NOT USE!                             
                                                                      
This is true only as long as the status of plain_name is unknown. It  
is not advisable to use mathematical function names for node names.   
                                                                      
The **full name** or **relative name** of a node can also be used as  
an operand. A full name starts from the super-node. A **relative      
name** can include "../" to indicate the parent node level. A relative name can also include "./' 
to indicate the same level, this is needed if the task name is        
numeric (otherwise its numeric value would be used in the expression.)                       

.. code-block:: shell

  family foo
    task bar
    task foobar
  endfamily
  family second
    task 00z
      trigger ../foo/foobar==complete # task from previous family
    task another
      trigger ./00z == complete # the previous task 

For events it is convenient to use a plain name, since an event can only have values **clear** or **set**, numerically 0 or 1. So triggers of the form:

.. code-block:: shell

  trigger taskname:event
  trigger taskname:event == set


Will hold as long as the event is not set. The second line shows a    
clearer alternative equivalent way to write the same trigger.         
                                                                    
Meters can be used in triggers the same as events, except that their  
value should be compared against numerical expression. It is          
important to remember to use **greater or equal** instead of **equals** . In the following  
example **foobar** will not be submitted if, let's say, suite is      
suspended while **foo** sets it meter to first 120 and then to 130. **bar** will still be submitted once the suite is resumed.                                                     

.. code-block:: shell

  task foo
    meter hour 0 240
    task bar
    trigger foo:hour >= 120
    task foobar
    trigger foo:hour == 120 # dangerous !!!
    
There is no automatic checking for deadlocks, which can be difficult to detect. However, if your suite is known to complete (i.e. it does not run forever), then simulation can be used to check for deadlocks.

The following example is a simple case:

.. code-block:: shell

  task a ; trigger ./b == complete
  task b ; trigger ./a == complete # DEADLOCKS tasks a & b 

There is no automatic simplification of the mathematics. ecFlow will
read the whole of a suite definition into memory, but with comment lines
removed and possibly different indentation.

Mathematical expressions must be given in a single logical line. Use
continuation lines for long expressions. For example:

.. code-block:: shell

  trigger /suite/family1/task1==complete and ( /suite/family2 \
          eq complete or /suite/family3 eq complete )

There cannot be any characters after the **line continuation character**
\'\\'; any keyword can appear in an expression but they must be used in a
way that makes sense. For example, a task can *never* be **set** or **clear** and,
likewise, an event can only be **set** or **clear** .

See section https://confluence.ecmwf.int/display/ECFLOW/extern  on for details on using triggers external to the suite.
