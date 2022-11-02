.. _template_suite:

Template suite
//////////////

Few questions were about starting a project from scratch, creating a
"template suite", or a suite skeleton, where to define tasks targeting
one destination or another at the centre.It generates a simple task
wrapper and link the trap.h from emos.

It is also common to design a suite with several families working at
different frequency (daily, monthly, yearly). This template provides
such structure as an example.

Another expectation from this example is to show that "action" attribute
was not preserved with ecFlow, while it can be simply replaced with a
one liner task. This use case is not advocated: it can be attractive for
simple "one liner" command, but error message may not be clear, would
there be any, "when things go wrong"

.. literalinclude:: src/ecflow_suite.py
   :language: python    


This script can be run from emos def:

.. code-block:: shell
  :caption: load suite

  python /home/ma/emos/def/o/admin/ecflow_suite.py -s suite -p $(($(id -u) + 1500)) -n localhost -r


.. image:: /_static/template_suite/image1.png
   :width: 2.67639in
   :height: 2.60417in

Another example would be for seasonal suites, where forecast is on the
first of the month, run few days after (here the 4th, in this example),
where families may have to work with previous month date.

.. code-block:: python

    Family("seas").add(
        Label("info", "micro as tilde"),
        Variables(ECF_MICRO= "~"),
        Repeat(name="YM", start=
              [ "%d" % ( YM)
                for YM in range(201601, 203212+1)
                if (YM % 100) < 13],
              kind="enum"),
        Family("assim").add(
            Variables(YMD= "$(if [[ ~YM~ = *01 ]]; then echo $((~YM~ - 89)); else echo $((~YM~ -1)); fi)01", ),
            Label("info", "back one month"),
            Task("impossible").add(Trigger("1==0")), # replace me
        ),
        Family("fc").add(
            Variables(YMD= "~YM~01"),
            Trigger("assim eq complete"),
            Task("impossible").add(Trigger("1==0")), # replace me
        ),
        Family("hold").add(
            Time("10:00"),
            Date("4.*.*"),
            Task("always").add(
                Trigger("1==0"),
                Complete("1==1")),
        ), 


.. image:: /_static/template_suite/image2.png
   :width: 5.55278in
   :height: 2.51111in
