release=$(cat ACore/src/ecflow_version.h | grep 'ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat ACore/src/ecflow_version.h   | grep 'ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat ACore/src/ecflow_version.h   | grep 'ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
ecflow_version=$release.$major.$minor

# use tr -d '\12' to remove trailing newline
echo "$ecflow_version" | tr -d '\12'
