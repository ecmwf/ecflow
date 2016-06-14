try:
   ci.child_complete()
   print("Sent complete")
except:
   print("complete aborted")
   ci.child_abort("Abort in tail")

