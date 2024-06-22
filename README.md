# VEABControl-fw-due-uros
VEABControlShieldをのせたArduino Dueで利用するROS2対応ファームウェア

# ビルド・アップロードする前に
## Arduino IDEの設定
### micro-ROS for Arduinoのインストール
1. [コンパイル済みライブラリのページ](https://github.com/micro-ROS/micro_ros_arduino/releases)からzipファイルをダウンロードする．ROS2ディストリビューションはfoxyとhumbleで動作確認済み．humble推奨．
2. Arduino IDEを開き，「Sketch -> Include library -> Add .ZIP Library...」を選択
3. ダウンロードしたzipファイルを選択して追加

### ビルドオプションの変更
1. Arduino IDEでDueを追加していない場合は，「Tools -> Board -> Boards Manager...」 を開き，検索窓にdueといれて追加する（Aruino SAM Boards...というボード）
2. Arduino Dueのplatform.txtの場所を端末画面（Windowsの場合はCommand Promptで，Linuxの場合はTerminal）で開く．
   - Windows: C:\Users\[ユーザ名]\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.12
   - Ubuntu: /home/[ユーザ名]/.arduino15/packages/arduino/hardware/sam/1.6.12/
3. 以下のコマンドを実行する
```
curl https://raw.githubusercontent.com/micro-ROS/micro_ros_arduino/main/extras/patching_boards/platform_arduinocore_sam.txt > platform.txt
```

### PWM周波数の変更
1. Dueのvariant.hを適当なエディタで開く
   - Windows: C:\Users\[ユーザ名]\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.12\variants\arduino_due_x\variant.h
   - Ubuntu: /home/[ユーザ名]/.arduino15/packages/arduino/hardware/sam/1.6.12/variants/arduino_due_x/variant.h
2. #define PWM_FREQUENCYの値と，#define TC_FREQUENCYの値を変更する．50kHzにする場合は，50000にする

## コードの編集
### VEABを使う場合とPOTを使う場合
- POTを使う場合は，#define POTを残し，#define VEABをコメントアウト．
- VEABを使う場合は，#define VEABを残し，#define POTをコメントアウト．
- トピック名を他と重複しないように設定．POTを使う場合，SUB_TOPICNAMEは無視される．

