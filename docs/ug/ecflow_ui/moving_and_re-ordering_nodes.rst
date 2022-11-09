.. _moving_and_re-ordering_nodes:

Moving and re-ordering nodes
////////////////////////////


ecFlowUI allows some degree of changes to be made to the structure of
suites once they are running on servers.

Re-ordering nodes
=================

The **Order** sub-menu in the node context menu allows for nodes to be
re-ordered with various options. Suppose that part of the node hierarchy
has a suite containing four families: s/ f1,f2,f3,f4. This table shows
what would happen when each operation is performed on node f3 (assume
the nodes are reset to their initial state before each operation) :

+----------------------+-----------------------------------------------+
| Operation            | Result when performed on f3                   |
+======================+===============================================+
| **(Initial state)**  | s/ f1,f2,f3,f4                                |
+----------------------+-----------------------------------------------+
| **Up**               | s/ f1,f3,f2,f4                                |
+----------------------+-----------------------------------------------+
| **Down**             | s/ f1,f2,f4,f3                                |
+----------------------+-----------------------------------------------+
| **Top**              | s/ f3,f1,f2,f4                                |
+----------------------+-----------------------------------------------+
| **Bottom**           | s/ f1,f2,f4,f3                                |
+----------------------+-----------------------------------------------+
| **Alphabetically**   | s/ f1,f2,f3,f4                                |
+----------------------+-----------------------------------------------+

Moving nodes
============

Nodes can be re-parented and even moved to different servers. In order
to allow nodes to be moved between servers displayed in different tabs
and windows, the following approach is taken. First, select a node which
is to be moved. From the context menu, select **Special > Mark for
move**. If it is not already suspended, ecFlowUI will suspend it for
you. Now select the node that will be its new parent. From the context
menu, select **Special > Move marked node here**.

Note that there are some combinations that will not work, e.g. moving a
suite into another suite. This operation should be performed with
caution, as the moved nodes may have dependencies which are not met by
its new location.
