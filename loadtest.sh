#!/bin/bash
COUNTER=0
while [ $COUNTER -lt 5000 ]; do
    curl -v 127.0.0.1:8000/hello.html &
    let COUNTER+=1
done
echo "Done!"
