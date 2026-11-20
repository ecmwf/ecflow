.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _user_names:

User names
//////////

ecFlow follows a single model for :term:`user names <user name>`, wherever a name is written or presented.

The model
=========

A user name is a non-empty string of valid UTF-8 (RFC 3629: no overlong form, no surrogate, no code point above
U+10FFFF) that:

- contains no white space, i.e. no character with the Unicode property ``White_Space``:

  .. list-table::
     :header-rows: 1

     * - Code points
       - Characters
     * - U+0009 to U+000D
       - tab, line feed, vertical tab, form feed, carriage return
     * - U+0020
       - space
     * - U+0085
       - next line
     * - U+00A0
       - no-break space
     * - U+1680
       - Ogham space mark
     * - U+2000 to U+200A
       - en quad to hair space
     * - U+2028, U+2029
       - line and paragraph separators
     * - U+202F
       - narrow no-break space
     * - U+205F
       - medium mathematical space
     * - U+3000
       - ideographic space

- contains no control character: U+0000 to U+001F, and U+007F to U+009F;
- contains none of ``#``, ``,``, ``:`` and ``;``;
- does not start with ``-``;
- is not ``*``.

Notes:

- Other characters are allowed, e.g. ``alice``, ``bob.smith``, ``x-y``, ``o'neil``, ``*bob``, or names written in
  any script.
- User names are compared exactly, byte for byte: ``alice`` and ``Alice`` are two users, and so are two names that
  differ only by their Unicode normalisation (e.g. a precomposed and a decomposed accented letter).

Where the model applies
=======================

The model applies to every user name ecFlow reads:

.. list-table::
   :header-rows: 1

   * - Source
     - When a name is outside the model
   * - Password files (``ECF_PASSWD``, ``ECF_CUSTOM_PASSWD``), see :ref:`black_list_file`
     - The file is rejected as a whole: at start-up, the server does not start; on ``--reloadpasswdfile`` or
       ``--reloadcustompasswdfile``, the reload fails and the previous content stays in force
   * - White list file (``ECF_LISTS``), see :ref:`ecflow_white_list_file`
     - The file is rejected as a whole: at start-up, the server does not start; on ``--reloadwsfile``, the reload
       fails and the previous content stays in force
   * - :ref:`Permissions attributes <permissions_attribute>`, in every form (definition files, server
       configuration, command line, Python, Rust)
     - The attribute is rejected
   * - The user name a client presents: the login name, ``ECF_USER``, ``--user``, ``Client.set_user_name`` (Python),
       ``Client::set_user_name`` (Rust), and the HTTP header ``X-Auth-Username``
     - The client refuses to send the request; a request that reaches the server all the same is refused by the
       authentication, ``--ping`` included
   * - User names given as arguments of commands, e.g. ``--ch_drop_user``
     - The request fails

In the white list file, ``-`` (read-only access) and ``*`` (every user) remain the syntax of the file, not part of a
name:

- ``-bob`` grants read-only access to ``bob``, and ``--bob`` is refused, since the name ``-bob`` starts with ``-``;
- ``*`` and ``-*`` name every user, while ``*bob`` names the user ``*bob``.

The password files have no such syntax: each entry names one user, and ``-bob`` or ``*`` is refused.

Error messages
==============

An error names the reason, and shows the name with its white space and control characters written as ``<U+XXXX>``,
and the bytes that are not valid UTF-8 as ``<0xXX>``, so that the name can be logged and shown safely. For a file,
the error also names the file and the line:

.. code-block:: text

  ECF_LISTS file /home/ecflow/host.3141.ecf.lists, line 4: invalid user name 'al<U+00A0>ice' contains white space (<U+00A0>)

.. code-block:: text

  Error: request( --ping ) failed! invalid user name 'al:ice' contains ':'
