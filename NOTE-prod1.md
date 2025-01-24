# NOTE-prod1.md - プロダクションの説明 (GSM予報値取得と図化)
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
## 動作説明
