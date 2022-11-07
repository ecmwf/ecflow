.. _ecflow_white_list_file:

ecFlow White list file
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

If you edit this file while ecFlow is running you need to use the
following command to activate the change in ECF::

    ecflow_client --reloadwsfile                                       
