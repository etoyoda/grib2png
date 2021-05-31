for file in wisgsmpng.bash grib2png gribslim
do
  sudo -u nwp install -m 0755 $file /nwp/bin
done
