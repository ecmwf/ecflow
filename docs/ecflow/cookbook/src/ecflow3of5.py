#!/usr/bin/env python
""" ./ecflow3of5.py 63 /e_41r2/main/12/prod """
import ecflow
import os
import sys
import time
host = os.getenv("ECF_HOST", "localhost")
port = os.getenv("ECF_PORT", 31415)
client = ecflow.Client(host, port)
wait = False; wait = True
interval = 30
outof5 = int(sys.argv[1])
node_path = sys.argv[2]
def stop(msg, num): print msg; sys.exit(num)
while 1:
    tot = 0
    count = 0
    done = True
    client.sync_local()
    node = client.get_defs().find_abs_node(node_path)
    if node is None: stop("node not found!!!", 1)
    for item in node.nodes:
        count += 1
        status = "%s" % item.get_state()
        # print item.get_abs_node_path(), status, outof5, tot   
        if status == "complete":
            tot += 1
            if tot >= outof5: stop("# OK", 0)
        elif status == "aborted":
            pass
        else: done = False
    if count < outof5: stop("# Impossible: %d < %d" % (count, outof5), 1)
    if done: stop("# KO %d" % tot, 1)
    print "# still possible",
    if wait:
        print "...", tot, outof5, count
        time.sleep(interval);
    else: stop("", -1)