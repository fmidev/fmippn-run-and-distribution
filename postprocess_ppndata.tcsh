#!/bin/tcsh
set BIN = $HOME/src/fmippn-vc/fmippn-run-and-distribution
setenv SUOMI_ARGPATH $BIN/config
setenv SUOMI_RAINCLASSES $SUOMI_ARGPATH/rainclasses.txt
set DOLOOP = 0
set INH5 = $1
set ORIGTIME = `echo $INH5:t:r | cut -d_ -f2`
set ORMINS = `echo $ORIGTIME | cut -c11-12`
set WWWDIR = /mnt/meru/data/prod/radman/ppn
set VERDIR = /mnt/meru/data/prod/radman/ppnverif

cd $BIN
../fmippn-member-interp/run_interp_oper.tcsh $ORIGTIME >& logs/interp.log

#cd /var/tmp/ppn/images
#rm *.p?m
./ensmean $INH5

foreach MZ (meandBZ_????????????.pgm)
  set T = `echo $MZ:r | cut -d_ -f2`
  set MIN = `echo $T | cut -c12`

  set SUOMI_MEAN = M_"$ORIGTIME"-"$T"_SUOMI1_dBZ.pgm
  pamcut 200 400 760 1226 $MZ > $SUOMI_MEAN
  $BIN/gdmap $SUOMI_MEAN "$SUOMI_MEAN:r".ppm 'PPN ensemble mean dBZ'
  pamtogif "$SUOMI_MEAN:r".ppm > "$SUOMI_MEAN:r".gif

  set SUOMI_DETERM = D_"$ORIGTIME"-"$T"_SUOMI1_dBZ.pgm
  set DZ = determdBZ_"$T".pgm
  pamcut 200 400 760 1226 $DZ > $SUOMI_DETERM
  $BIN/gdmap $SUOMI_DETERM "$SUOMI_DETERM:r".ppm 'PPN deterministic dBZ'
  pamtogif "$SUOMI_DETERM:r".ppm > "$SUOMI_DETERM:r".gif

  set SUOMI_UNPERTURB = U_"$ORIGTIME"-"$T"_SUOMI1_dBZ.pgm
  set DZ = unpertdBZ_"$T".pgm
  pamcut 200 400 760 1226 $DZ > $SUOMI_UNPERTURB
  $BIN/gdmap $SUOMI_UNPERTURB "$SUOMI_UNPERTURB:r".ppm 'PPN unperturbed dBZ'
  pamtogif "$SUOMI_UNPERTURB:r".ppm > "$SUOMI_UNPERTURB:r".gif

  echo $T

  if("$ORMINS" == "00" || "$ORMINS" == "30" || ) then
     set DOLOOP = 1
     if("$MIN" == "0") then
        set TUL = `ls -1 /mnt/meru/data/prod/radman/fmi/prod/run/fmi/radar/tuliset/pic/G_"$ORIGTIME"-"$T".gif`
        if("$TUL" != "") then
           montage -tile x1 -geometry 100% -border 3x3 -background black -frame 0 $TUL "$SUOMI_MEAN:r".gif "$SUOMI_UNPERTURB:r".gif "$ORIGTIME"-"$T"_TULISET+PPN.gif
        endif
     endif
  endif
end

if($DOLOOP) then
   set ANIM = "$ORIGTIME"_TULISET+PPN_anim.gif 
   convert -loop 0 -delay 20 "$ORIGTIME"-*_TULISET+PPN.gif $ANIM 
   cp $ANIM $WWWDIR
   unlink $WWWDIR/latest.gif 
   ln -s $WWWDIR/$ANIM $WWWDIR/latest.gif 
   find $WWWDIR -name '*_anim.gif' -mmin +1100 -exec rm -f {} \; &
endif

find . -name '*.gif' -mmin +360 -exec rm -f {} \;
find /dev/shm/ppn -name 'nc_*.h5' -mmin +20 -exec rm -f {} \;


# Verification: Observer dBZ vs. PPN deterministic dBZ

set OBSNOWC = `ls -1tr D_*-"$ORIGTIME"_SUOMI1_dBZ.gif | head -1`
set OBSORIGTIME = `echo $OBSNOWC | cut -d_ -f2 | cut -d- -f1`
foreach DETFILE (D_"$OBSORIGTIME"-*.gif)
   set OBSTIME = `echo $DETFILE | cut -d_ -f2 | cut -d- -f2`
   set OBSFILE = /mnt/meru/data/prod/radman/latest/fmi/radar/iris/finland1/"$OBSTIME"_fmi.radar.iris.finland1.gif
   # montage of obs vs. determ
   if(-e $OBSFILE) then
      montage -geometry 100% -border 3x3 -background black -frame 0 $OBSFILE $DETFILE "$OBSORIGTIME"-"$OBSTIME"_OBS+DETERM.gif
   endif
end
# animation of obs vs. determ
set ANIM = "$OBSORIGTIME"_OBS+DETERM_anim.gif 
convert -loop 0 -delay 20 "$OBSORIGTIME"-*_OBS+DETERM.gif $ANIM
cp $ANIM $VERDIR
unlink $VERDIR/latest_verif.gif 
ln -s $VERDIR/$ANIM $VERDIR/latest_verif.gif 
find $VERDIR -name '*_anim.gif' -mmin +1100 -exec rm -f {} \; &

exit
