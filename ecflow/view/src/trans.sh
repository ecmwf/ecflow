while read -r line ; do
    l=$(echo $line | sed -e 's:":\\":gi')
  echo "(char*) \" $l \","
done
echo NULL