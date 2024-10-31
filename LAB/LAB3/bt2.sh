#!/usr/bin/env bash

echo "--- START $0 ---"
echo "\$1: $1"
echo "\$2: $2"

[ -n "$NAME" ] && echo "Hello $NAME"

if [ -n "$MSSV" ]; then
  echo "\$MSSV is set as: $MSSV"
else
  echo "\$MSSV is not set"
fi

echo "--- END $0 ---"
