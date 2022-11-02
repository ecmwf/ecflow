.. _the_ecflow_pre-processor:

The ecFlow pre-processor
//////////////////////////////////

The pre-processor in ecFlow reads the ecFlow files and processes them
to form a job-file, manual-page, or for editing in ecflow_ui. It also
does the interpretation of the commands defined in **ECF_JOB_CMD** ,
**ECF_KILL_CMD, and ECF_STATUS_CMD.**

The **ecFlow pre-processor** was developed to work on any file(i.e.   
script or even python file). It allows users to do some of the        
*C-pre-processor* like tasks, at the      
first character on each line in **ecFlow file** (or **ecFlow       
script**. If that is found to be an ECF-micro character (by default '%', see ECF_MICRO) the line is for the pre-processor. If, however, the line starts with two of these characters, a single \`%'-character is passed on to the next phase (to be used as an ECF-variable introducer). 
                                           
Currently, there is no %if - statement in the pre-processor. If -  
statements, however, can be handled by the shell running the       
script. The pre-processor also carries out the variable **substitution** .    
When ecFlow is preparing to execute a task it reads the ecFlow script 
and produces a job file in which it replaces all the relevant variables.                   
                                                                  
The table below shows the pre-processor symbols that ecFlow understands.  Notice that some of them work in pairs.                            

.. list-table::
   :header-rows: 1
   :widths: 30 70
   :name: preproc_options_table

   * - Symbol
     - Description
   * - %include <filename>
     - %ECF_INCLUDE%/filename directory is searched for the **filename** and the contents included in the output. If that variable is not defined ECF_HOME is used instead. This is the recommended format for include.
   * - %include "filename"
     - Include the contents of file **%ECF_HOME%/%SUITE%/%FAMILY%/filename** into the output
   * - %include filename
     - Include the contents of the file filename into the output. Notice that since the $CWD of ecFlow can be anywhere, the only form that can be safely used must start with a slash '/'.
   * - %includeonce filename
     - nclude the contents of file filename into the output ONCE. if the filename is encountered again as **%includeonce**, then it is ignored. (Same three formats for filename as for plain **%include**)
   * - %includenopp filename
     - Same as %include, but the file is not interpreted at all. This allows you to test the filename separately with ease. (Same three formats for filename as for plain **%include**.)
   * - %comment
     - Remove all the lines from the output until a line with **%end** is found.
   * - %manual
     - If creating a job-file remove all the lines from the output until a line with **%end** is found. If creating a manual page include all the lines until a line with **%end** is found.
   * - %nopp
     - Stop the pre-processing until a line starting with **%end** is found. No interpretation of the text will be done (e.g. no variable substitutions). The line is retained, if pre-processing is requested by ecflow_ui
   * - %end
     - End processing of **%comment** or **%manual** or **%nopp**
   * - %ecfmicro CHAR
     - Change the ECF_MICRO character to the character given. If set in an include file the effect is retained for the rest of the job (or until set again). This does not change how ECF_FETCH or ECF_JOB_CMD work, they still use ECF_MICRO
                        
     
Note that for **%include,** if the **filename** starts with slash,
'/' character, no interpretation will be made. The full pathname of
the file will be used.

From ecFlow release 4.4.0 you can have variables in the include file
definition.

.. code-block:: shell

   %include <%SUITE%/file.h>
   %include %MY_INCLUDE%            # here MY_INCLUDE must expand, <filename> or "filename" or filename
   %include %MY_INCLUDE:<filename>% # if MY_INCLUDE not defined use <filename>



.. toctree::
    :maxdepth: 1

   locating_ecf_file
   manual_pages
   include_files
   comments
   stopping_pre-processing
   ecf_micro
   how_a_job_file_is_created_from_an_ecflow_file
  
