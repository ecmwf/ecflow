.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _open_ssl:

Open SSL
////////

OpenSSL enables encrypted communication between the client and the
server. For ecFlow, this can be used for user commands.

To enable this for ecFlow 4, please ensure you build ecFlow with
'-DENABLE_SSL'. You will need to ensure that open SSL is installed on
your system. This is **enabled by default for ecFlow 5 if** the SSL libraries are
found on the system.

.. code-block:: shell
   :caption:  Check if openssl enabled for ecflow

   ecflow_client --version # look for a string openssl
   ecflow_server --version # look for a string openssl

Certificates
===============

SSL communication requires matching self-signed certificates.

ecFlow loads certificates from ``$HOME/.ecflowrc/ssl/`` by default.
``ECF_SSL_DIR`` overrides the directory for the current process.
A trailing ``/`` is optional:

.. code-block:: shell

   export ECF_SSL_DIR="/path/to/certificates"

Enable SSL with ``ECF_SSL`` or ``--ssl`` for the CLI and server. In the UI,
select **TCP/IP with SSL** for each server.

Server-specific certificates are recommended. Set ``ECF_SSL=X`` for the CLI
and server to try ``<host>.<port>.crt`` first, falling back to ``server.crt`` if
it is absent. The UI always uses this selection mode, independently of ``ECF_SSL``.

Shared certificates should be avoided. Use ``ECF_SSL=1`` only when a shared
certificate is intentional. Its existing shared-first behaviour is retained
for compatibility. Certificate selection within the directory:

.. list-table::
   :header-rows: 1
   :widths: 30 20 25 25

   * - Program
     - ``ECF_SSL``
     - First certificate
     - Fallback if absent

   * - ``ecflow_client`` / ``ecflow_server``
     - Any other value, such as ``X``
     - ``<host>.<port>.crt``
     - ``server.crt``

   * - ``ecflow_client`` / ``ecflow_server``
     - ``1`` or empty
     - ``server.crt``
     - ``<host>.<port>.crt``

   * - ``ecflow_ui``
     - Ignored
     - ``<host>.<port>.crt``
     - ``server.crt``

``ECF_SSL_DIR`` does not enable SSL. ``ECF_SSL`` values select a mode,
not a certificate path.

If the selected certificate cannot be loaded or fails verification, the
client reports an error without trying another certificate.

The files expected in the selected directory depend on the certificate
that is selected:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - File
     - Server-specific certificate
     - Shared certificate

   * - Certificate (server and client)
     - ``<host>.<port>.crt``
     - ``server.crt``

   * - Private key (server)
     - ``<host>.<port>.key``
     - ``server.key``

   * - DH parameters (server)
     - ``<host>.<port>.pem``
     - ``dh2048.pem`` (``dh1024.pem`` if absent)

   * - Passphrase (server, optional)
     - ``<host>.<port>.passwd``
     - ``server.passwd``

Notes:

-  The client requires only the certificate, which must be identical
   to the certificate of the server.

-  The passphrase file, if present, must contain the passphrase used
   to create the private key.


How to create certificates?
============================

The following steps create the files of a shared certificate. For a
server-specific certificate, use ``<host>.<port>`` instead of ``server``
and ``dh2048`` in every filename, for example ``myhost.3141.key``,
``myhost.3141.crt`` and ``myhost.3141.pem``.

-  Generate a password-protected private key. This will request a
   passphrase.

   Consider using a 2048 bit RSA key, encrypted using Triple-DES
   and stored in a PEM format so that it is readable as ASCII text.

   .. code-block:: shell
      :caption: Password protected private key

      openssl genrsa -des3 -out server.key 2048

   .. warning::

      Although it was previously suggested to use 1024 bit RSA keys,
      when using OpenSSL with TLS Security Level set to 2 (typical of recent OSes)
      these keys are regarded as unsecure and therefore are automatically rejected.

      The use of 2048 bit RSA keys is *strongly* recommended.

-  If you want additional security. Create a file called
   **'server.passwd'** and add the passphrase to the file. Then set the
   file permission so that the file is only readable by the server
   process.

   **Or** you can choose to remove the password requirement. In that
   case, we don't need **server.passwd** file.

   .. code-block:: shell
      :caption: Remove password requirement

      cp server.key server.key.secure
      openssl rsa -in server.key.secure -out server.key

-  Sign a certificate with a private key (self-signed certificate).
   Generate Certificate Signing Request(CSR).

   .. warning::

      This will prompt a number of questions. However please ensure
      '**common name**' **matches** the host where your server is going to
      run.

   .. code-block:: shell
      :caption: Generate Certificate Signing Request(CSR)

      openssl req -new -key server.key -out server.csr


-  generate a self-signed certificate CRT, by using the CSR and private
   key.

   .. code-block:: shell
      :caption: Sign the certificate server.crt must be accessible by client and server

      openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt

-  Generate dhparam file. ecFlow expects 2048 key.

   .. code-block:: shell

      openssl dhparam -out dh2048.pem 2048
