.. _dependencies:

Dependencies
////////////

The following section describes commands that are used to create      
dependencies between nodes in a suite.                                
                                                                     
A node can be made dependent in following ways: time/date, other      
node(s) or limited from running by some resource. There can be        
multiple time dependencies which may be expressed using the time of   
day, the day of the week, or the date. A node dependency is expressed 
as a logical statement about another node and its state, like         
**taskname == complete** . A dependency may involve several other     
nodes, preferably all in the same suite. A node that is dependent     
cannot be started as long as some dependency is **holding** it. For   
triggers, the phrase **trigger is set** means a trigger has           
**expired** , and **trigger is not set** means it is still            
**holding** . By default, a node depends on its parent. So, for       
example, a task cannot start if the family to which it belongs is     
still waiting on a dependency.                                        
                                                                     
A node can have many time dependencies, but only one, albeit complex, 
trigger. When a suite begins, the trigger and all the time            
dependencies hold the node. The node stays queued as long as the      
trigger is not set. Only one of the dependency types (time, date or   
day, and trigger) can **expire** at a time, the others still remain   
holding or in use. If the trigger is set, one of the possible time    
dependencies may expire and let the node go. When the node completes, 
the expired time dependency is marked as being used, and the other    
two time dependencies are processed.                                  
                                                                     
The node can be dependent because:                                    

- Server is halted or shutdown
- Its parent is dependent
- It is triggered by a state of another node
- It is waiting for time of day
- It is waiting for date of year
- It is waiting for day of a week
- Limit it uses does not have a free token
- It is migrated (restored at begin)
- It is suspended

The following sections discuss the different dependency types, and give
examples of how to use them together.

Table of contents
=================

.. toctree::
    :maxdepth: 1

   trigger
   date
   day
   time
   today
   cron/index  
   using_dependencies_together
