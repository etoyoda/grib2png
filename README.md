# grib2png
simple converter from WMO GRIB Edition 2 to PNG
# 何がしたいか

毎日の気象実況監視と、気象界へのタイル工法普及のため、
衛星・レーダーに次いで数値予報GPVもタイルまたはWebメルカトル空間上画像で
アーカイブして置こうと思いました。

データソースについては長くなるので datasource.rd に分けて書きます。
日量 700 メガバイトとかなりデカいので、
画像化したものだけを保存しようと思います。

# おまけプログラム gribslim

GRIB2 のファイルから一部を抽出するプログラム。
詳細は gribslim.md に書いておきます。
