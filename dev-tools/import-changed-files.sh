#!/bin/bash

if [ "$(uname)" != "Minix" ]; then
    echo "This script must be executed on your Minix VM, not in your host machine"
    echo "Aborting"
    exit 1
fi

echo "Syncing changed files... "

rsync -c --progress --files-from=/mnt/shared/dev-tools/changed-files.list /mnt/shared/ /home/repo

echo "[Done]"
