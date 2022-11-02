.. _viewing_scripts,_jobs_and_output:

Viewing scripts, jobs and output
////////////////////////////////


Overview
========

ecFlowUI provides rich facilities for viewing tasks' scripts, jobs and
job output. Although their usage should be fairly self-explanatory, some
details are given here.

To see this information about a node, either right-click on the node and
choose one of the options from the context menu such as "**Output
...**", "**Script ...**" or "**Job ...**", or create an :ref:`Info
Panel <getting_started_with_ecflowui>`
- information for the selected node will appear there. The script, job
and output tabs have much in common, so most of the information below is
relevant for them all.

Header strip
============

At the top of these tabs is a shaded area which provides information
about the file being viewed, for instance its size and how it was
retrieved (e.g. straight from disk, from a log server or from the ecFlow
server).

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image1.png
   :width: 5.90069in
   :height: 1.28498in

To the right of this is a toolbar, providing common buttons to control
the following:

-  to change the font size, click on the two 'A' buttons; alternatively,
   hold down the CTRL button whilst moving the mouse wheel

-  to bring up the search bar, click the magnifying glass

-  to go to a particular line in the file, click the magnifying
   glass/123 icon

All these buttons have tooltips, providing further information about
their keyboard shortcuts if you hover the mouse cursor over them.

Searching for text
==================

The search bar allows for powerful text searching within the viewed
file.

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image2.png
   :width: 5.90069in
   :height: 3.40065in

The start of the search bar is a verb, representing the search type:

-  **Matches** is a wildcard match, similar to UNIX file syntax, where
   "*" means "any number of any characters" and "?" means "one of any
   character", e.g. "*monitor?". If no wildcard characters are used, the
   result will be that only those strings that exactly match the search
   expression will be found. This is often the simplest way of
   searching. See also
   http://doc.qt.io/qt-5/qregexp.html#wildcard-matching.

-  **Regexp** is a regular expression match, using the Qt regexp syntax.
   The regexp equivalent of the above example would be ".*monitor.". See
   also http://doc.qt.io/qt-5/qregexp.html#introduction.

-  **Contains** finds strings which *contain* the given regular
   expression as a substring; similar to **Regexp**, but can find more
   matches since the regexp only needs to match a substring rather than
   the whole string.

Note that the search works within a line, not between lines.

The spanner icon to the right of the search bar provides more options to
customise the search, and also the ability to highlight all matches in
the text.

As text is typed into the search bar, the results of the search are
updated almost immediately (but see `Large file
mode <#large-file-mode>`__, below).

Caching
=======

Once a remote file has been transferred and viewed, it will be cached
locally to speed up future access. To get the latest version of a job's
output, see `Reload <#reload>`__, below.

Job output
==========

The job output tab has some additional functionality over the others,
described in the following sub-sections.

File listing
------------

Beneath the output file is a list of files available for that node;
select a file to view its contents. The file defined by ECF_JOBOUT is
considered to be the the **current** one, and will be selected by
default and is highlighted in the file list. If ECF_TRYNO is 0 then the
task is not currently running and hence there is no **current** output
file; in this case the viewer will be empty until you select one of the
files from the list.

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image3.png
   :width: 5.90069in
   :height: 2.4283in

Detailed file information
-------------------------

Since there can be several steps to retrieving an output file, this
information is made available by clicking on the Information button in
the toolbar.

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image4.png
   :width: 3.76802in
   :height: 2.60417in

Local output files
------------------

Output files may be read in one of three ways, attempted in the
following order:

-  from a log server (if defined)

-  directly from disk (optional)

-  from an ecFlow server

ecFlowUI will always attempt to obtain a file from the log server if it
is defined for this task. Then, optionally, it attempts to read the file
directly from disk if it is accessible. Finally, if the file has not yet
been read, ecFlowUI will request the file from the ecFlow server.

This behaviour tries to avoid the situation where the wrong file is read
because the same path exists on the machine where the task is running
and on the machine where ecFlowUI is running, but they are not the same
file. As an example, consider that the machine that ran the job wrote
its output into a file called /tmp/myjob.1, where /tmp is a file system
local to that machine. If a different file of the same name exists on
the machine where ecFlowUI is running, this could be read instead of the
'real' output file.

If you know that this could be the case, you should go to the :ref:`Server
Settings <communication_with_ecflow_servers>`
and deactivate the option **Read files from disk when appropriate**.

Reload
------


.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image5.png
   :width: 0.23659in
   :height: 0.20001in
\ Click
the **Reload** button to obtain the latest version of the job output
file. Note that these files can become very large, and can take some
time to transfer across a network if they are hundreds of megabytes or
more. If you are viewing an old output file and you click the **Reload**
button, the file viewer will switch to the **current** version of the
file (it assumes there is no need to reload an old version of the file).
This button also causes the file listing to be updated.

Saving a local copy of the job output
-------------------------------------

The job output toolbar has a 'disk' icon which allows you to save a
local copy of the currently-displayed file. This will invoke a standard
'file save' dialogue from where you can choose the location to save the
file to.

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image6.png
   :width: 3.02083in
   :height: 0.73958in

Automatic search
----------------

The job output tab follows this algorithm when it is reloaded or loaded
for the first time:

-  if the search bar is open and contains a search term, then its search
   is performed

-  otherwise, if the viewed file is the **current** output for the
   selected node, an automatic search for keywords is invoked: search
   backwards through the document for the strings --abort and
   --complete; it is intended to make this behaviour user-configurable
   in the future.

Large file mode
---------------

This should be largely transparent to the user, but when viewing a log
file which is greater than 1MB, the file viewer goes into "large file
mode", where it loads only the visible portion of the file into memory.
This mode gives a massive saving in memory usage and also significantly
increases performance.

If the file is larger than 5MB, the search results are no longer updated
immediately as text is typed into the search bar; you must hit **enter**
to initiate the search (as indicated in a warning bar). This mode allows
the user interface to remain responsive while typing a search term.

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image7.png
   :width: 5.90069in
   :height: 0.41963in

.. image:: /_static/ecflow_ui/viewing_scripts,_jobs_and_output/image5.png
   :width: 0.23659in
   :height: 0.20001in
