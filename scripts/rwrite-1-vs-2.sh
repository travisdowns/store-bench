#!/bin/bash
source "$(dirname "$0")/common.sh"

export TITLE="Interleaved vs Single Stores${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1 3"
export CLABELS="region size (KiB),single,interleaved"
export YLABEL="cycles/iteration"

"$PLOT2" wrandom1 interleaved
