# wisgsmpng.md - プロダクション用スクリプト (GSM予報値取得と図化)
## プログラム
本 grib2png パッケージ内の wisgsmpng.bash
## 成果物
6時間毎にディレクトリ
https://toyoda-eizi.net/nwp/p1/jmagrib/{yyyy}{mm}{dd}T{HH}Z/
を作って次を作成
* gsm{yyyy}{mm}{dd}T{HH}.bin GRIB2データ（必要な要素・面・予報時間に限定）
* gsm{yyyy}{mm}{dd}T{HH}.txt 地点データデコード形式で10度格子で抽出した6時間予報値
* v{yyyy}{mm}{dd}T{HH}00Z\_f{fth}\_{level}\_{element}.png 全球メルカトル画像
## データ源
WISで提供されている気象庁GSM予報値
https://www.wis-jma.go.jp/d/o/RJTD/GRIB/Global_Spectral_Model/Latitude_Longitude/1.25_1.25/90.0_-90.0_0.0_358.75/Upper_air_layers/{yyyy}{mm}{dd}/{hh}0000/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_{yyyy}{mm}{dd}{hh}0000_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin
## 動作説明
* 取得対象時刻決定（現在時刻の3時間前を6時間単位で切り捨て)
* 作業フォルダ作成（排他ロックを兼ねる）
* データ取得
* gribslim によって必要な要素・面・予報時間を抽出
* grib2png によって図化および地点データデコード形式を作成
* 成果物置き場に移動
