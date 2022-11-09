.. _tutorial-operational-suite:

Operational Suite
-----------------

* The operational suite contains two cycles named 00 and 12.
* Each cycle contains three parts:
     o Analysis
        + fetch the observations,
        + run the analysis model,
        + post-process the data 
     o Forecast
        + Prepare the input data
        + Run the forecast model. The forecast model outputs its results every 6 hours up to 24 hours for cycle 00 and up to 240 hours for cycle 12. 
     o Archive
        + Save the analysis results.
        + Save the forecasts results as they become available. 

What do:

1. Write a :term:`suite definition` for this suite.
2. How should the :term:`ecf script`\ s be organised? 


Useful pointers:

    * :ref:`tutorial-add-trigger`  
    * :ref:`tutorial-add-meter` 
    * :ref:`tutorial-add-variable` 
    * :ref:`tutorial-using-python-scripting`
    * :ref:`tutorial-file-location` for ECF_FILES  
    * :ref:`python_based_suite_definition`
    
One possible :ref:`tutorial-operational_suite_soln`
    