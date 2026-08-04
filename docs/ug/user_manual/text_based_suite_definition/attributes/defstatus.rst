.. _defstatus:

defstatus
/////////

A **defstatus** is a :term:`node` attribute, that determines the default state assigned to the node
when it *begins* or is *requeued*.


Unless explicitly defined by the user, the default state of a node is :term:`queued`.

The values for **defstatus** are:

- :term:`unknown`
- :term:`complete`
- :term:`queued`
- :term:`aborted`
- :term:`submitted`
- :term:`active`
- :term:`suspended`

The **defstatus** value :term:`suspended` is particular, in the sense that, at begin/re-queue time, it sets the
node to state :term:`queued` but requires an explicit resume instruction by the user to eventually allow the
node to move to the :term:`submitted` state (the node is effectively *suspended* in the meantime).

A **defstatus** is useful to prevent suites from running automatically upon beginning a suite, or to
assign tasks with :term:`complete` state so they can be run selectively.

For a :term:`suite` or a :term:`family`, the meaningful **defstatus** values are :term:`queued`,
:term:`complete` and :term:`suspended`:

- :term:`queued` is the default. The state of the node reflects the state of its children.
- :term:`complete` is propagated down the hierarchy, so that the node and all its children are marked
  complete without being run.
- :term:`suspended` holds the node, and thereby its children, until the node is explicitly resumed.

The remaining values are accepted, but have no lasting effect on a :term:`suite` or a :term:`family`: the
state is assigned to the node alone, is not propagated to the children, and is replaced as soon as any of
the children changes state.

.. code-block:: shell

    family f
        task t1
        task t2
            defstatus complete # by default will not be run
        task t3
            defstatus suspended # needs 2 B resumed

Relation to the suspend command
===============================

A :term:`node` can be suspended in two distinct ways: by means of the **defstatus** attribute, or by means
of the :code:`--suspend` command. Both result in a suspended node, but they act on different node
properties, and are therefore independent of each other.

.. list-table::
   :header-rows: 1

   * -
     - :code:`defstatus suspended`
     - :code:`--suspend`
   * - Effect
     - Defines the default state, applied when the node *begins* or is *requeued*
     - Suspends the node immediately
   * - Acts on
     - The **defstatus** attribute, which is part of the :term:`suite definition`
     - The node internal suspend property, part of the node state
   * - Persistence
     - Restored whenever the definition file is loaded
     - Retained in the :term:`check point` file, but is not part of the definition
   * - Reverted by
     - Assigning a different **defstatus** value
     - The :code:`--resume` command

Since the two act on different properties, neither supersedes the other, and the order in which they are
issued is irrelevant.

.. important::

   The :code:`--resume` command does not alter the **defstatus** attribute.

   A node with **defstatus** :term:`suspended` is suspended again the next time it *begins* or is
   *requeued*, even after having been explicitly resumed. To prevent this, the **defstatus** attribute
   itself must be changed, for example:

   .. code-block:: shell

       ecflow_client --alter change defstatus queued /s/f/t

The following example illustrates the difference:

.. code-block:: shell

    # Suspend the node immediately; the effect lasts until the node is resumed
    ecflow_client --suspend /s/f/t

    # Define the node to be suspended whenever it begins or is requeued;
    # the current state of the node is not affected
    ecflow_client --alter change defstatus suspended /s/f/t
