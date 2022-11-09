.. _cron_repeat_day_1_repeat_date:

Cron, repeat day 1, repeat date
///////////////////////////////

Attributes cron, repeat day 1 and repeat date can be used to get a suite running in real time mode.

- :term:`cron` is essentially restricted to "house cleaning" tasks, while it gets the task requeued as soon as it completes.No trigger can be expected from such task. It is used in operation to get nodes recording a day and time is achieved so they can alter a variable that is used in a trigger in a "user family". That way, there is no need for persistent event or meter for cron. As an example, /admin/times is memorising that the milestone is achieved, and /mofc/thu/01/ref is the user family where a trigger is defined: that way we avoid cron dependency directly into the functional suite.


   .. image:: /_static/cron_repeat_day_1_repeat_date/image1.png
      :width: 4.48889in
      :height: 1.22917in

   .. code-block:: shell
      :caption: Admin suite definition

      node=/%SUITE%/%FAMILY%/%TASK%
      %nopp
      ecflow_client --alter change variable LAST_YMD "$(date +%Y%m%d)" $node
      ecflow_client --alter change variable LAST_HMS "$(date +%H%M%S)" $node
      %end


   .. image:: /_static/cron_repeat_day_1_repeat_date/image2.png
      :width: 7in
      :height: 1.38333in

   .. code-block:: shell
      
      node=/%SUITE%/%FAMILY%/%TASK%
      %nopp
      ecflow_client --alter change variable LAST_YMD "$(date +%Y%m%d)" $node
      ecflow_client --alter change variable LAST_HMS "$(date +%H%M%S)" $node
      %end



   Be careful that cron may prevent an **inherited repeat attribute** to loop as
   expected. We use it with a **complete attribute** when we have to (
   ).

- **repeat day 1** is used in few suites, to progress each day. day is incremented once all tasks are complete, i.e. there must be a task with a time attribute to ensure the suite is not looping before a given time. Additionally, we may appreciate to have a task which turns aborted when few families/task remain queued for some reason that would prevent the suite to loop.

   .. image:: /_static/cron_repeat_day_1_repeat_date/image3.png
      :width: 4.26389in
      :height: 2.60417in

   .. code-block:: shell
      :caption: repeat day 1

      suite mars
      repeat day 1
      family last
         task logfiles
            time 16:00
         task date
            complete ../stage eq complete and ../check eq complete and ../sapp eq complete and ../statistics eq complete
            edit ECF_TRIES '1'
            time 16:00
      endfamily
      # ...

- **repeat date** is the most frequently used, while it is the most convenient to run a suite in **real-time mode** and in             **catchup-mode.** repeat is incremented once **all** families/tasks below are complete. A task with a time attribute will prevent the increment before a given time. In Catchup-mode, **defstatus complete attribute** will inhibit the time dependency                                                    

   .. image:: /_static/cron_repeat_day_1_repeat_date/image4.png
      :width: 3.13611in
      :height: 1.5625in

   .. code-block:: shell
      :caption: repeat date

      family main
      repeat date YMD 20151118 20201212 1
      family ref
         task dummy
            # defstatus complete # uncomment to shift to catchup-mode
            time 12:10
      # ...      

