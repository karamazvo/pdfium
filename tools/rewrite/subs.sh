#!/bin/bash

# Substitute one pattern for another thoughout the repository
if  [ -z "$1" ] ; then
    echo "Missing old pattern"
    exit 1
fi

if  [ -z "$2" ] ; then
    echo "Missing new pattern"
    exit 1
fi

sed -i "s/\\b$1\\b/$2/g" `git grep -l "$1"`


