ecflow.Label
////////////


.. py:class:: Label
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

A :term:`label` has a name and value and provides a way of displaying information in a GUI.

The value can be anything(ASCII) as it cannot be used in triggers.
The value of the label is set to be the default value given in the definition
when the :term:`suite` is begun. This is useful in repeated suites: A task sets the label
to be something.

Labels can be set at any level: Suite,Family,Task.
There are two ways of updating the label

- A :term:`child command` can be used to automatically update the label on a :term:`task`
- Using the alter command, the labels on :term:`suite` :term:`family` and :term:`task` can be changed manually

Constructor::

   Label(name,value)
      string name:  The name of the label
      string value: The value of the label

Usage:

.. code-block:: python

   t1 = Task('t1',
             Label('name','value'),  # create Labels in-place
             Label('a','b'))
   t1.add_label('l1','value')
   t1.add_label(Label('l2','value2'))
   for label in t1.labels:
      print(label)


.. py:method:: Label.empty( (Label)arg1) -> bool :
   :module: ecflow

Return true if the Label is empty. Used when returning a NULL Label, from a find


.. py:method:: Label.name( (Label)arg1) -> str :
   :module: ecflow

Return the :term:`label` name as string


.. py:method:: Label.new_value( (Label)arg1) -> str :
   :module: ecflow

Return the new label value as string


.. py:method:: Label.value( (Label)arg1) -> str :
   :module: ecflow

Return the original :term:`label` value as string

