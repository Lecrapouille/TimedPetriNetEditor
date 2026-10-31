#!/bin/bash
##=====================================================================
## TimedPetriNetEditor: make again the pictures of the documentation.
##
## The editor takes them itself (-s: the net, -w: the whole window), so
## they can always be made again after a change of the editor or of a net.
## Run from the root of the project, after make:
##
##   tools/screenshots.sh              # all the pictures
##   tools/screenshots.sh Philosophers # the ones whose path contains this
##
## 1. Examples: each net of examples/ gives <folder>/<name>.png, the net
##    alone. A file screenshot.args next to the net gives other options
##    (for example "-w -r -d 3": the whole window after 3 s of simulation).
## 2. Documentation: each line of tools/screenshots/*.txt is
##      <picture.png> <options of the editor> <net>
##    (one list per chapter; # starts a comment; quote an option with
##    spaces: -f "Timing diagram").
## In the options, -s (the net) or -w (the whole window) is given alone:
## the picture is added after it. Without both, -s.
##=====================================================================

set -u
EDITOR=${TPNE_EDITOR:-build/linux-gcc-debug/bin/TimedPetriNetEditor}
GEOMETRY=${TPNE_GEOMETRY:-1400x900}
FILTER=${1:-}
status=0

shoot() # picture options... net
{
    local picture=$1; shift
    [[ -n "$FILTER" && "$picture" != *"$FILTER"* ]] && return
    local args=() target=-s
    for option in "$@"; do
        case "$option" in
            -s|-w) target=$option ;;
            *) args+=("$option") ;;
        esac
    done
    mkdir -p "$(dirname "$picture")"
    rm -f "$picture"
    if timeout 120 "$EDITOR" -g "$GEOMETRY" "$target" "$picture" "${args[@]}" > /dev/null 2>&1 && [ -s "$picture" ]; then
        echo "ok   $picture"
    else
        echo "FAIL $picture ($*)"; status=1
    fi
}

# 1. The examples.
while IFS= read -r net; do
    folder=$(dirname "$net")
    options=""
    [ -f "$folder/screenshot.args" ] && options=$(cat "$folder/screenshot.args")
    eval "set -- $options"
    shoot "$folder/$(basename "${net%.*}").png" "$@" "$net"
done < <(find examples \( -name '*.json' -o -name '*.flowshop' -o -name '*.teg' \) | sort)

# 2. The pictures of the documentation.
for list in tools/screenshots/*.txt; do
    [ -f "$list" ] || continue
    while read -r picture options; do
        [[ -z "$picture" || "$picture" == \#* ]] && continue
        # Quotes group words: -f "Timing diagram".
        eval "set -- $options"
        shoot "$picture" "$@"
    done < "$list"
done
exit $status
