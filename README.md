# 3大学連携プロジェクト作品 - topology
3Tproject[^*1] の一環で制作したサウンドインスタレーション作品  
Arduinoを内蔵した立体を傾けることで、遠隔のPCで制御しているサウンドが変化  
鑑賞者が表現者であると同時に、聞き手でもあるインタラクティブ作品  

[^*1]: 大学間の結束力強化を目的に、東京音楽大学, 多摩美術大学, 東京電機大学の3大学が  
体験型アートを協働制作するプロジェクト。音楽, 芸術, 技術を融合して新たな価値創出を試みる  
今年のテーマは「TS UNAGU」  

詳しいイベント概要は[こちら](https://www.tokyo-ondai.ac.jp/information/43227.php)  
展示作品のPR映像は[こちら](https://www.instagram.com/reel/DHa6gJnh3G4/?utm_source=ig_web_copy_link&igsh=MzRlODBiNWFlZA==)

# デモ
* お客さんは立体を自由に傾けたり揺らしながら作品と関わる
* 立体のインタラクションにより、8つのスピーカーから流れている音楽の要素が変化  
立体は6つ設置されており、音響効果は「ピッチ(音高)」「エコー(反響具合)」など全6種



https://github.com/user-attachments/assets/3899bba9-66c7-4516-a5fb-9064c85aa13b



上記は「ピッチ」の体験例

![音楽の道_夜](https://github.com/user-attachments/assets/ad9215d0-77c3-42e7-8170-cc3ca1718776)
夜の展示風景

# 特徴
* アートを支える「システム」として技術で貢献 (組み込み, 電気回路, 通信…)
* 1ヶ月強のスピード開発

### 開発者担当箇所
* 回路図の作成
* 基盤ハンダ付け
* プロトタイプ作成
* モジュール選定
* ジャイロセンサの制御
* 各システム間のデータ通信(Arduino, Processing, Max/MSP)
* ドキュメント作成

### 分担箇所(多摩美術大学)
* 基盤ハンダ付け
* LEDの制御
* モジュール選定
* 立体の外観設計/制作

### 分担箇所(東京音楽大学)
* 音源の制作
* Max/MSPを用いた音源の加工システム構築

# 学び
* 理想と展示当日までの作品仕様の擦り合わせとブラッシュアアップ -> 三現主義(現場,現物,現実)
* 異なる開発手法への対応力  
立体側(Arduino)はC/C++でコーディング  
音響側はノードベースのサウンドプログラミングソフト(Max/MSP)へシステムを組み込み  
ArduinoとMax/MSPでUDP, OSCと異なる通信方式を扱う点で苦労  
* アートを仕様に落とし込むこと(設計図などの作成)
* ソフトからハードまで一貫して制作
* データシート/レジスタマップの読み取り
* 小さく作って大きく育てること

# 開発環境
* Arduino
* Processing
* Max/MSP

# 使用法
詳細は立体システム起動手順.pdfを参照

以下のディレクトリにconfig.hファイルを作成してください  
/Arduino/config.h
```
#ifndef config_h
#define config_h

#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"

#endif
```
> [!NOTE]
> Max/MSP側の使用音源,加工機構は添付しておりません  
> 皆様で自由に制作してご利用ください

# 展望
* ジャイロセンサの精度向上
* 立体(Arduino)間の距離検出
* 通信環境の安定化
