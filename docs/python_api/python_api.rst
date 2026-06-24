
.. _python_api:

Python API
**********


Suite Definition API
====================

The Suite Definition API is used to define suites, families, tasks and their attributes,
and can be used to create a suite definition in memory, which can then be written to a
file or submitted to the server.

The elements in this API do not allow to directly set the state of a suite, family or task,
or any of their attributes. Such a state is only relevant when the suite definition is submitted to the server,
and the server is responsible for maintaining the state of the suite, family, task and their attributes.

The state of the elements of a suite definition can be set using the Client-server API.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/Alias
   reference/Autoarchive
   reference/Autocancel
   reference/Autorestore
   reference/Clock
   reference/Complete
   reference/Cron
   reference/Date
   reference/Day
   reference/Defs
   reference/Defstatus
   reference/Edit
   reference/Event
   reference/Expression
   reference/Family
   reference/File
   reference/Flag
   reference/Generic
   reference/InLimit
   reference/Label
   reference/Late
   reference/Limit
   reference/Meter
   reference/Node
   reference/NodeContainer
   reference/PartExpression
   reference/Queue
   reference/Repeat
   reference/RepeatDate
   reference/RepeatDateList
   reference/RepeatDateTime
   reference/RepeatDateTimeList
   reference/RepeatDay
   reference/RepeatEnumerated
   reference/RepeatInteger
   reference/RepeatString
   reference/SState
   reference/State
   reference/Submittable
   reference/Suite
   reference/Task
   reference/Time
   reference/TimeSeries
   reference/TimeSlot
   reference/Today
   reference/Trigger
   reference/Variable
   reference/Verify
   reference/Zombie
   reference/ZombieAttr


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    

Container API
=============

The Container API provides types capable of holding a set of objects.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/FamilyVec
   reference/FlagTypeVec
   reference/NodeVec
   reference/SuiteVec
   reference/TaskVec
   reference/VariableList
   reference/ZombieVec


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    

Command API
===========

The Command API provides command types, that can be executed directly by the client.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/UrlCmd
   reference/WhyCmd


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    

Client-Server API
=================

The Client-Server API provides the capability of communicating with the server.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/Client


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    

Common API
==========

The Common API provides configuration or support types for other parts of the API.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/Ecf
   reference/PrintStyle
   reference/JobCreationCtrl


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    

Enums API
=========

The Enums API provides types representing enumerations used in the Suite Definition API.


.. toctree::
   :maxdepth: 1
   :glob:

   reference/AttrType
   reference/CheckPt
   reference/ChildCmdType
   reference/DState
   reference/Days
   reference/FlagType
   reference/State
   reference/SState
   reference/Style
   reference/ZombieType
   reference/ZombieUserActionType


.. contents::
   :depth: 2
   :local:
   :backlinks: top
    
