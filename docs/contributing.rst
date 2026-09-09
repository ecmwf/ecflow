.. _contributing:

Contributing
------------

The main repository is hosted on GitHub, testing, bug reports and contributions are highly welcomed and appreciated:

https://github.com/ecmwf/ecflow

Code contributions on ECMWF GitHub space should follow the standard fork-based contribution model on GitHub, which ends with opening of a pull request. 
Any contribution should follow these steps:

- Fork the develop branch of the targeted package from GitHub to your own GitHub space
- Clone your fork locally
- Make the necessary code changes & add and run tests to ensure the new codes works as intended
- Push changes back to fork on GitHub
- Create a pull request (PR) back to ECMWF:
   * Describe the motivation of the change and impact on code
   * Accept the ECMWF Contributors License Agreement (CLA - see below for more information)
   * Make sure that all requirements of the PR are addressed
- As soon as all conditions are fulfilled an ECMWF staff member will review the PR and either merge the request or comment on the PR

Licensing of new files
~~~~~~~~~~~~~~~~~~~~~~

Every new file that can carry a comment declares its copyright holder and
licence through two SPDX tags, placed at the top of the file in the comment
syntax of that file::

    SPDX-FileCopyrightText: <year>- European Centre for Medium-Range Weather Forecasts (ECMWF)
    SPDX-License-Identifier: Apache-2.0

Where a file begins with an interpreter line, or with an interpreter line and
an encoding declaration, those remain the first lines and a blank line
separates them from the tags. A blank line follows the tags.

A file co-developed with another organisation names each holder on its own
``SPDX-FileCopyrightText`` line, above a single licence identifier.

Files that cannot carry a comment are covered by an entry in ``REUSE.toml``
instead. Third-party files retain the header of their origin and are never
re-stamped. Compliance is verified with ``reuse lint``.

Also see :ref:`licence`
