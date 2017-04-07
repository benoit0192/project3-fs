#!/bin/bash

if [ "$(uname)" == "Minix" ]; then
    echo "This script must be executed on your host machine, not in Minix VM"
    echo "Aborting"
    exit 1
fi

echo -n "Computing diff... "

git diff --name-only 32978a18fde64152bf8e94b2076789df9616db79 > changed-files.list

echo "[Done]"
