.. index::
   single: Getting Started
   
.. _getting-started:

===================
**Getting Started**
===================

| First ensure that the paths to ecFlow executables are accessible.
| At ECMWF this is done using **use** command. Hence type the following at the command line.

::

  > use ecflow

Create a directory called course in your home directory and change to that directory.
::

  > mkdir course; cd course


   
| In order to use ecFlow we first need to start the :term:`ecflow_server`

Shared Machine
--------------
| On a shared machine multiple users and ecFlow servers can coexist. Start an :term:`ecflow_server`:
| by typing the following

::
      
   > ecflow_start.sh
      
| This will start an :term:`ecflow_server` running on your system with a port number unique to your user ID. 
| With this script ecFlow log files and :term:`check point` files are created in the directory $HOME/ecflow_server.
| This can be changed by using:

::

   > ecflow_start.sh -d $HOME/course
   
| Please keep a note of the **Host** and **Port** given from your ecf_start.sh output for later. 
| The host and port number uniquely identify your :term:`ecflow_server`. 
| When you want to access this server using :term:`ecflow_client`, :ref:`client_server_python_api` 
| or :term:`ecflowview` you need to know these information.

| By setting the value of the environment variables ECF_NODE and ECF_PORT you 
| identify the server you wish to access. Multiple :term:`ecflow_server`'s can run on the same system.

Local Machine
-------------
| We prefer to start the ecFlow server with the ecflow_start.sh script to help prevent unintentional 
| shared usage of the server. You could have used the default ECF_PORT and started a server running 
| on your own local machine using the following command:

::
  
      > ecflow_server
  
at the unix prompt. 

| This will start an :term:`ecflow_server` running on your system with a default host name of "localhost" and 
| port number of 3141. If another program on your machine is using this same port number, then you will get 
| an "Address in use" error. To start the server on a specific port number you can use:

::
   
      > ecflow_server --port=3500
      
or::
   
      > export ECF_PORT=3500; ecflow_server

| :term:`ecflow_server` log files and :term:`check point` files are created in the current directory by default, and have
| a prefix <machine_name>.<port_number>. As this allows multiple servers to run on the same machine.


| If you had previously run the same :term:`ecflow_server` in the past it will also attempt to recover the :term:`suite definition`
| from the :term:`check point` file. 
 
 
**What to do:**

* Type 'use ecflow' to setup up the paths.
* Create $HOME/course directory
* Start the server using the ecflow_start.sh -d $HOME/course 
* | Note: If in the subsequent sections, you have the need to start a new shell, 
  | and want access the server, then ensure that ECF_PORT is set. 

