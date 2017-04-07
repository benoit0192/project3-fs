#!/bin/bash

if [ "$(uname)" != "Minix" ]; then
    echo "This script must be executed on your host machine, not in Minix VM"
    echo "Aborting"
    exit 1
fi

echo "Syncing changed files... "

rsync -c --progress --files-from=/mnt/shared/tools/changed-files.list /mnt/shared /home/repo

echo "[Done]"
