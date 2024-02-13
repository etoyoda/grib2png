for file in wisgsmpng.bash grib2png gribslim makelist.sh gribpick emagram
do
  sudo -u nwp install -m 0755 $file /nwp/bin
done
sudo -u nwp install -m 0644 map.html /nwp/p1/
