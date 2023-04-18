#!/bin/bash

OFILE=$1
INFILES=${@:2}

# Build a blank image, with some buffer space for us
totalsectors=$(wc -c $INFILES | tail -n1 | awk '{print $1}')
totalsectors=$(( ($totalsectors + 65536) / 512 ))
dd if=/dev/zero of=$OFILE bs=512 count=$totalsectors

filesizes=$(wc --total=never -c $INFILES | awk '{print $1}')

sfdisk_cmd="label: gpt\n\n"
for s in $filesizes; do
    # Only give sfdisk the size, everything else has the default values
    sfdisk_cmd+=",+"$(( $s / 512 + 1 ))",,\n"
done

echo -e $sfdisk_cmd | sfdisk $OFILE

# Get the actual sector offsets (the tail is dodgy as fuck)
part_offsets=$(fdisk $OFILE -l -ostart -Lnever | tail -n+9)

read -ra INFILES <<< "$INFILES" # Needs to be an array

count=0
for o in $part_offsets; do
    dd if=${INFILES[count]} of=$OFILE bs=512 seek=$o conv=notrunc

    count=$(( count + 1 ))
done;
