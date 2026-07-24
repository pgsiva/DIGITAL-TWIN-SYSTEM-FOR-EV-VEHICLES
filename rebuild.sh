#!/usr/bin/env bash
# Rebuilds the real "Digital-Twin/" project folder structure from the
# flattened files sitting next to this script (filenames use "__" in place
# of "/"). Run this from inside the extracted zip folder:
#
#   bash rebuild.sh
#
# It creates a new "Digital-Twin/" folder here with the correct nested
# structure, ready to push to GitHub.

set -e
shopt -s dotglob nullglob
OUT="Digital-Twin"
mkdir -p "$OUT"

for f in *; do
  # skip this script, the manifest, and directories
  case "$f" in
    rebuild.sh|0_MANIFEST_AND_FOLDER_GUIDE.txt) continue ;;
  esac
  [ -f "$f" ] || continue

  # convert "a__b__c.txt" -> "a/b/c.txt"
  relpath=$(echo "$f" | sed 's/__/\//g')
  destdir=$(dirname "$OUT/$relpath")
  mkdir -p "$destdir"
  cp "$f" "$OUT/$relpath"
done

echo "Done. Rebuilt structure is in ./$OUT"
echo "cd $OUT && git init && git add . && git commit -m 'initial commit'"
