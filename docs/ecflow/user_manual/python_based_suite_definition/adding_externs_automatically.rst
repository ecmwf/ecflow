.. _adding_externs_automatically:

Adding externs automatically
////////////////////////////

Extern refers to nodes that have not yet been defined typically due to
cross suite dependencies. Trigger and complete expressions may refer
to paths and variables in other suites that have not been loaded yet.
The references to node paths and variables must exist or exist as
externs. Externs can be added manually or automatically

- Manual Method:

  .. code-block:: python

      defs = ecflow.Defs("file.def") # open and load file 'file.def' into memory
      defs.add_extern("/temp/bill:event_name")


- Automatic Method; this will scan all trigger and complete expressions, looking for paths and variables that have not been defined. The added benefit of this approach is that duplicates will not be added. It is the user's responsibility to check that extern's are eventually defined otherwise trigger expression will not evaluate correctly

  .. code-block:: python
    
      defs = ecflow.Defs("file.def") # open and load file 'file.def' into memory
      ...
      defs.auto_add_extern(True) # True means remove existing extern first.
