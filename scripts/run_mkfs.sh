#!/bin/bash


if [ $# -lt 1 ]; then
  echo 1>&2 "$0: not enough arguments"
  exit 2
elif [ $# -gt 1 ]; then
  echo 1>&2 "$0: too many arguments"
  exit 2
fi

BUILD_DIR="$1"
if [ ! -d "$BUILD_DIR" ]
then
    echo 1>&2 "$0: The directory $BUILD_DIR does not exist"
    exit 2
fi

# import common functions
source "${BASH_SOURCE%/*}"/util.sh

OUT_DIR="$BUILD_DIR/test_mkfs"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"


set -x
exec 2>"$OUT_DIR/test_mkfs.log"

test_mkfs "mkfs_small" 1048576 64           # 1 MB
test_mkfs "mkfs_medium" 16777216 128        # 16 MB

