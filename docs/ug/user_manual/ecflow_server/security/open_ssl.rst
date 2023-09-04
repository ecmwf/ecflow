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

In order to use OpenSSL, we need to set up some certificates. (These
will be self-signed certificates, rather than a certificate authority).

The ecFlow client and server will look for the certificates in
**$HOME/.ecflowrc/ssl** directory.

ecFlow server expects the following files in : **$HOME/.ecflowrc/ssl**

-  dh2048.pem

-  server.crt

-  server.key

-  server.passwd (optional) if this exists it must contain the
   passphrase used to create server.key.

ecFlow client expects the following files in: **$HOME/.ecflowrc/ssl**

-  server.crt ( this must be the same as the server)


How to create certificates?
============================

The following steps, show you how to create these files:

-  Generate a password-protected private key. This will request a
   passphrase.

   Consider using a 2048 bit RSA key, encrypted using Triple-DES
   and stored in a PEM format so that it is readable as ASCII text.

   .. code-block:: shell
      :caption: Password protected private key

      openssl genrsa -des3 -out server.key 2048

   .. warning::

      Although it was previously suggested to use 1024 bit RSA keys,
      when using OpenSSL 1.1.1 these keys are regarded as unsecure
      and therefore are automatically rejected.
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
