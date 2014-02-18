ecflow_version=$(cat VERSION.cmake | awk '{print $3}'|sed 's/["]//g' )

# use tr -d '\12' to remove trailing newline
echo "$ecflow_version" | tr -d '\12'
