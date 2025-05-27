
FROM debian:12.7-slim

RUN apt-get update \
    && apt-get install -y \
        nginx \
        openssl  \
    && rm -rf /var/lib/apt/lists/*

RUN openssl req \
    -x509 \
    -nodes \
    -days 365 \
    -subj "/C=UK/ST=QC/O=Company/CN=localhost" \
    -addext "subjectAltName=DNS:mydomain.com" \
    -newkey rsa:2048 \
    -keyout /etc/ssl/private/nginx-selfsigned.key \
    -out /etc/ssl/certs/nginx-selfsigned.crt

ENTRYPOINT ["nginx", "-g", "daemon off;error_log /dev/stdout info;"]
