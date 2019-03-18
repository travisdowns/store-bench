#!/bin/bash
source "$(dirname "$0")/common.sh"

export TITLE="Interleaved vs Single Stores${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1 3 5"
export CLABELS="region size (KiB),interleaved,sfenceA,sfenceB"
export YLABEL="cycles/iteration"

"$PLOT3" interleaved interleaved-sfenceA interleaved-sfenceB
