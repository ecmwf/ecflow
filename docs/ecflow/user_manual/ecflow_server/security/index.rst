.. _security:

Security
////////

An ecFlow server is started by one user account and all tasks are
submitted by this user account by default. The advantage of this open
way of working is that anyone can support your suite, which can of
course be the disadvantage. Tasks (or suites) can be run using other
user IDs as allowed by standard UNIX permission.

At ECMWF we run ecFlow in a relatively open way. We have decided to
limit the number of accounts/users running ecFlow to simplify
cooperation and file permission problems. Most research ecFlow servers
run under one research account allowing greater cooperation. However,
for operations, we want to limit full access to a handful of trusted
users, whilst giving others the ability to monitor the operational
suites. We use the ecFlow white list file to limit access in operations.

For additional security you may choose, to have blacklist files. i.e.
password-based authentication.

.. toctree::
    :maxdepth: 1
    
    ecflow_white_list_file
    black_list_file
    open_ssl