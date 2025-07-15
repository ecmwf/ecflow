

# reverse-proxy

## Build 'reverse-proxy' Docker image

```
docker build -t imachination_revproxy ./revproxy/
```

## Launch 'reverse-proxy'

```
docker run --rm -i -t -p 80:80 -p 443:443 -v ./revproxy/server:/usr/share/nginx/html/server -v ./revproxy/cfgs/nginx/default.conf:/etc/nginx/conf.d/default.conf imachination-revproxy
```

# auth-o-tron

## Launch auth-o-tron

```
docker run --rm -i -t -p 8080:8080 -v ./authotron/config.yaml:/app/config.yaml eccr.ecmwf.int/auth-o-tron/auth-o-tron-debug:0.2.1 imachination-authotron
```
> [!NOTE]
> Even though the reverse proxy access to auth-o-tron is internal to Docker (i.e., via referencing host.docker.internal:8080), the port 8080 still needs to be exported!

# ecflow

## Build 'ecflow' deb package within builder Docker image

```
docker build -t imachination_ecflow_builder ./ecflow-builder/
```

## Launch 'ecflow' builder

ecflow builder is a Docker image capable of building ecFlow from source.
The following command will launch the builder and start a shell inside it:

```
docker run --rm -i -t -v $(pwd)/ecflow:/workspace/ecflow -v $(pwd)/ecbuild:/workspace/ecbuild imachination_ecflow_builder /bin/sh -c "cd /workspace/ecflow && /bin/bash"

To build ecFlow, run the following commands inside the builder:

cd /workspace/ecflow

# check the available presets
cmake --list-presets

# configure
cmake --preset linux.gcc.serveronly.relwithdebinfo

# build
cmake --build --preset linux.gcc.serveronly.relwithdebinfo --target all

# install 
cmake --build --preset linux.gcc.serveronly.relwithdebinfo --target install

# package
cmake --build --preset linux.gcc.serveronly.relwithdebinfo --target package

# make the package available in the current directory
mv /workspace/ecflow/build/linux.gcc.serveronly.relwithdebinfo/ecflow-*.tar.gz ./
```
# Deploy 'ecflow' deb package within runner Docker image

## Build 'ecflow' runner Docker image

```
docker build -t imachination_ecflow_runner ./ecflow-runner/
```

## Launch 'ecflow' runner

```
docker run --rm -i -t -p 3141:3141 -v $(pwd):/workspace imachination_ecflow_runner"
```

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
