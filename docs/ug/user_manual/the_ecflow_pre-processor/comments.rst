.. _comments:

Comments
////////

Comments are enclosed between pre-processor lines **%comment** and
**%end** . These lines are then ignored when the file is being
pre-processed, either for running the job or for the manual-page. You cannot, however,
**comment-out** a part of the manual page.

Since comments are processed before a job is created they can be placed
anywhere in the script. Just remember that comments do not **nest** and
that you cannot use comments inside manual pages. (You can, of course, change the
**%manual** to be **%comment** )

The following extract is an example of using comments:

.. code-block:: shell
    
    mars << EOF
        RETRIEVE,
        PARAM=10U/10V,DATE=...,
    %comment
        temp mod by OP / 1.1.2012 BC
        TARGET="/xx/yy/zz",
    %end
        TARGET="zz",
        END
        EOF
        And the following is the corresponding output when pre-processed.
        mars << EOF
        RETRIEVE,
        PARAM=10U/10V,DATE=...,
        TARGET="zz",
        END
    EOF
