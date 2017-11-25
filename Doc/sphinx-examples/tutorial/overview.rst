
.. index::
   single: overview
   
.. _overview:
   

Overview
========

This section gives an overview of the steps involved in using ecFlow.

Write a suite definition
------------------------
     
The :term:`suite definition` defines how your tasks run and interact.
:term:`task` s are placed in families, which themselves may be placed in families
and/or :term:`suite` s. All the entities are called :term:`node` s and form a
hierarchical tree.
  
There are two main methods for describing a :term:`suite definition` to the :term:`ecflow_server`.  
  
* via a **text** :term:`suite definition` 
    
  The grammar of this text definition is described by :ref:`grammer`.  
  This grammar does not support conditional statements (such as if,while,for)
  nor the ability to define functions. However, the text definition file can 
  be generated/created using any language which in itself supports conditional statements.  
  The text definition is similar to that offered by SMS/CDP and as such may be an 
  appropriate migration path for some.

* via a **python** :term:`suite definition`
    
  This allows more checking and functionality and as 
  such is our **preferred** method. See :ref:`python_api`.
     
Write your scripts
------------------
   
:term:`ecf script` will correspond with the :term:`task` in the :term:`suite definition`. 
The script defines the **main work** that is to be carried out.
The script includes :term:`child command` s and special comments and manual sections
that provides information for operators.
     
The :term:`child command` s are a restricted set of :term:`ecflow_client` commands that communicate with
the :term:`ecflow_server`. They inform the server when the job has started, completed or
set some :term:`event`.

Start a server
--------------

After :term:`ecflow_server` is started, the :term:`suite definition`, can then be loaded into it.

* The user then initiates :term:`scheduling` in the :term:`ecflow_server`
* :term:`scheduling` will check :term:`dependencies` in the :term:`suite definition`
  every minute. If these :term:`dependencies` are free, the server will submit the :term:`task`.
  This process is called :term:`job creation`. The running process corresponding to the
  :term:`task` is referred to as a job.
     
The running jobs will communicate back to the server using :term:`child command` s.
These cause:

* :term:`status` changes on the :term:`node` s held in the server. 
* update to attributes of a node (i.e like :term:`event` s, :term:`meter` s and :term:`label` s)
   
Use the GUI
-----------

ecFlow has a specialised GUI client, called :term:`ecflowview`. This is used to
visualise and monitor:

* The hierarchical structure of the :term:`suite definition`. (:term:`suite`, :term:`family`, :term:`task`)
* state changes in the nodes and server.
* Attributes of the nodes and any :term:`dependencies`.
* :term:`ecf script` file and the expanded :term:`job file`.
   
In addition :term:`ecflowview` provides a rich set of :term:`ecflow_client` commands that can interact with
the server.
   
.. note::

   The following tutorial will show examples in plain text and python.
   However it is recommended that you use python, since the later tutorial
   examples use conditionals like 'if' and looping constructs.
