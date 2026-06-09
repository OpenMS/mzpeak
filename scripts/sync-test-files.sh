#!/usr/bin/env bash

################################################################################
set -eu
set -o pipefail

################################################################################
function usage() {
  cat <<EOF
Usage: $(basename "$0") [options] mzpeak_rust_dir

  -h      This message

Sync test files from the given mzPeak Rust implementation.

EOF
}

################################################################################
function main() {
  local top
  top=$(realpath "$(dirname "$0")/..")

  # Option arguments are in $OPTARG
  while getopts "h" o; do
    case "${o}" in
    h)
      usage
      exit
      ;;

    *)
      exit 1
      ;;
    esac
  done

  shift $((OPTIND - 1))

  if [[ $# -ne 1 ]]; then
    echo >&2 "ERROR: missing path to the mzpeak_rust repo"
    exit 1
  fi

  while IFS= read -r -d "" file; do
    cp -v "$file" "$top"/test/files
  done < <(
    find "$1" -type f \
      \( -name 'small.*' -o -name '*.mzpeak' \) \
      -print0
  )

  (cd "$top/test/files" &&
    rm -rf "small.dir" &&
    mkdir small.dir &&
    cd small.dir &&
    unzip ../small.mzpeak)
}

################################################################################
main "$@"
