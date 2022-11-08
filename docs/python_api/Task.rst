ecflow.Task
///////////


.. py:class:: Task
   :module: ecflow

   Bases: :py:class:`~ecflow.Submittable`

Creates a :term:`task` :term:`node`. Task is a child of a :py:class:`ecflow.Suite` or :py:class:`ecflow.Family` node.

Multiple Tasks can be added, however the task names must be unique for a given parent.
Note case is significant. Only Tasks can be submitted. A job inside a Task :term:`ecf script` (i.e .ecf file)
should generally be re-entrant since a Task may be automatically submitted more than once if it aborts.
There are serveral ways of adding a task, see examples below

Constructor::

  Task(name, Attributes)
     string name : The Task name.Name must consist of alpha numeric characters or
                   underscore or dot. First character cannot be a dot.
                   Case is significant
     attributes: optional, i.e like Meter, Event, Trigger etc

Exception:

- Throws a RuntimeError if the name is not valid
- Throws a RuntimeError if a duplicate Task is added

Usage:

.. code-block:: python

  task = Task('t1')            # create a stand alone task
  family.add_task(task)        # add to the family
  t2 = family.add_task('t2')   # create a task t2 and add to the family

  # Create Task in place
  defs = Defs(
           Suite('s1',
              Family('f1',
                 Task('t1',
                    Trigger('1==1'),
                    Edit(SLEEP='10'))))) # add Trigger and Variables in place


.. py:property:: Task.aliases
   :module: ecflow

Returns a list of aliases


.. py:property:: Task.nodes
   :module: ecflow

Returns a list of aliases

