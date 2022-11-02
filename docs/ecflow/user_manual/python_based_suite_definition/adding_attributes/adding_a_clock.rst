.. _adding_a_clock:

Adding a Clock
//////////////

.. code-block:: python

    suite = ecflow.Suite("suite") # create a suite
    clock = ecflow.Clock(1, 1, 2010, False)  # day, month, year, hybrid
    clock.set_gain(1, 10, True)  # hour, minutes, bool(true ~positive gain )
    suite.add_clock(clock)

    clock = ecflow.Clock(1, 1, 2010, True)  # day, month, year, hybrid
    clock.set_gain_in_seconds(300, True)
    s1 = ecflow.Suite("s1", clock)  # create a different suite
