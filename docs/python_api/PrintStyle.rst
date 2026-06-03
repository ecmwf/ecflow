ecflow.PrintStyle
/////////////////


.. py:class:: PrintStyle
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Singleton used to control the print Style. See :py:class:`ecflow.Style`


Usage::

   old_style = PrintStyle.get_style()
   PrintStyle.set_style(PrintStyle.STATE)
   ...
   print(defs)                     # show the node state
   PrintStyle.set_style(old_style) # reset previous style


.. py:method:: PrintStyle.get_style() -> ecflow.Style
   :module: ecflow
   :staticmethod:

Returns the style, static method


.. py:method:: PrintStyle.set_style(arg0: ecflow.Style) -> None
   :module: ecflow
   :staticmethod:

Set the style, static method

