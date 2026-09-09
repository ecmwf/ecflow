.. _licence:

Licence
///////

Copyright 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)

ecFlow is licensed under the Apache Licence, Version 2.0. The complete text is
distributed with the source, as ``LICENSE`` at the root of the repository and,
in the unmodified form expected by licence tooling, as
``LICENSES/Apache-2.0.txt``. It is also published at
https://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, the software is
distributed on an "AS IS" basis, without warranties or conditions of any kind,
either express or implied. The Licence governs the specific language for
permissions and limitations. Further explanation is given in the `Apache
License and Distribution FAQ
<https://www.apache.org/foundation/license-faq.html>`__.

In applying this licence, ECMWF does not waive the privileges and immunities
granted to it by virtue of its status as an intergovernmental organisation nor
does it submit to any jurisdiction.

Third-party software included in the source tree retains the licensing of its
origin. Each such component is recorded in ``NOTICE``, together with the
licence under which it is distributed.

Licensing metadata
==================

Every source file declares its copyright holder and licence through two SPDX
tags, rather than repeating the licence notice in full::

    SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
    SPDX-License-Identifier: Apache-2.0

Files that cannot carry a comment, such as images and test fixtures compared
byte for byte, are covered by ``REUSE.toml`` instead. The repository follows the
`REUSE Specification <https://reuse.software/spec/>`__, and compliance is
verified with ``reuse lint``.

The convention that applies to new files is described in :ref:`contributing`.
