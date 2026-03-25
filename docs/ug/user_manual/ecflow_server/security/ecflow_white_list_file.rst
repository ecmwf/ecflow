.. _ecflow_white_list_file:

ecFlow White List file
//////////////////////


Ecflow black list file files( <host>.<port>.ecf.passwd) deals with
authentication and white list files(<host>.<port>.ecf.lists) deals with
authorisation.

ecFlow white list file is a way of restricting the access to ecFlow to
only known users.

The file lists users with full access and users with only read access.
The read-only user names start with '-' (dash/minus). Note you must
include a version number, e.g. 4.4.14 as the first non-comment line.

The environment variable ECF_LISTS is used to point to the white list
file. The white list file is an ASCII file.

Named users with write and read access (file ecf.lists):

.. code-block:: shell

  4.4.14 # whitelist version number
  #Maintenance group and operators
  #
  uid1
  uid2
  cog
  #
  #Read-only users
  #
  -uid3
  -uid4                                                 


Provide all users with read access:

.. code-block:: shell

  4.4.14 # whitelist version number
  #Maintenance group and operators
  #
  uid1
  uid2
  cog
  #
  #All other users have read access
  #
  -*                                                            

Restrict access to given set of nodes:

.. code-block:: shell

  4.4.14
  fred             # has read /write access to all suites
  -joe             # has read access to all suites
    
  *  /x /y    # all users have read/write access to suites /x /y
  -* /w /z    # all users have read access to suites /w /z
    
  user1 /a,/b,/c  # user1 has read/write access to suite /a /b /c
  user2 /a
  user2 /b
  user2 /c       # user2 has read write access to suite /a /b /c
  user3 /a /b /c # user3 has read write access to suite /a /b /c
    
  -user4 /a,/b,/c  # user4 has read access to suite /a /b /c
  -user5 /a
  -user5 /b
  -user5 /c    # user5 has read access to suite /a /b /c
  -user6 /a /b /c   # user6 has read access to suite /a /b /c

Composable White List files
***************************

In case there are distinct lists of users to manage (e.g. users from
different teams or entities), consider composing white list files using
the following syntax.

.. code-block:: shell

    #include <path_to_other_white_list_file>

This allows having a 'main' white list file that includes other white
list files. The ecFlow server will consider the combination of all the
white list files as a single white list file.

.. important::

    The path, given in between `<>`, is considered to be:

      1. an relative to the ecFlow server current working directory, if it is a relative path;
      2. otherwise, it must be a valid absolute path.

The following example shows how to have a 'main' file include two other white list files.

.. code-block:: shell
   :caption: The 'main' white list file

    5.16.0

    #Include the white list file for the maintenance team and the operators

    #include <operators.ecf.lists>
    #include </opt/shared/maintenance.ecf.lists>

.. code-block:: shell
   :caption: The 'operators.ecf.lists' white list file (located in the ecFlow server current working directory)

    5.16.0

    alice
    bob /a,/b,/c
    -charlie /a,/b,/c

.. code-block:: shell
   :caption: The 'maintenance.ecf.lists' white list file (located at /opt/shared/)

    5.16.0

    xavier
    yolanda
    -zach /x /y /z

Reload the White List file
**************************

If you edit this file while ecFlow is running you need to use the
following command to activate the change in ECF::

    ecflow_client --reloadwsfile                                       
