.. _black_list_file:

Black list file (experimental)
//////////////////////////////

This allows user commands to be authenticated via passwords.

**In ecflow 5 this functionality is enabled by default**.       

One password file must be specified for the client and server. In both
cases, the file is located via ECF_PASSWD environment variable.

The default internal name of the password file is **ecf.passwd**, hence
the server will by default look for the password of name
**<host>.<port>.ecf.passwd**.

The format of the file is the same for both client and server. 

.. warning::

    Is up to the user and server administrator, to **set the right     
    permissions** on the file.                                         

.. code-block:: shell
    :caption: Client side password file, for user fred

    4.5.0  # this is the version number
    # comment
    # <user> <host> <port> <passwd>
    fred machine1 3141 xydd5j
    fred machine2 3141 xydd5j
    fred machine3 3141 xydd5jggg

This format allows the same file to be used for multiple servers. 

.. code-block:: shell
    :caption: Example server password file, running on machine1 and port 3141
    
    4.5.0
    user1 machine1 3141 sdfrg
    user2 machine1 3141 ssdft
    fred  machine1 3141 xydd5j                                          

The password file for the server must contain at **least one** user that
matches the host and port of the server, otherwise, an error is issued,
and the server can not be started.

If ECF_PASSWD environment variable is specified, then the file is read
by the client and server.

Every client user command sends the password to the server. The server
will then compare the password for the user with its own. If it matches,
the command is authenticated, otherwise, the command fails. 

If the password is set on the server, but not on a client, then that
user will be denied all access.

The password file can be reloaded to add/remove users. However, only
users who appear in the password file can do this:

.. code-block:: shell
   :caption: Reload password file, after adding/removing user

    ecflow_client --reloadpasswdfile                                   


.. warning::

    Although the password file can be re-loaded, its file location can 
    not be changed.                                                    
