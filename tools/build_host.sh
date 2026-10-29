#!/bin/sh
set -e
echo "UI-only developer target; native Windows engine is not linked by this script."u
cd "$(dirname "$0")/.."
if ! command -v qmake >/dev/null 2>&1; then
  echo 'Qt development kit is missing: cannot compile native Qt application.' >&2
  exit 2
fi
mkdir -p build-host
cd build-host
qmake ../SDIO_Modern_Qt.pro 'CONFIG+=release'
make -j2
