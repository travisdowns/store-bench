# run all the plot generation scripts to generate assets
if [ -z "$SUFFIX" ]; then
    echo "Set filename SUFFIX to new or old depending on microcode"
    exit 1
fi

# turn off prefetching
sudo wrmsr -a 0x1a4 "$((2#1111))" && sudo rdmsr -a -x 0x1A4

OUTFILE=assets/i-vs-s-$SUFFIX.svg ./scripts/rwrite-1-vs-2.sh
OUTFILE=assets/i-plus-counters-$SUFFIX.svg ./scripts/interleaved-1.sh
OUTFILE=assets/i-sfence-$SUFFIX.svg ./scripts/rwrite2-vs-sfence.sh

export ARRAY1_SIZE=2048

OUTFILE=assets/i-vs-s-2mib-$SUFFIX.svg ./scripts/rwrite-1-vs-2.sh
OUTFILE=assets/i-plus-counters-2mib-$SUFFIX.svg ./scripts/interleaved-1.sh
OUTFILE=assets/i-sfence-2mib-$SUFFIX.svg ./scripts/rwrite2-vs-sfence.sh
OUTFILE=assets/i-unrolled-2mib-$SUFFIX.svg ./scripts/rwrite2-unrolled.sh
