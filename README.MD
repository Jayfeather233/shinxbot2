# shinxBot2

一个基于[go-cqhttp](https://github.com/Mrs4s/go-cqhttp)的机器人。

~~做着玩的~~

是我原来项目[shinxBot](https://github.com/Jayfeather233/shinxBot)的C++重构版，

### 它能做什么？

- 恶臭数字论证器，来源：[itorr](https://github.com/itorr/homo)

- 来点二次元。来源：[dmoe](https://www.dmoe.cc)

- 首字母缩写识别，来源：[itorr](https://github.com/itorr/nbnhhsh)

- 在群或私聊中生成一个假的转发信息。

- 来点色图：随机色卡

- 赛博养猫（与 gpt3.5 共同创作）

- cat image for http code。来源[httpcats](https://httpcats.com/)

- 图片OCR (qq自带的那个)

- openai的 `gpt-3.5-turbo` API支持

- 群友聊天截图保存功能（美图）

- 龙王提醒

- 群成员变动提醒

- 人云亦云（复读）

- 不同群聊，私聊的消息互相转发

- 灰名单管理

### 关于源文件

./functions 所有功能的代码

./inc 功能头文件

./interfaces 接口，所有功能继承这个类

./utils 一些工具

### 如何运行这个机器人

需要几个支持库：[libcurl](https://curl.se/libcurl/) + openssl-dev / [jsoncpp](https://github.com/open-source-parsers/jsoncpp) / [CImg](https://www.cimg.eu/) / [base64](https://github.com/tobiaslocker/base64) / [libzip](https://github.com/nih-at/libzip)

```
CImg & base64: please git clone them into ./lib
libcurl: sudo apt-get install libcurl4-openssl-dev
openssl: sudo apt-get install openssl-dev (or libssl-dev)
jsoncpp: sudo apt-get install libjsoncpp-dev
CImg: already included
      and sudo apt-get install libjpeg-dev libpng-dev
base64: already included
libzip: sudo apt-get install libzip-dev
maybe it will work
```

CImg 与 base64 作为子模块在 ./lib 下，请也一并 clone 它们。

需要先配置并启动[go-cqhttp](https://github.com/Mrs4s/go-cqhttp)。打开 "反向HTTP POST地址"，并设置你自己的接收端口。（一般在go-cqhttp目录下的config.yaml）

linux: 运行 build.sh ，自动执行cmake和make编译，使用 start.sh 无限循环运行

windows：手动通过CMake编译，需要更改TCP连接的头文件（和可能的代码细微差别，包括不限于sleep,file:///）本人没有使用Windows编译过（

### 如何添加新功能

functions和events可以互换。

functions传参仅为消息相关，events传入事件的JSON数据

在收到聊天消息的时候调用 functions 里的方法，在其他提示性消息的时候用 events 的方法

1. ./functions 下新建目录并把代码放入，写一个类继承 `./interfaces/processable.h` 中的类并实现virtual方法（可以参考我其他功能的写法，一个简单示例是 `functions/getJapaneseImage/Anime_Img.cpp`）

2. 继承的 virtual 方法里只要 check() 成功了就会执行 process() 方法

3. ./functions/functions.h 中加入你的.h

4. main.cpp中main函数里加入 `functions.push_back(new your_name());`

#### 开发提示
1. 如果使用clang,根目录下`.clang-format`为你提供了自动格式化设置
2. `dev_tools`下的`sender.sh`可用于向shinxbot发送信息，`inspector`则截获shinxbot向go-cqhttp发送的数据。`inspector`不能与go-cqhttp同时运行。这两者在功能上等同于go-cqhttp，为无法运行go-cqhttp的开发者提供了便利。（你可能需要更改`inspector.cpp`与`sender.sh`中的相关路径指向`config`文件）
例如：
```bash
bash sender.sh “.ai reset”
```

### TODO

- [ ] 把所有的功能编译成动态库，方便更换和加载

- [x] 把整个`main.cpp`转换成`class bot{}`，更c++
