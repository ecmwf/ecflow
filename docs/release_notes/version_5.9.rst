.. _version_5.9:

Version 5.9 updates
*******************


Version 5.9.2
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2022-11-30


ecFlowUI
----------

- **Improvement**: added option to specify remote user id for ssh transfer when run via proxychains

- **Fix**: fixed issue where the UI could not generate the output directly listing when run via proxychains if the local username does not match the remote username


Version 5.9.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2022-11-10


ecFlowUI
----------

- **Fix**: fixed issue where clicking on a node in the Triggers tab could cause a crash


Version 5.9.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2022-10-07


ecFlowUI
----------

**Output log**

- **Improvement**: remote access via teleport to ecFlow servers at ECMWF was improved as described here: :ref:`using_ecflowui_via_the_ecmwf_teleport_gateway`
- **New feature**: added new request type to the **log server** to transfer only the portion of the log file from a given byte offset. With this ecFlowUI can highly optimise file transfer from log servers supporting this option (see below).
- **Improvement**: previously the **log server** applied a text filter on the file contents it transmitted. This filter is now removed and the log server sends the full file contents leaving the optional filtering to the client applications.
- **Improvement**: the log file access has been refactored internally to optimise loading updates for the currently viewed log file. When pressing the (blue) reload button in the **Output panel**:

    - for files served by a logserver an incremental file transfer is initiated whenever the logserver is supporting this request type

    - for local files only the increment is added to the text viewer

    - when there is no increment (the file did not change) the text viewer is not updated at all (previously it always reloaded the whole file)

  A full reload can still be initiated by using the **Load whole file** button under the **More actions** submenu. In the same submenu the **Load current jobout file** button was added to load the current jobout file.

    .. image:: /_static/ecflowui_release_notes/image1.png
          :width: 230px
         
- **Improvement**: rearranged buttons in the **Output panel**. Some buttons were moved into a menu accessible from the **More actions** button to the right.
  
    .. image:: /_static/ecflowui_release_notes/image2.png
          :width: 350px
          
- **Improvement**: the progress bar for remote file transfers now only appears after a delay so it is not visible for short enough transfers. Previously it appeared immediately the file update was requested.

- **Improvement**: added options to **show/hide** the **directory listing panel** at the bottom. This can be done using the toggle button at the top of the **Output panel**. On top of that a close button was added to the directory panel. When the directory panel is hidden it is also **disabled** and ecFlowUI does not try to update it automatically.

    .. image:: /_static/ecflowui_release_notes/image3.png
         :width: 220px

- **New feature:**  added button to the **directory listing panel** in the **Output panel**  to show detailed information about how the directory information was updated.
  
    .. image:: /_static/ecflowui_release_notes/image5.png
        :width: 250px


- **Improvement**: In the various views and info labels the file size label for files larger than 1 GB is now displayed with 1 decimal place precision, e.g. 1.3 GB (previously it was displayed as an int).
 
- **Fix**: fixed issue when a stray coloured rectangle appeared in the Output panel message label when it was selected
   

**Manage servers**   

- **New feature**: the system server list access has been redesigned. Previously a centrally installed file defined this list for each ecflow installation at ECMWF. From this version on **multiple** system server files can be defined independently from the actual installation using either the **ECFLOW_SYSTEM_SERVERS_LIST** environment variable or specific settings in the ecFlowUI **configuration dialogue**. See the full description here: :ref:`system_server_list_files_in_ecflowui`.

- **Improvement**: when changing a host or port or renaming a server all the user interface components (views and panels) are automatically adjusted to the changes. Note: when the host or port altered the original server will be removed from the tree view and a newly created one will be added to the end of the tree.

- **Improvement**: reports error for newly created servers when host or port contain whitespace characters. When a server added/modified in the ui the following rules are applied:
  
   - name: cannot contain whitespace characters except space. Cannot start or end with space.
   - host: cannot contain whitespace characters
   - port: can only contain digits
   - user: cannot contain whitespace characters
   
**Node search**

- **New feature**: added new Status change option called "older than" to the filter.  It is used to filter nodes with status changes older than the specified period.
  
    .. image:: /_static/ecflowui_release_notes/image6.png
       :width: 400px
   
**Timeline and Server load panels**

- **Improvement**: added button (see the << or >> icon) to show compacted file information in the Timeline and Server load panels. In compact mode only the file name is visible in the file information label, but the full information is still displayed as a tooltip.
  
    .. image:: /_static/ecflowui_release_notes/image7.png
           :width: 250px
         
   
**Miscellaneous**   

- **Improvement**:  when opening the Preferences dialog it will show the last used tab in the current configuration category.
     
- **Improvement**:  when ecflow_ui is started with the -h option (help) the version info is also displayed
    
    .. image:: /_static/ecflowui_release_notes/image8.png
           :width: 250px
           
- **Fix**: fixed an issue when shortcuts were not visible in the the node context menus.
    
- **Fix**: fixed an issue when setting variable value for multiple nodes was only applied to the first selected node.
     
- **Fix**: fixed an issue when Info panel tabs were not notified about selection change when they became unselected
