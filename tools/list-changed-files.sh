#!/bin/bash

if [ "$(uname)" == "Minix" ]; then
    echo "This script must be executed on your host machine, not in Minix VM"
    echo "Aborting"
    exit 1
fi

echo -n "Computing diff... "

git diff --name-only 8de4188f9233bce734fab23088fafb8a187f8c9b > changed-files.list

echo "[Done]"
