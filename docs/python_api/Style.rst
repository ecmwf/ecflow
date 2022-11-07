ecflow.Style
////////////


.. py:class:: Style
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

Style is used to control printing output for the definition

- DEFS:  This style outputs the definition file in a format that is parse-able.
         and can be re-loaded back into the server.
         Externs are automatically added.
         This excludes the edit history.
- STATE: The output includes additional state information for debug
         This excludes the edit history
- MIGRATE: Output includes structure and state, allow migration to future ecflow versions
         This includes edit history. If file is reloaded no checking is done

The following shows a summary of the features associated with each choice

   ===================== ==== ===== =======
   Functionality         DEFS STATE MIGRATE
   ===================== ==== ===== =======
   Auto generate externs  Yes  Yes   No
   Checking on reload     Yes  Yes   No
   Edit History           No   No    Yes
   Show trigger AST       No   Yes   No
   ===================== ==== ===== =======


.. py:attribute:: Style.DEFS
   :module: ecflow
   :value: ecflow.Style.DEFS


.. py:attribute:: Style.MIGRATE
   :module: ecflow
   :value: ecflow.Style.MIGRATE


.. py:attribute:: Style.NOTHING
   :module: ecflow
   :value: ecflow.Style.NOTHING


.. py:attribute:: Style.STATE
   :module: ecflow
   :value: ecflow.Style.STATE


.. py:attribute:: Style.names
   :module: ecflow
   :value: {'DEFS': ecflow.Style.DEFS, 'MIGRATE': ecflow.Style.MIGRATE, 'NOTHING': ecflow.Style.NOTHING, 'STATE': ecflow.Style.STATE}


.. py:attribute:: Style.values
   :module: ecflow
   :value: {0: ecflow.Style.NOTHING, 1: ecflow.Style.DEFS, 2: ecflow.Style.STATE, 3: ecflow.Style.MIGRATE}

