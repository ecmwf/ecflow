.. _ecflow_variables:

ecFlow variables
////////////////

ecFlow makes heavy use of different kinds of variables. There are
several kinds of variables:

- **Environment variables** that are set in the UNIX shell before the ECFLOW-programs start. These control the server, and client (CLI).
  
- **Internal variables**: **suite definition variables**. These control servers, ecflow_ui, and CLI.
  
- **Generated variables**: These are generated within the suite definition node tree during job creation and are available for use in the job file.

This chapter lists the generated and user-defined variables which have
special meaning for ecFlow itself.

In an ecFlow script, ecFlow variables are written as text enclosed by a
pair of %-characters (the micro-character.) As in C-format strings, if
there are two %-characters together they are concatenated to form a single %-character
in the job-file. For example, if you need to execute the UNIX date
command **date +%d**. For a job, you must enter it as **date +%%d** into the
ecFlow file.

The default micro-character is defined when ecFlow is compiled. It is
possible to configure the micro-character to be defined as a variable
**ECF_MICRO** (see section 3.6). The default installation uses the %-character.

You can define variables in a suite definition file using the **edit**
keyword. User-defined variables can occur at any node level: suite,
family, or task. ecFlow also **generates** variables from the node name, the host on which
ecFlow is running, the time, the date, and so on.

.. toctree::
    :maxdepth: 1

   variable_inheritance
   ecflow_server_environment_variables
   environment_variables_for_the_ecflow_client
   ecflow_suite_definition_variables
   generated_variables
   variables_and_substitution
