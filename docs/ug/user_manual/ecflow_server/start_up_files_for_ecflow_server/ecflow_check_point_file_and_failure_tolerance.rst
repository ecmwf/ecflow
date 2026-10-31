.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _ecflow_check_point_file_and_failure_tolerance:

ecFlow check point file and failure tolerance
/////////////////////////////////////////////

A checkpoint saves the workflow definitions and task states. The server can restore
this saved state after a server or host failure.

Checkpoint files
----------------

.. list-table::
   :header-rows: 1

   * - Setting
     - Purpose
   * - ``ECF_CHECK``
     - The main checkpoint file.
   * - ``ECF_CHECKOLD``
     - The backup from an earlier checkpoint save.

Recovery at startup
-------------------

The server chooses a checkpoint as follows:

.. list-table::
   :header-rows: 1

   * - Available files
     - Result
   * - ``ECF_CHECK`` can be loaded.
     - Restore the workflow from that file. The backup is not used.
   * - ``ECF_CHECK`` is missing or cannot be loaded, but ``ECF_CHECKOLD`` can be loaded.
     - Restore the workflow from the backup automatically; no manual copy is needed.
   * - Neither file exists.
     - Start with an empty workflow.
   * - At least one file exists, but neither can be loaded.
     - Refuse to start, report the filenames and errors, and leave both files unchanged.

Checkpoint files must be readable files containing a valid workflow definition.
A directory cannot be used as a checkpoint file.

After successful recovery:

- The server starts in ``HALTED`` state; job scheduling does not resume automatically.
- Workflow definitions and task states come from the chosen checkpoint.
- Server settings, such as ``ECF_HOME`` and ``ECF_PORT``, come from the current startup
  configuration rather than the saved server settings.

.. warning::

   Saved task states may be out of date. A task that was running when the checkpoint
   was saved may have completed since then. Check those tasks before restarting job scheduling.

Saving checkpoints
------------------

The server saves a new checkpoint in three steps:

#. Write the workflow to a temporary file beside ``ECF_CHECK``.
#. Move an existing non-empty ``ECF_CHECK`` to ``ECF_CHECKOLD``, replacing the old backup.
#. Rename the completed temporary file to ``ECF_CHECK``.

If writing the temporary file fails, the existing main checkpoint and backup remain unchanged.
The backup contains an older snapshot; changes made after that snapshot are not recovered
when starting from the backup.

Recovery on another system
--------------------------

Checkpoint files can be copied between systems, including systems with different
operating systems. A replacement ecFlow server can restore a copied checkpoint
to take over after a host failure.
