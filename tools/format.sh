#/bin/bash

SCRIPT_PATH=$(readlink -f "${BASH_SOURCE[0]}")
PROJECT_ROOT=$(dirname $(dirname "$SCRIPT_PATH"))

pushd "$PROJECT_ROOT" > /dev/null
{ git ls-files -- '*.cpp' '*.h' '*.hpp' '*.c' && git ls-files --others --exclude-standard -- '*.cpp' '*.h' '*.hpp' '*.c'; } | sort -u | xargs clang-format -i
popd > /dev/null
