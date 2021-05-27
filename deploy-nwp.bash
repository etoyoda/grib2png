for file in wisgsmpng.bash grib2png
do
  sudo -u nwp install -m 0755 $file /nwp/bin
done
