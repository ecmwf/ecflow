
.. index::
   single: cookbook
   
.. _automated_zombies:
   
Automated zombies ?
-------------------

When :term:`zombie` s arise they can be handled manually by :term:`ecflowview`. (See :ref:`zombie`)
**or** via the command line interface:

* ecflow_client --zombie_get 
* ecflow_client --zombie_fail   <task-path>
* ecflow_client --zombie_fob    <task-path>
* ecflow_client --zombie_adopt  <task-path>
* ecflow_client --zombie_remove <task-path>
* ecflow_client --zombie_block  <task-path>

However it is also possible to ask :term:`ecflow_server` to make the same response in
an automated fashion. How ever **very** careful consideration should be made before doing this.
Otherwise it could mask a serious underlying problem. 

The automated response can be defined with:

* python interface( See :py:class:`ecflow.ZombieAttr`)
  
* text interface ( See :ref:`grammer`) ::

    zombie             ::=  "zombie" >> `zombie_type` >> ":" >> !(`client_side_action` | `server_side_action`) >> ":" >> *`child` >> ":" >> !`zombie_life_time` 
    zombie_type        ::=  "user" | "ecf" | "path"
    child              ::=  "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete"
    client_side_action ::=  "fob" | "fail" | "block"
    server_side_action ::=  "adopt" | "delete"
    zombie_life_time   ::=  unsigned integer  ( default:  user(300), ecf(3600), path(900)  )

The zombie attribute is inherited in the same manner as :ref:`variable-inheritance`.

Example: For tasks under suite "s1" add a zombie attribute, such that child label commands(i.e ecflow_client --label) never block:

* python ::

   s1 = ecflow.Suite('s1')
   child_list = [ ChildCmdType.label ]
   zombie_attr = ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fob, 300)
   s1.add_zombie(zombie_attr)
 
* text ::

   suite s1
      zombie ecf:fob:label:  
      

Example: For tasks under suite "s1" add a zombie attribute, such that child commands( event, meter, label) never block:

* python ::

   s1 = ecflow.Suite('s1')
   child_list = [ ChildCmdType.label, ChildCmdType.event, ChildCmdType.meter ]
   zombie_attr = ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fob, 300)
   s1.add_zombie(zombie_attr)
 
* text ::

   suite s1
      zombie ecf:fob:label,event,meter:  
      
      
Example: For all tasks under family "critical", if any zombies arise then fail the job:

* python ::

   with ecflow.Suite('s1') as s1:
      with s1.add_family("critical") as crit :
         child_list = [ ]  # empty child list means apply to all child commands
         crit.add_zombie(ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fail, 300))
         crit.add_zombie(ZombieAttr(ZombieType.path, child_list, ZombieUserActionType.fail, 300))
         crit.add_zombie(ZombieAttr(ZombieType.user, child_list, ZombieUserActionType.fail, 300))

* text ::

   suite s1
      family critical
         zombie ecf:fail::  
         zombie path:fail::
         zombie user:fail:: 
   