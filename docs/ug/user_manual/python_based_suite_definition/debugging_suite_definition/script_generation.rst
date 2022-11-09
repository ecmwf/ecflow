.. _script_generation:

Script Generation
///////////////////

This is suitable when you are only concerned about the design and behaviour of your suite definition.

It is only available with the ecFlow :ref:`python_api`.

The ecFlow python API can automatically generate the '.ecf' scripts for
any definition.

The header and tail include that allows communication with the server
will also be generated. (This assumes that server is running locally) It
uses the content of the definition to parametrise what gets generated,
and the location of the files.

Hence if a task definition specifies events, meters, or labels, then the
generated scripts, will place ecFlow child commands that will exercise
those attributes in the generated scripts.

It makes a few assumptions.

- ECF_HOME directory is specified and accessible, with valid
  permission for auto-creation of directories
- ECF_INCLUDE directory is specified, with valid permission for the
  auto-creation of directories. There is where the auto-generated
  header file are placed( **head.h** and **tail.h** )
- If ECF_FILES is specified, then scripts are generated under this
  directory otherwise ECF_HOME is used
- If the variable ECF_DUMMY_TASK is specified on a task, it will
  **not** generate a script.
- If the script file already exists, the script generation will fail.
  (To avoid overwriting user-specified .ecf scripts)
- The generated scripts assume that ecflow_client executable is
  accessible. This can be overridden by adding an ecFlow variable
  ECF_CLIENT_EXE_PATH
- The generated header and tail include files, assume the server is
  on the local machine.

A ready-made python script that calls script generation can be seen in
the cookbook section. See https://confluence.ecmwf.int/display/ECFLOW/How+can+I+test+a+definition+without+writing+scripts
