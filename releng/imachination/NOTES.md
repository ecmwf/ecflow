

# Build reverse-proxy Docker image

```
docker build -t imachination_revproxy ./revproxy/
```

# Launch reverse-proxy

```
docker run --rm -i -t -p 80:80 -p 443:443 -v ./revproxy/server:/usr/share/nginx/html/server -v ./revproxy/cfgs/nginx/default.conf:/etc/nginx/conf.d/default.conf imachination_rp
```

# Launch auth-o-tron

```
docker run --rm -i -t -p 8080:8080 -v ./authotron/config.yaml:/app/config.yaml eccr.ecmwf.int/auth-o-tron/auth-o-tron-debug:0.2.1
docker run --rm -i -t -p 8080:8080 -v ./authotron/config.yaml:/app/config.yaml eccr.ecmwf.int/auth-o-tron/auth-o-tron-debug:0.2.3-amd64
docker run --rm -i -t -p 8080:8080 -v ./authotron/config.yaml:/app/config.yaml eccr.ecmwf.int/auth-o-tron/auth-o-tron-debug:0.2.4-amd64
```
> [!NOTE]
> Even though the reverse proxy access to auth-o-tron is internal to Docker (i.e., via referencing host.docker.internal:8080), the port 8080 still needs to be exported!

# Contacting ecFlow through the reverse proxy

The following simulates contacting ecFlow via the reverse proxy, triggering the authentication feedback loop:

## Using Basic authentication

```
BASIC_TOKEN=$(echo -n 'user:somesecret' | base64)
curl -v -k -X GET -H "Authorization: Basic ${BASIC_TOKEN}" https://localhost/v1/ecflow
```

## Using Bearer authentication

```
BEARER_TOKEN=$(cat ~/.ecmwfapirc | jq '.["key"]' --raw-output0)
curl -v -k -X GET -H "Authorization: Bearer ${BEARER_TOKEN}" https://localhost/v1/ecflow
```
