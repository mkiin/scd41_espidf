# ESP32-S3 + SCD41 / ESP-IDF v5.4 リファレンス

## このサンプルの前提

- ESP-IDF v5.4系の通常のCプロジェクト。Arduino / PlatformIOは使用しない。
- ESP-IDFに同梱されているFreeRTOSを使用する。FreeRTOSを別にインストールしない。
- センサー1台、I2C0をこのコンポーネントだけが初期化・所有する。
- SCL=GPIO47、SDA=GPIO21、VDD=3.3 V、GND=共通GND。
- ピン番号はGPIO番号。Freenoveの具体的な製品型番は未提示なので、基板上の端子配置は断定しない。
- SDA/SCLには3.3 Vへの外部プルアップがあること。センサ基板にない場合は各4.7 kΩを追加。
- 3.3 V電源はSCD41の最大205 mAのピーク負荷に加え、ボード本体の負荷を支えられること。
- 最初はセンサーを含めて電源投入する。別プログラムでSCD41をpower_downした状態からの復帰は含めない。

## PCに必要な環境

ESP-IDF v5.4系とESP32-S3用ツールチェーンをインストールする。
公式案内: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/get-started/index.html

WindowsではESP-IDF環境のターミナル、Linux/macOSではESP-IDFのexport.shを読み込んだターミナルを使う。
VS CodeのESP-IDF拡張を使用している場合は、拡張のESP-IDF Terminalを開く。
`idf.py --version`でv5.4系と表示されることを確認する。

## ビルドシステムと各ファイルの役割

`idf.py`はビルド・書き込み用の窓口。CMakeが構成を決め、Ninjaがコンパイラとリンカを実行する。
ESP32-S3向けにクロスコンパイルしたファームウェアを、esptool経由でボードに書き込む。
ESP-IDFではコードを「コンポーネント」という単位にまとめる。

| ファイル | 役割 |
|---|---|
| CMakeLists.txt | プロジェクト全体をESP-IDFのビルドシステムに接続する |
| sdkconfig.defaults | 新しく作られる設定の初期値 |
| main/CMakeLists.txt | main.cとその依存コンポーネントを登録する |
| main/main.c | 起動処理とFreeRTOSの測定タスク |
| components/scd4x/CMakeLists.txt | 公式Cソースと自作HALをひとつのコンポーネントとして登録 |
| components/scd4x/sensirion_i2c_hal_espidf.c | ESP-IDFのI2C APIを呼ぶ処理、待機、終了処理 |
| components/scd4x/vendor/ | 版を固定して同梱したSensirion公式ソースとライセンス |

`components/scd4x`はESP-IDFが標準の探索対象として見つけるため、EXTRA_COMPONENT_DIRSは不要。
今回はソースをプロジェクトに直接同梱するため、idf_component.ymlも不要。
公式リポジトリのexample-usage/Makefileは使わない。このプロジェクトのCMakeでコンパイルする。
FreeRTOSConfig.hはESP-IDF側が用意するため、自作しない。独自partitions.csvもこの用途では不要。

ビルド後に生成されるもの:
- sdkconfig: 全項目が埋まった実際の設定。
- build/config/sdkconfig.h: Cソースで利用する設定ヘッダ。
- build/: 中間ファイル、bootloader、アプリ等の書き込み用バイナリ。

## なぜHALを書くのか

公式ドライバは、SCD41へ送る命令や受信データの解釈を知っている。
しかし使用するマイコン、GPIO番号、OS、I2C APIは決めていない。
そのため公式sensirion_i2c_hal.cの通信・待機関数は未実装になっている。
この空欄をESP-IDFの関数につなぐコードが必要になる。
HALはHardware Abstraction Layerの略で、ここでは機種ごとの差を受け持つ接続部分を指す。

例: main.cのscd4x_read_measurement()を呼ぶと、公式ドライバが命令を組み立て、
sensirion_i2c_hal_write()を呼ぶ。その関数をこのサンプルで実装し、
i2c_master_transmit()を使ってGPIO21/47から送信する。
待機後にsensirion_i2c_hal_read()がi2c_master_receive()で応答を受け取る。
受信データのCRC検証（通信データの破損検出）と温湿度の単位変換は公式ソースが行う。

本サンプルでは公式の空のsensirion_i2c_hal.cを取り込まず、同じ関数名を定義した
sensirion_i2c_hal_espidf.cをビルドする。ファイル名自体は異なってよい。
公式版と自作版の両方をビルドすると関数定義が重複するので、CMakeのSRCSは明示してある。

## 公式ドライバの導入方法

このZIPでは必要なソースを導入済み。利用者が追加ダウンロードする必要はない。
出所は https://github.com/Sensirion/embedded-i2c-scd4x 。
使用したコミットは `b52cebe1bb1b7050feaac75d7cd33e56c6a8a4e9`。
上流ファイルは無変更であり、LICENSEを保持している。

自分で同じ配置を再現する場合は、次の手順:

```sh
git clone https://github.com/Sensirion/embedded-i2c-scd4x.git
git -C embedded-i2c-scd4x checkout b52cebe1bb1b7050feaac75d7cd33e56c6a8a4e9
```

取得したディレクトリの直下から、以下の9ファイルをcomponents/scd4x/vendor/へコピーする。

```text
scd4x_i2c.c
scd4x_i2c.h
sensirion_common.c
sensirion_common.h
sensirion_i2c.c
sensirion_i2c.h
sensirion_config.h
sensirion_i2c_hal.h
LICENSE
```

ESP-IDFは標準Cの整数型・boolを提供するので、sensirion_config.hは変更不要。
公式のsensirion_i2c_hal.cとexample-usageのmain()はコピーしない。
その代わりに、このサンプルのHALとmain/main.cを使う。
ドライバの版によってAPIの引数が異なることがあるため、上記のコミットに固定している。

## sdkconfig.defaults

CONFIG_FREERTOS_HZ=100は1 tick = 10 msの設定。測定間隔の指定ではない。
CONFIG_LOG_DEFAULT_LEVEL_INFO=yはESP_LOGIによる測定ログを表示する設定。
UART0のコンソールを115200 baudにし、アプリログを内蔵USB Serial/JTAGにも複製する。
したがって、UART0へつながるUSB-UARTポートまたはGPIO19/20へつながるネイティブUSBポートでログを確認できる。
Freenoveの基板型番が分からないため、USB端子の名称・位置は基板の資料で確認する。
このプログラムはUSB OTG/TinyUSBを使用しない。

SCD41固有のCONFIG_SCD41等は用意していない。I2Cピンと周波数はHALのCコードにある。
FreeRTOSはESP-IDFに含まれるので、FreeRTOSを有効化する独自設定も不要。

sdkconfigがすでにある場合、defaultsを変更しても既存値は上書きされない。
既存プロジェクトでは`idf.py menuconfig`で対象項目を変更する。
今回のZIPはsdkconfigを含まないので、初回ビルドでdefaultsが反映される。
`idf.py fullclean`だけではsdkconfigは初期化されない。

## 動作の詳細

app_mainはESP-IDFが呼び出す入口。FreeRTOSのスケジューラはすでに起動している。
app_mainからxTaskCreateでscd41_taskを作る。4096はESP-IDFではバイト単位のスタック容量。

タスクはI2Cを初期化し、センサー起動待ち、測定停止、連続測定開始の順で処理する。
stop関数の500 ms待機は公式ドライバからHAL経由で行われる。
センサー自身が約5秒おきに測定値を更新し、タスクは1秒おきに新しい値の有無を確認する。
準備完了時だけCO2・温度・湿度を一緒に読み取り、ログへ出力する。

現在の公式ドライバではCO2はppm、温度はmilli-degree C、湿度はmilli-percent RHで返る。
後者2つを1000で割って表示する。
CRCやI2Cエラーが起きた周期では値を出さず、次の周期で確認する。
初期化時のstop/start失敗ではタスクを終了する。配線・電源を修正後にリセットする。
I2Cドライバの生成に失敗した場合はESP_ERROR_CHECKで停止・再起動する。
センサー電源断からの自動復旧やバスの物理的な固着解除はこの入門例に含めていない。

HALの待機は、マイクロ秒をtickへ切り上げ、呼び出した瞬間とtick境界のずれを考慮して1 tick加える。
例えば1 ms要求は100 Hz時に2 tickとなり、必要時間以上待つ。CPUをビジーループで占有しない。
コマンド送信と応答受信を分けて、公式ドライバが指定する待機を挟む。
ESP-IDFのesp_err_tをint8_tに直接キャストせず、HALの規約に合わせて成功0・失敗-1を返す。
詳細なESP-IDFエラー名はHALのログに残す。

このHALはI2C0・アドレス0x62の1台用。全SCD41アクセスをこのタスクにまとめる。
ほかのタスクから公式ドライバを同時に呼ばない。公式ドライバは共有の静的バッファを使う。
表示やWi-Fi送信を足す場合は、取得結果をFreeRTOS Queue等でそのタスクへ渡す。

## ビルド・書き込み

ZIPを展開したscd41_espidfディレクトリで、ESP-IDF環境のターミナルから実行する。

```sh
idf.py --version
idf.py set-target esp32s3
idf.py build
```

set-targetは新規プロジェクトの初回に実行する。毎回のビルドは`idf.py build`だけでよい。
既存プロジェクトに対して実行すると設定の再作成が行われるので、新規サンプルで実行する。

接続先のシリアルポート名を指定する。以下のPORTは文字列をそのまま入力せず、実際のポートに置き換える。

```sh
idf.py -p PORT flash monitor
```

ポート名の例: Windows=COM5、Linux=/dev/ttyUSB0や/dev/ttyACM0、macOS=/dev/cu.usbmodem...。
monitor終了はCtrl+]。
サンプル表示（実測値ではない）:

```text
I (...) scd41: Periodic measurement started; updates every ~5 s
I (...) scd41: CO2=612 ppm, T=24.30 C, RH=48.20 %
```

## 問題の切り分け

| 状態 | 確認箇所 |
|---|---|
| idf.pyが見つからない | ESP-IDF環境のターミナルを開いているか |
| driver/i2c_master.hが見つからない | ESP-IDF v5.4系か、scd4xのCMakeにesp_driver_i2cがあるか |
| scd4x関数の引数・型のエラー | 同梱の公式ソースを使っているか。別版との混在がないか |
| multiple definition | 公式の空HALと自作HALを両方SRCSへ追加していないか |
| stop failed / writeエラー | 共通GND、GPIO番号、SDA/SCL、プルアップ、3.3 V電源 |
| ログが出ない | 書き込んだポート、コンソール設定、USB端子の接続先 |

## 確認範囲

公式リポジトリを取得し、同梱ファイルが指定コミットと同一であること、APIの型・関数名、
CMakeのソース一覧、必要な関数定義の対応を確認した。
作成環境にESP-IDFツールチェーンとESP32-S3実機はなく、`idf.py build`・書き込み・実機測定は未実施。
コンパイル済みや実機検証済みのサンプルとは扱わないこと。

## 公式資料

- ドライバ: https://github.com/Sensirion/embedded-i2c-scd4x
- ビルドシステム: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-guides/build-system.html
- sdkconfig: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/kconfig.html
- I2C: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/peripherals/i2c.html
- FreeRTOS: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/freertos_idf.html
- SCD4x: https://sensirion.com/media/documents/48C4B7FB/67FE0194/CD_DS_SCD4x_Datasheet_D1.pdf
