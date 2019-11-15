#!/bin/tcsh
setenv LD_LIBRARY_PATH /var/opt/lib
setenv TZ UTC
setenv RAVAKE_ODIM_HDF5_HDRFILE ravake.h5
setenv RAVAKE_ACCPREF RAVACC

set TIMESTAMP = $1
set THRCFG = precipitation_thresholds_Helsinki.conf
set ACCDIR = /dev/shm/ppn/acc
set OUTDIR = /dev/shm/ppn/prob
set AREA = Ravake
set MEMBERS = 15
set TIMESTEPS = 18

./prob_thresholding $TIMESTAMP $THRCFG $ACCDIR $OUTDIR $AREA $MEMBERS $TIMESTEPS
exit
