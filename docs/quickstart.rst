.. _ecflow_quickstart:

Quickstart
**********

This page gives a simple exercise that will allow you to create a very
simple ecFlow server, load it with a small suite of minimal tasks and
interact with it using the graphical user interface, :ref:`ecflow_ui`.

Once you have completed this, the :ref:`tutorial` is a
great way to gradually familiarise yourself with more details of ecFlow.

Create a directory to work in
=============================

The instructions here will assume we are working in the directory:

.. code-block:: shell

   $HOME/ecflow/quickstart
   
but any directory will do.

Download the example suite definition
=====================================

Click the following link to download the files needed to define our
sample suite:

`ecflow_quickstart.tar.gz <https://sites.ecmwf.int/repository/ecflow/test-data/tutorial/ecflow_quickstart.tar.gz>`__

Now, move it into your chosen working directory and untar it:

.. code-block:: shell

   tar xzvf ecflow_quickstart.tar.gz
   
Inspect and update the suite definition
=======================================

The file ``test.def`` defines a suite called "test". Open this file in a
text editor and note the structure.

.. code-block:: shell

   suite test
      edit ECF_HOME "/replace/with/own/home/ecflow/quickstart"
      task t1
      task t2
         trigger t1 eq complete
   endsuite

We define a suite called "test"; it defines a variable called ECF_HOME,
then it defines two tasks ("tasks" are things that actually run) and
specifies that task t2 will be run as soon as task t1 has completed. The
scripts corresponding to the tasks are specified in ".ecf" files in the
test directory. Have a look - they simply print some information about
themselves, then sleep for 2 seconds.

The first thing you must do is to complete the path to your working
directory in the ECF_HOME definition in test.def and save the file.

Start an ecFlow server
======================

We will start a new running instance of an :term:`ecflow_server` using the
default port. It is possible to use a different port by adding
--port=3500 (for example) to every ecFlow command-line action, or by setting
the environment variable ECF_PORT to the desired port number. Note also
that we start it as a background task - it will run until the server is
stopped. It can be run in the foreground, but in that case you will need
to use a new terminal for any subsequent commands!

.. code-block:: shell

   ecflow_server &

Check that it is running using the :term:`ecflow_client`:

.. code-block:: shell

   ecflow_client --ping    

Load your suite definition into the server
==========================================

.. code-block:: shell

   ecflow_client --load=test.def   

Check that it is loaded by asking the server to give you back the suite
definition:

.. code-block:: shell

   ecflow_client --get 

Monitor and interact via the GUI
================================

Start :ref:`ecflow_ui`:

.. code-block:: shell

    ecflow_ui &  
    
Once :ref:`ecflow_ui` has started, you must tell it how to reach your server. Go
to the Servers → Manage Servers menu, click "Add server", then enter the
details of your server. **Name** can be anything you want - it's for you
to identify the server to your self; something like "localtest" would be
fine here. **Host** should in this case be "localhost", and **Port**
should be XXX. The other fields can be left blank, but keep the "Add
server to current view" box ticked.

You should now see your suite loaded into the GUI! To make the server
active, right-click on the top-level node representing the server
("localtest in our case) and choose "Restart". Now right-click on the
"test" node and choose "Begin" to make the suite active. The default
behaviour is to only refresh its view of the suite every 60 seconds, so
you will need to click the green refresh button at the top every so
often to see the progress of the tasks.

.. image:: /_static/ecflow_quickstart/image1.png
   :width: 3.4375in
   :height: 2.60417in
