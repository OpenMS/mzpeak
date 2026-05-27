#!/usr/bin/env bash

set -eu
set -o pipefail

################################################################################
header_file=include/mzpeak.h

################################################################################
function generate() {
  awk 'NR == 1, /pragma/' "$header_file"
  echo

  while IFS= read -r -d "" file; do
    if [[ $file != "include/mzpeak.h" ]]; then
      sed -E 's|^include/(.*)+$|#include "\1" // IWYU pragma: keep|' <<<"$file"
    fi
  done < <(find include -maxdepth 2 -type f -name '*.h' -print0)
}

################################################################################
function main() {
  generate | clang-format >"$header_file"
}

################################################################################
main "$@"
