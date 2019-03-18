#!/bin/bash
source "$(dirname "$0")/common.sh"

export TITLE="Interleaved vs Single Stores${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1 3 5 7"
export CLABELS="region size (KiB),interleaved,sfenceA,sfenceB,sfenceC"
export YLABEL="cycles/iteration"

"$PLOT4" interleaved interleaved-sfenceA interleaved-sfenceB interleaved-sfenceC
