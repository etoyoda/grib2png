for file in wisgsmpng.bash grib2png gribslim makelist.sh gribpick emagram \
 run-emagram.bash send_png_mail.rb
do
  sudo -u nwp install -m 0755 $file /nwp/bin
done
sudo -u nwp install -m 0644 map.html /nwp/p1/
test -d /nwp/a1 || sudo install -d -o nwp -g nwp -m 0755 /nwp/a1
sudo install -m 0644 crontab.txt /etc/cron.d/grib2png
