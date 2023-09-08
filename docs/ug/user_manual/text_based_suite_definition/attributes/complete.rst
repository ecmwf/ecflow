.. _complete:

complete
//////////

Force a node to be complete if the trigger evaluates, without running any of the nodes.

This allows the user to have tasks in the suite which run only in case others
fail. In practice the node would need to also have a trigger. A complete
enables making nodes **standby** depending on the success of other nodes.

Here is an example where task **tt** does not run if task **t** meter is more than 120. If task **t** however completes, but the meter **step** is less that 120 the **standby** job will run.

.. code-block:: shell

  family f
    task t
        meter step 0 240 120
    task tt
        complete t:step ge 120
        trigger t==complete
  endfamily


Using complete
================

The **complete** attribute can be used to **force** things to a complete
state if they are queued and the condition is met.

Here is an example when we have a task that decides that the whole
family should not run. As it has **repeat** in it, it will advance the
repeat to the next date and run that.

.. code-block:: shell
    :caption: Conditional complete

    suite x
    family f
        repeat date YMD 20120601 20200531
        complete ./f/check:nofiles
        task check
            event 1 nofiles
        task t1 ; trigger check==complete
        task t2 ; trigger t1==complete
    endfamily

Here is a python example where we create a simple reusable experimental
meteorological suite. There is a **configuration section** for the dates
and synoptic cycles to be selected, and there is a **function**
(add_complete()) to select the contents for the complete statement. This
is needed since you can only have one **complete** statement for any node. The main loop of the
suite is pretty straightforward.

.. code-block:: python

  class ExperimentalSuite(object):    
      def _init_(self,start,end) :
          self.start_ = start
          self.end_ = end
          self.start_cycle_ = 12
          self.end_cycle_ = 12
      
      def generate(self) :
          suite = Suite("x")
          make_fam = suite.add_family("make")
          make_fam.add_task("build")
          make_fam.add_task("more_work")
          main_fam = suite.add_family("main")
          main_fam.add_repeat( RepeatDate("YMD",self.start_,self.end_) )
          main_fam.add_trigger( "make == complete" )
          previous = 0
          for FAM in ( 0, 6, 12, 18 ) :
              fam_fam = suite.add_family(str(FAM))
              if FAM > 0 :
                  fam_fam.add_trigger( "./" + str(previous) + " == complete " )
              self.add_complete(fam_fam,FAM)
              fam_fam.add_task("run")
              fam_fam.add_task("run_more").add_trigger( "run == complete")
              previous = FAM
          return suite
      
      def add_complete(self,family,fam):
          if fam < self.start_cycle_ and fam > self.end_cycle_ :
              family.add_complete( "../main:YMD eq " + str(self.start_) + " or ../main:YMD ge " + str(self.end_) )
          elif fam < self.start_cycle_ :
              family.add_complete( "../main:YMD eq " + str(self.start_) )
          elif fam > self.end_cycle_ :
              family.add_complete( "../main:YMD ge " + str(self.end_) )
          return

  print str( ExperimentalSuite(20120601,20120605).generate() )
