#!/bin/bash
OPER=$HOME/src
cd $OPER
LASTCOMP=`ls -1t /mnt/meru/data/prod/radman/latest/fmi/radar/composite/lowest/*_fmi.radar.composite.lowest_FIN_RAVAKE.pgm | head -1`
TIMESTAMP=`basename $LASTCOMP | cut -c-12`
echo "PPN process for $TIMESTAMP started at"
date
source ~/miniconda3/etc/profile.d/conda.sh
conda activate fmippn
export OMP_NUM_THREADS="1"
pushd ~/devel/fmippn/src
$HOME/miniconda3/envs/fmippn/bin/python ~/devel/fmippn/src/run_ppn.py --timestamp=${TIMESTAMP} --config=esteri # >> $OPER/esteri_realtime_ppn.log 2>&1
date
popd
$OPER/fmippn-vc/fmippn-run-and-distribution/postprocess_ppndata.tcsh /dev/shm/ppn/nc_"$TIMESTAMP".h5 # > vis.log 2>&1
# ./visualize_ensmean.tcsh /dev/shm/ppn/nc_"$TIMESTAMP".h5 # > vis.log 2>&1
echo "PPN process for $TIMESTAMP ended at"
date
echo
exit
