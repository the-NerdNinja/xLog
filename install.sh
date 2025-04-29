#!/usr/bin/env bash
set -e

PREFIX=/usr/local
BINDIR=${PREFIX}/bin

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

sudo install -Dm755 xlog "${BINDIR}/xlog"

DATA="${HOME}/xLog"
mkdir -p "$DATA"

echo "✓ Installed xlog to ${BINDIR}/xlog"
echo "✓ Database directory: ${DATA}"

