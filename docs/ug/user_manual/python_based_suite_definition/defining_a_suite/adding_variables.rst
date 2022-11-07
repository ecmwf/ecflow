.. _adding_variables:

Adding Variables
////////////////////////////////////////////////

.. code-block:: python
   :caption: Use key value arguments on node creation

   from ecflow import Defs, Suite, Variable, Edit

   defs = Defs(
      Suite("s1", HELLO="world", FRED="bloggs", BILL=1, NAME="value", NAME2="value2")
   )
   defs.s1.add_variable("NAME4", 4)
   defs.s1 += Edit(NAME3="value3")

The following examples show alternative styles that produce the same definition:

.. code-block:: python
   :caption: Using add() with Edit

   defs = Defs().add(
      Suite("s1").add(
         Edit(
               HELLO="world",
               FRED="bloggs",
               BILL=1,
               NAME="value",
               NAME2="value2",
               NAME3="value3",
               NAME4=4,
         )
      )
   )

.. code-block:: python
   :caption: Using += with dictionary

   defs = Defs() + Suite("s1")
   defs.s1 += {
      "HELLO": "world",
      "NAME": "value",
      "NAME2": "value2",
      "NAME3": "value3",
      "NAME4": 4,
      "BILL": 1,
      "FRED": "bloggs",
   }


.. code-block:: python
   :caption: Using Edit() on node creation and += with list

   defs = Defs() + Suite("s1", Edit(HELLO="world"))
   defs.s1 += [
      Edit({"NAME": "value", "NAME2": "value2", "NAME3": "value3", "NAME4": 4}, BILL=1),
      Edit(FRED="bloggs"),
   ]

.. note::

   Although we are using class Edit as a short cut, the objects that are added are still of type Variable.

.. warning::

   In the example above we use 'defs.s1' to reference a node by name.
   This is useful in small designs but will produce maintenance issues in large designs IF the node names are changed.
