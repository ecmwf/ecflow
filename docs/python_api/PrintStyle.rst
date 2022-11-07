ecflow.PrintStyle
/////////////////


.. py:class:: PrintStyle
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Singleton used to control the print Style. See :py:class:`ecflow.Style`


Usage::

   old_style = PrintStyle.get_style()
   PrintStyle.set_style(PrintStyle.STATE)
   ...
   print(defs)                     # show the node state
   PrintStyle.set_style(old_style) # reset previous style


.. py:method:: PrintStyle.get_style() -> Style :
   :module: ecflow
   :staticmethod:

Returns the style, static method


.. py:method:: PrintStyle.set_style( (Style)arg1) -> None :
   :module: ecflow
   :staticmethod:

Set the style, static method

