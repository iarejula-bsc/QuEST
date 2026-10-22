#!/bin/bash
# Write a copy of the installed dmr_wrapper whose prte stays in the
# foreground of a background job instead of daemonizing, so its output (and
# that of the prted daemons, with --leave-session-attached) goes to
# $DMR_PRTE_LOG instead of /dev/null. Extra prte flags come from $DMR_PRTE_EXTRA.
#   make_debug_wrapper.sh <output path>
set -euo pipefail

src=$(command -v dmr_wrapper)
out=$1
line='    if ! "$PRTE_WITH_PATH" --daemonize --no-ready-msg --report-uri "$DVM_URI_FILE" "${ELASTIC_FLAGS[@]}"; then'
grep -qxF "$line" "$src" || { echo "make_debug_wrapper: prte launch line not found in $src" >&2; exit 1; }

new='    if ! { "$PRTE_WITH_PATH" --no-ready-msg --report-uri "$DVM_URI_FILE" "${ELASTIC_FLAGS[@]}" $DMR_PRTE_EXTRA >>"$DMR_PRTE_LOG" 2>\&1 \& }; then'
sed "s|^$(printf '%s' "$line" | sed 's/[][\.*^$|]/\\&/g')\$|$new|" "$src" > "$out"
chmod +x "$out"
grep -q 'DMR_PRTE_LOG' "$out" || { echo "make_debug_wrapper: substitution failed" >&2; exit 1; }
