.. _ecflow_overview:

ecFlow overview
///////////////


To use ecFLow you need to carry out a few steps

-  Write a **Suite definition**. This shows how your various tasks run
   and interact. **Tasks** are placed into **families** which themselves
   may be placed into families and/or **suites**. All these entities are
   called **nodes**

-  Write your scripts (.ecf files); these will correspond with Task's in
   the **suite** **definition.** The script defines the main work that
   is to be carried out. The script includes **child commands** and
   special comments and manual sections that provide information for
   operators.

The `child commands <https://confluence.ecmwf.int/display/ECFLOW/Glossary>`__ are a restricted set of client commands that communicate with the server. They inform the server when the job has started, completed, or set an **event**.

Once these activities are done, the ecFlow server is started and the suite definition is loaded into the server.

-  The user then initiates **task scheduling** in the server

-  **Task scheduling** will check dependencies in the suite definition
   every minute. If these dependencies are free, the server will submit
   the task. This process is called **job creation**.

The process of **job creation** includes:

-  Locating '.ecf' the script, corresponding to the Task in the **suite
   definition**

-  Pre-processing the '.ecf' file. This involves expanding any includes
   files, removing comments and manual sections, and performing
   **variable substitution**.

-  The steps above transform a '.ecf' script to a job file that can be
   submitted.

The running jobs will communicate back to the server using `child commands <https://confluence.ecmwf.int/display/ECFLOW/Glossary>`__. This causes **status changes** on the **nodes** in the server and flags can be set to indicate various events.
  
ecFLow has a specialised GUI client, called `ecflow_ui <https://confluence.ecmwf.int/display/ECFLOW/Using+ecflow_ui>`__. This is used to visualise and monitor:

-  The hierarchical structure of the **suite definition** (Suite,
   Family, Task's i.e. **nodes**)

-  **state changes** in the **nodes** and the server

-  Attributes of the **nodes** and any **dependencies**

-  Script file '.ecf' and the expanded job file

-  The log file

In addition, it provides a rich set of client commands that can interact with the server.

The following sections will provide more detail on the overall process.
