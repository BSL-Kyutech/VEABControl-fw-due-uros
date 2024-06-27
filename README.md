# VEABControl-fw-due-uros
VEABControlShieldをのせたArduino Dueで利用するROS2対応ファームウェア

# ビルド・アップロード
## 1. Arduino IDEの設定
### A) micro-ROS for Arduinoのインストール
1. [コンパイル済みライブラリのページ](https://github.com/micro-ROS/micro_ros_arduino/releases)からzipファイルをダウンロードする．ROS2ディストリビューションはfoxyとhumbleで動作確認済み．humble推奨．
2. Arduino IDEを開き，「Sketch -> Include library -> Add .ZIP Library...」を選択
3. ダウンロードしたzipファイルを選択して追加

### B) ビルドオプションの変更
1. Arduino IDEでDueを追加していない場合は，「Tools -> Board -> Boards Manager...」 を開き，検索窓にdueといれて追加する（Aruino SAM Boards...というボード）
2. Arduino Dueのplatform.txtの場所を端末画面（Windowsの場合はCommand Promptで，Linuxの場合はTerminal）で開く．
   - Windows: C:\Users\[ユーザ名]\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.12
   - Ubuntu: /home/[ユーザ名]/.arduino15/packages/arduino/hardware/sam/1.6.12/
3. 以下のコマンドを実行する
```
curl https://raw.githubusercontent.com/micro-ROS/micro_ros_arduino/main/extras/patching_boards/platform_arduinocore_sam.txt > platform.txt
```

### C) PWM周波数の変更
1. Dueのvariant.hを適当なエディタで開く
   - Windows: C:\Users\[ユーザ名]\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.12\variants\arduino_due_x\variant.h
   - Ubuntu: /home/[ユーザ名]/.arduino15/packages/arduino/hardware/sam/1.6.12/variants/arduino_due_x/variant.h
2. #define PWM_FREQUENCYの値と，#define TC_FREQUENCYの値を変更する．50kHzにする場合は，50000にする

## 2. コードの編集
VEABを使う場合とPOTを使う場合で以下の項目を編集する．
- POTを使う場合は，#define POTを残し，#define VEABをコメントアウト．
- VEABを使う場合は，#define VEABを残し，#define POTをコメントアウト．
- トピック名を他と重複しないように設定．POTを使う場合，SUB_TOPICNAMEは無視される．

## 3. PCとの接続
- Programming Port（電源ジャックに近い方のUSB micro-b）を使う．
- VEABControlShieldはのせたままで大丈夫．

# テストする手順
- Ubuntuマシンを用意する
- Dockerをインストールする
- 以下を実行してmicro-ROS agentのためのイメージを準備する
  ```
  sudo docker pull microros/micro-ros-agent:<ディストリビューション名（humbleとか）>
  ```
- マシンにArduino Dueを接続し，dmesgとかでデバイスファイルを特定
- Arduino Dueが何個もあればその数だけ繰り返す（基本的には /dev/ttyACM0, /dev/ttyACM1, ...となる）
- つなげたArduino Dueの数だけ端末を開いておく
- それぞれで以下を実行
  ```
  sudo docker run -it --rm -v /dev:/dev --privileged --net=host microros/micro-ros-agent:<ディストリビューション名（humbleとか）> serial --dev <デバイスファイル名(/dev/ttyACM0とか)>
  ```
- Arduino Dueのリセットスイッチを押すとagentにつながる（トピックが現れる）
- ROS2がインストールされたマシンから`ros2 topic pub`をすると簡単にテストできる．たとえばUInt16MultiArray型のメッセージをpublishしたければ以下の様にする：
  ```
  ros2 topic pub /example_topic std_msgs/msg/UInt16MultiArray "{
  \"layout\": {
    \"dim\": [
      {
        \"label\": \"example\",
        \"size\": 12,
        \"stride\": 12
      }
    ],
    \"data_offset\": 0
  },
  \"data\": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
  }"
  ```
