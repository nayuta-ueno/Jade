#!/bin/bash
set -eo pipefail

(cd main && clang-format -i *.c *.h */*.{c,h})

if [ -f /.dockerenv ]; then
    PATH=${PATH}:/root/.local/bin
fi

if [ -x "$(command -v pycodestyle)" ]; then
    pycodestyle --max-line-length=100 *.py pinserver/*.py pinserver/test/*.py jadepy/*.py
fi
