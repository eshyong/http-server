#!/bin/bash
COUNTER=0
while [ $COUNTER -lt 10000 ]; do
    curl -v 127.0.0.1:8000/test.html
    let COUNTER+=1
done
