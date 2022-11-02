.. _task:

task
////

This keyword defines a new task inside a family or suite. The only
parameter that this keyword takes is the name of the new task to be
defined. This keyword acts as an implicit **endtask** for the previous
task.

Only tasks can be submitted. A job inside a task script should generally
be re-entrant so that no harm is done by rerunning it, since a task may
be automatically submitted more than once if it aborts.

The ability to restart is important. The idea of using ecFlow is to
divide a real problem into small manageable parts each handled by a
separate task or family. On occasion, especially when events are used, a
task should be able to start from the point at which a previous job
finished. Events should be sent only once, although there is no harm in
sending them more than once. A typical example of a large real problem
is a weather forecast which may take several hours to run,
check-pointing itself from time to time. If the forecast fails and is
restarted, it can determine how far it had already progressed and
continued from where it left off.
