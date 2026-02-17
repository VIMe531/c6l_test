# c6l_test
こちらの[ブログ][blog]にて使用した，M5Stack社製 M5 Unit C6Lを使用するためのプログラムです．
使用前には，以下の諸注意等をお読みいただいた上で，ご自身の判断でご使用くださいますようお願いいたします．

[blog]: https://www.switch-science.com/blogs/magazine/m5-c6l-lora

## 使用に際して
M5 Unit C6Lでは，使用する周波数帯をプログラム内で設定することができます．868 MHz～923 MHzの周波数に対応していますが，日本国内で使用が認められているLoRaWANの周波数帯域は920.5~928.0 MHzです．また，200 Hzごとにチャンネルが割り当てられており，そのチャンネルに準じた周波数を使用する必要があります．チャンネルに関する技術条件をまとめた[PDF][pdf-soumu]が，総務省から公開されていますので，ご確認の上ご使用ください．

また，M5 Unit C6Lは，最大電波出力が+22 dBm（158.49 mW）のデバイスであり，最大出力での使用には，[陸上移動局][license-soumu]として登録が必要です．登録なしで利用するためには，20 mW以下の出力で使用する必要があります．

ARIBが公開している「[920MHz帯テレメータ用、テレコントロール用及びデータ伝送用無線設備(ARIB STD-T108)][regulation-pdf]」についてもご確認の上，ご利用いただくことをお勧めします．日本語版は有料ですが，英語版は無料で公開されています．

公式から提供されている[サンプルプログラム][m5-sample]をそのまま書き込み，日本国内で実行すると，周波数帯や送信出力，休止時間など諸々の規定に引っ掛かります．


| 周波数	| チャンネル	| 出力			|
|-----------|---------------|---------------|
| 920.6 MHz	| 24			| 250 mW, 20 mW	|
| 920.8 MHz	| 25			| 250 mW, 20 mW	|
| 921.0 MHz	| 26			| 250 mW, 20 mW	|
| 921.2 MHz	| 27			| 250 mW, 20 mW	|
| 921.4 MHz	| 28			| 250 mW, 20 mW	|
| 921.6 MHz	| 29			| 250 mW, 20 mW	|
| 921.8 MHz	| 30			| 250 mW, 20 mW	|
| 922.0 MHz	| 31			| 250 mW, 20 mW	|
| 922.2 MHz	| 32			| 250 mW, 20 mW	|
| 922.4 MHz	| 33			| 250 mW, 20 mW	|
| 922.6 MHz	| 34			| 250 mW, 20 mW	|
| 922.8 MHz	| 35			| 250 mW, 20 mW	|
| 923.0 MHz	| 36			| 250 mW, 20 mW	|
| 923.2 MHz	| 37			| 250 mW, 20 mW	|
| 923.4 MHz	| 38			| 250 mW, 20 mW	|
| 923.6 MHz	| 39			| 20 mW			|
| 923.8 MHz	| 40			| 20 mW			|
| 924.0 MHz	| 41			| 20 mW			|
| 924.2 MHz	| 42			| 20 mW			|
| 924.4 MHz	| 43			| 20 mW			|
| 924.6 MHz	| 44			| 20 mW			|
| 924.8 MHz	| 45			| 20 mW			|
| 925.0 MHz	| 46			| 20 mW			|
| 925.2 MHz	| 47			| 20 mW			|
| 925.4 MHz	| 48			| 20 mW			|
| 925.6 MHz	| 49			| 20 mW			|
| 925.8 MHz	| 50			| 20 mW			|
| 926.0 MHz	| 51			| 20 mW			|
| 926.2 MHz	| 52			| 20 mW			|
| 926.4 MHz	| 53			| 20 mW			|
| 926.6 MHz	| 54			| 20 mW			|
| 926.8 MHz	| 55			| 20 mW			|
| 927.0 MHz	| 56			| 20 mW			|
| 927.2 MHz	| 57			| 20 mW			|
| 927.4 MHz	| 58			| 20 mW			|
| 927.6 MHz	| 59			| 20 mW			|
| 927.8 MHz	| 60			| 20 mW			|
| 928.0 MHz	| 61			| 20 mW			|
|-----------|---------------|---------------|

[m5-sample]: https://docs.m5stack.com/ja/arduino/unit_c6l/lora
[pdf-soumu]: https://www.soumu.go.jp/main_content/000302185.pdf
[license-soumu]: https://www.tele.soumu.go.jp/j/adm/system/ml/920mhz/index.htm
[regulation-pdf]: https://www.arib.or.jp/kikaku/kikaku_tushin/std-t108.html

