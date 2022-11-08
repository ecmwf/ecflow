ecflow.NodeContainer
////////////////////


.. py:class:: NodeContainer
   :module: ecflow

   Bases: :py:class:`~ecflow.Node`

NodeContainer is the abstract base class for a Suite and Family

A NodeContainer can have Families and Tasks as children


.. py:method:: NodeContainer.add_family( (NodeContainer)arg1, (str)arg2) -> Family :
   :module: ecflow

Add a :term:`family`. See :py:class:`ecflow.Family`.
    
    Multiple families can be added. However family names must be unique.
    for a given parent. Families can be hierarchical.
    
    Exception:
    
    - Throws RuntimeError if a duplicate is added
    
    Usage:
    
    .. code-block:: python
    
      suite = Suite('suite')          # create a suite
      f1 = Family('f1')               # create a family
      suite.add_family(f1)            # add family to suite
      f2 = suite.add_family('f2')     # create a family and add to suite
    

add_family( (NodeContainer)arg1, (Family)arg2) -> Family


.. py:method:: NodeContainer.add_task( (NodeContainer)arg1, (str)arg2) -> Task :
   :module: ecflow

Add a :term:`task`. See :py:class:`ecflow.Task`
    
    Multiple Tasks can be added. However Task names must be unique,
    for a given parent. Task can be added to Familiy's or Suites.
    
    Exception:
    
    - Throws RuntimeError if a duplicate is added
    
    Usage:
    
    .. code-block:: python
    
      f1 = Family('f1')      # create a family
      t1 = Task('t1')          # create a task
      f1.add_task(t1)          # add task to family
      t2 = f1.add_task('t2') # create task 't2' and add to family

add_task( (NodeContainer)arg1, (Task)arg2) -> Task


.. py:method:: NodeContainer.find_family( (NodeContainer)arg1, (str)arg2) -> Family :
   :module: ecflow

Find a family given a name


.. py:method:: NodeContainer.find_node( (NodeContainer)arg1, (str)arg2) -> Node :
   :module: ecflow

Find immediate child node given a name


.. py:method:: NodeContainer.find_task( (NodeContainer)arg1, (str)arg2) -> Task :
   :module: ecflow

Find a task given a name


.. py:property:: NodeContainer.nodes
   :module: ecflow

Returns a list of Node's

