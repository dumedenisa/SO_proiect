#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <character>"
    exit 1
fi

character="$1"
count=0

while IFS= read -r line || [ -n "$line" ]; do
    if [[ $line =~ ^[[:upper:]][[:alnum:] ,.!]*[[:upper:]]$ && $line != *,* ]]; then
        if [[ $line == *"$character"* ]]; then
            ((count++))
        fi
    fi
done


echo $count
