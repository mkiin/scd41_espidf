# 自動light sleep

## 動作

- 起動時にESP-IDFの電源管理を有効にする。CPU周波数は40 MHzから設定済みの最大値まで可変。
- SCD41は起動時に前回の測定完了を待ち、周期測定を停止してpower-downする。
- 測定タスクは初回をすぐに実行し、以後3600秒ごとに単発測定する。
- wake-up後はシリアル番号の読み取りで応答を確認する。単発測定の5秒待ちはvendorドライバーが行う。
- 読み取り後はpower-downし、有効な測定値だけ長さ1のQueueに渡す。CO2=0は破棄。
- 送信タスクはQueueで無期限に待機する。受信時だけWi-Fiを開始し、接続を最大30秒待つ。
- Wi-Fiの開始・接続・停止は送信タスクから順番に呼ぶ。イベントハンドラーは接続状態と停止完了を通知する。
- 切断イベントからの自動再接続は行わず、次の測定周期で接続を試す。EventGroupは各APIへ引数で渡し、mutexやstaticな状態変数は持たない。
- POSTの通信タイムアウトは5秒。HTTPクライアントの接続を閉じ、Wi-Fi停止イベントを最大5秒待つ。
- 測定失敗・接続タイムアウト・送信失敗の回は保存せず、次の測定周期で再試行する。

測定・通信の各タスクは処理中に自身のESP_PM_NO_LIGHT_SLEEPロックを保持する。
両タスクが待機し、ドライバーなどのPM制約もなくなると、ESP-IDFが自動でlight sleepへ入る。
タイマーや他の処理による途中起床はあり得る。独自vote Queue、suspend、手動のsleep呼び出しは使わない。

SCD41停止に失敗した場合はログを残す。センサーが省電力になった保証はできない。
Wi-Fi停止に失敗した場合は通信側のPM lockを保持し、次の測定値を受けたサイクルで停止を再試行する。
停止イベントが遅れて到着した場合も回収できる。停止を確認するまでは自動light sleepを禁止する。

## 設定

sdkconfig.defaultsと現在のローカルsdkconfigに以下を設定する。
既存sdkconfigにはdefaultsだけでは反映されないため、別環境で既存設定を使う場合はmenuconfigでも設定する。

- CONFIG_PM_ENABLE=y
- CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
- CONFIG_ESP_CONSOLE_SECONDARY_NONE=y（ログはUART0を利用）

Application settingsで測定周期と接続待ちを変更できる。

- CONFIG_APP_MEASUREMENT_INTERVAL_SECONDS：既定3600秒、動作試験では30秒などに短縮可能
- CONFIG_APP_NETWORK_WAIT_SECONDS：既定30秒

## 校正とハードウェア

power-down/wake-upを挟む単発測定ではASC（自動校正）は利用できないため、初期化時に無効化する。
設定のEEPROM保存は行わない。長期運用の校正は別途検討する。
温度オフセットは周期測定時と同じとは限らないため、実装基板上で確認する。
センサーとI2Cプルアップには待機中も給電する。センサー電源の物理的な遮断は行わない。

参考：

- [ESP-IDF 5.5.2 電源管理](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/system/power_management.html)
- [SCD4xデータシート 1.7、3.11節](https://sensirion.com/media/documents/48C4B7FB/67FE0194/CD_DS_SCD4x_Datasheet_D1.pdf)

## 検証

ホストでドライバーのNULL引数、測定失敗、読み取り失敗、起床確認、停止失敗を検証：

```sh
cc -std=c11 -Wall -Wextra -Werror \
  -Itests/host/stubs -Icomponents/scd4x/include \
  -Icomponents/scd4x -Icomponents/scd4x/vendor \
  tests/host/test_scd4x_driver.c components/scd4x/scd4x_driver.c \
  -o /tmp/scd41-driver-test
/tmp/scd41-driver-test
```

実機では以下を確認する。ビルドやホストテストだけではsleep突入と消費電流は確認できない。

1. 周期を30秒にして、測定・POST・Wi-Fi停止が複数回継続する。
2. APを停止して接続待ちが終了し、AP復帰後の周期でPOSTが再開する。
3. HTTPサーバー停止時にもWi-Fiを停止して待機へ戻る。
4. センサー読み取り失敗時には送信せず、次の周期へ進む。
5. センサーpower-down後にESP32だけ再起動しても復帰する。
6. UARTでログを確認し、USBデバッグ接続を外した状態で待機電流を測る。
7. 必要ならCONFIG_PM_PROFILINGを有効にし、esp_pm_dump_locks(stdout)で保持ロックを確認する。
8. 周期を3600秒に戻し、実際の1時間待機からの復帰を確認する。
