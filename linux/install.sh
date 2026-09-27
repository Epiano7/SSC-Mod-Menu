#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$HERE/python/bin/python3" "$HERE/ssc_installer.py" "$@"
