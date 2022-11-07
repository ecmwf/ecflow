.. index::
   single: server security  (tutorial)
   single: whitelist file (tutorial)

.. _tutorial-server-security-whitelist-file:

Server security - whitelist file
=================================

To control other users' access rights to your ecFlow server you can define a white list file.

The default name of the file is **ecf.lists** hence the server will by default look for **<host>.<port>.ecf.lists**.

The ECFLOW white list file is a way of restricting the access to ecFlow to only known users. 
The file lists users with full access and users with only read access.
Including a user name gives full read/write access, starting with '-' (dash/minus) gives read access only.
Note you must include a version number, e.g. 4.4.14 as the first non-comment line to allow for potential future updates to the format.
The environment variable ECF_LISTS is used to define the white list file name which should exist in the ECF_HOME folder of your server.
The white list file is an ASCII file.

.. note::

    When creating a white list file (ecf.lists) be sure to include your user name with read/write privileges. Otherwise, you can be locked out of your own server.   

File ecf.lists:

.. code-block:: shell
    :caption: Named users with write and read access

    4.4.14 # whitelist version number
    #Maintenance group and operators
    #users with read/write access
    myuid
    uid1
    uid2
    #
    #Read-only users
    #
    -uid3
    -uid4    

For more information see :ref:`ecflow_white_list_file` and :ref:`reloadwsfile_cli`.

**What to do**

#. The expected name of the whitelist file for your server is available from the ECFLOW variable ECF_LISTS. Check this.
#. In the home directory of your ecFlow server create a file given by ECF_LISTS containing your user id with read/write access.
#. Load the white list file onto your server using the client command::

    ecflow_client --reloadwsfile

