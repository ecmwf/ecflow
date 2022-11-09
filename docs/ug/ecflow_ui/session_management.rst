.. _session_management:

Session management
//////////////////

ecFlowUI automatically saves its configuration into a *session*. A
session describes the following things:

-  the set of open windows and tabs

-  the lists of servers viewed in each window and tab

-  the settings (e.g. update period) for each server

-  visual preferences such as colour schemes and tree display options

Without any explicit action from the user, ecFlowUI will record these
details into a session called *default*, and use this each time it is
started.

However, it is also possible to maintain several sessions, via the
*session manager*. To access this, start ecFlowUI with the -s
command-line option::

    ecflow_ui -s                                                       

The session management dialogue will appear, providing the following
options:

-  clone an existing session

-  rename a session

-  delete a session

There is also the option to use the selected session by default; if
ecFlowUI is started up without the -s switch and this option is set,
then it will automatically load the specified session; otherwise it will
load the *default* session.

.. image:: /_static/ecflow_ui/session_management/image1.png
   :width: 3.54715in
   :height: 2.60417in

If any session other than *default* is chosen, it will show in the main
window's title bar:

.. image:: /_static/ecflow_ui/session_management/image2.png
   :width: 3.65633in
   :height: 0.26317in
