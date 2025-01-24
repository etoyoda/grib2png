# grib2png パッケージ
simple converter from WMO GRIB Edition 2 to PNG
# 何がしたいか

## 当初想定
毎日の気象実況監視と、気象界へのタイル工法普及のため、
衛星・レーダーに次いで数値予報GPVもタイルまたはWebメルカトル空間上画像で
アーカイブして置こうと思いました。

データソースについては長くなるので datasource.rd に分けて書きます。
日量 700 メガバイトとかなりデカいので、
画像化したものだけを保存しようと思います。

## 拡張

# プログラム
## gribslim - GRIB2の一部抽出

GRIB2 のファイルから一部を抽出するプログラム。節単位の抽出を行うことにより計算誤差などのない高速な処理ができる。
詳細は https://github.com/etoyoda/grib2png/blob/sfcanal/gribslim.md に書いておきます。

## wisgsmpng.bash - GSM予報値の取得・図化の運用スクリプト

https://github.com/etoyoda/grib2png/blob/sfcanal/wisgsmpng.md
