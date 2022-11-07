.. _searching_for_nodes:

Searching for nodes
///////////////////

ecFlowUI has a built-in fast, powerful engine to search for nodes. To
activate the search dialogue, click the **Search** button on the
toolbar, or choose **Tools >Search** from the main menu.
Alternatively, right-click on a node in the tree and select **Search**
from the context menu - the search will be performed on that node and
its descendants.

.. image:: /_static/ecflow_ui/searching_for_nodes/image1.png
   :width: 4.15101in
   :height: 0.76845in

The search dialogue is split into three parts - the editor, the
generated query and the results. The first two parts can be hidden in
order to give more space to the results

.. image:: /_static/ecflow_ui/searching_for_nodes/image2.png
   :width: 3.95833in
   :height: 4.48591in

The **Scope** section allows you to specify which server(s) to restrict
the search to. ALL means that all servers in the current tab will be
searched. The root node for the search can also be set here - only that
node and its descendants will be searched.

The area below the **Scope** section contains *two tabs* to specify the
search criteria for *nodes* and *attributes* respectively. By default,
nodes are searched for, not attributes. If any attribute options are
specified, ecFlowUI will search for matching attributes belonging to the
nodes which match the options in the **Nodes** tab.


* Node options:

   * The **Nodes** tab by default specifies no filtering, and thus will find all nodes. The search can be limited to a set of nodes specified through the **Name** and **Path** options.
   * The **Type** selection box by default has no entries checked. This means that the type of the node will not be checked, so nodes of all types may be found. Checking the node type boxes restricts the search to only those node types.
   * The **Status** selection box works in a similar way to the **Type** selection box.
   * The **Flag** box is also similar - if nothing is selected, then nodes will be found regardless of their flags; otherwise, the search will be restricted to just those nodes that contain the specified flags.

* Attribute options: The **Attributes** tab defines the options used for attributes search. If nothing is selected, no attributes are searched. By selecting an attribute from the list in the left, the search options available for this this attribute will appear in the right hand side.


   .. image:: /_static/ecflow_ui/searching_for_nodes/image3.png
      :width: 3.33333in
      :height: 1.45348in

Both the **Nodes** and **Attributes** tabs contain search options where
the search syntax can be set interactively using the methods listed
below:

.. note::

   **String match options**                                           
                                                                       
   *  **Matches** is a wildcard match, similar to UNIX file syntax, where "*" means "any number of any characters"  and "?" means "one of any character", e.g. "*monitor?*". If no wildcard characters are used, the result will be that only those nodes hose names/paths exactly match the string will be found. This is often the simplest way of searching. See http://doc.qt.io/qt-5qregexphtml#wildcard-matching. 
                                                      
   *  **Regexp** is a regular expression match, using the Qt regexp syntax. The regexp equivalent of the above example would be ".*monitor.". See http://doc.qt.io/qt-5/qregexp.html#introduction.                
                                                                       
   * **Contains** finds nodes whose names/paths *contain* the given regular expression as a substring; similar to   **Regexp**, but an find more nodes since the regexp only needs to match a substring rather than the whole name                            

The **Query** section shows the query which is generated from the
selections in the dialogue. It is presented using a MySQL-like syntax
for the sake of easier interpretation.

Once the options have been set, click the **Search** button. The results
panel will be populated with the found nodes and attributes. If only
node properties have been specified in the query, the results will show
one found node per row. If attribute properties have been specified, the
results will show one found attribute per row; if a node contains
multiple attributes which match the query (e.g. multiple variables),
that node would appear multiple times in the search results, once for
each matching attribute.

.. image:: /_static/ecflow_ui/searching_for_nodes/image4.png
   :width: 3.95833in
   :height: 3.46897in

The nodes have the normal context menu, allowing you to perform actions
on them straight from the results box. Multiple node selection is
allowed. Clicking on a node here also selects it in the node tree.
Currently there is no context menu assigned to the attributes.

.. note::

    An important thing to note about the search results is that the    
    found nodes will be updated when ecFlowUI syncs with the server.   
    The list of found nodes will not be changed, but their latest      
    state will be reflected.                                           

.. image:: /_static/ecflow_ui/searching_for_nodes/image3.png
   :width: 3.33333in
   :height: 1.45348in
