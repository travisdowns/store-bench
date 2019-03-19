#!/bin/bash
source "$(dirname "$0")/common.sh"

export TITLE="Interleaved Stores with Unrolling${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1 3 5"
export CLABELS="region size (KiB),interleaved,interleaved (unroll 2),interleaved (unroll 4)"
export YLABEL="cycles/iteration"

"$PLOT3" interleaved interleaved-u2 interleaved-u4
