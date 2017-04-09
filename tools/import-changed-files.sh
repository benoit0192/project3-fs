#!/bin/bash

if [ "$(uname)" != "Minix" ]; then
    echo "This script must be executed on your Minix VM, not in your host machine"
    echo "Aborting"
    exit 1
fi

echo "Syncing changed files... "

rsync -c --progress --files-from=/mnt/virtual_shared/project3-fs/tools/changed-files.list /mnt/virtual_shared/project3-fs/ /home/repo

echo "[Done]"
