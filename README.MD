# shinxBot2

一个基于 ~~go-cqhttp~~ [Lagrange.Onebot](https://github.com/LagrangeDev/Lagrange.Core) 的机器人以及（轻微的（还没写））多机器人管理框架。

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

- 图片处理：对称、旋转、万花筒

- openai的 `gpt-3.5-turbo` API支持

- google的 `gemini-pro[-vision]` API支持，[url](https://ai.google.dev/docs)

- StableDiffusion的 `SDXL-Turbo` API支持，[url](https://sdxlturbo.ai/)

- 群友聊天截图保存功能（美图）

- 龙王提醒

- 群成员变动提醒

- 人云亦云（复读）

- 不同群聊，私聊的消息互相转发

- 灰名单管理

- 管理多个bot，并（以后）可以通过广播传递消息

### 关于源文件

./src/functions 所有功能的代码

./src/inc 功能头文件

./src/interfaces 接口，所有功能继承这个类

./src/utils 一些工具

./lib/functions/ & ./lib/events/: bot功能的动态链接库目录

### 如何运行这个机器人

需要几个支持库：[libcurl](https://curl.se/libcurl/) + openssl-dev / [jsoncpp](https://github.com/open-source-parsers/jsoncpp) / [base64](https://github.com/tobiaslocker/base64) / [libzip](https://github.com/nih-at/libzip) / [Magick++]()

```
base64: please git clone them into ./lib
libcurl: sudo apt-get install libcurl4-openssl-dev
openssl: sudo apt-get install openssl-dev (or libssl-dev)
jsoncpp: sudo apt-get install libjsoncpp-dev
libzip: sudo apt-get install libzip-dev
Magick++:
    sudo apt-get install libmagick++-dev libmagick++-6.q16hdri-dev
    then remove them (this step actually do dependency install),
    then refer to [Magick++ installing page](https://www.imagemagick.org/Magick++/Install.html) to manually install it. (for a higher version of 7)
maybe it will work
```

确保您的任何Onebot11标准后端正在运行，并有A为其上报端口，B为其api端口。（如果只是测试可以随意填）

Release中下载解压，运行./shinxbot，port行输入 A B。

### 如何编译这个机器人

base64 作为子模块在 ./lib 下，请也一并 clone 它。

在运行bot之前，请确保 `./lib/functions | ./lib/events` 里放入了功能的动态链接库，或者通过 `generate_cmake.py & make_all.sh` 来生成。

linux: 运行 build.sh ，自动执行cmake和make编译，使用 start.sh 开始后台运行。主程序作为管理程序，会以子进程方式启动其他bot。

windows：手动通过CMake编译，需要更改TCP连接的头文件（和可能的代码细微差别，包括不限于sleep,file:///）本人没有使用Windows编译过（

### 如何添加新bot

所有的bot应该继承`./src/interfaces/bot.h`里`class bot`。在`./config/port.txt`中加入你的发送与接收端口。在`./src/main.cpp`的main()中加入`add_new_bot(new yourbot(receive_port[id], send_port[id]));`

可以直接复制`./src/bots`里的`mybot`，是一个已经接好Onebot11的类。

### 如何添加新功能

仅限于class mybot，如果是其他类型bot，可以参考我的功能插件实现。

本节中functions和events可以互换，functions传参仅为消息相关，events传入事件的JSON数据，在收到聊天消息的时候调用 functions 里的方法，在其他提示性消息的时候用 events 的方法。

1. ./src/functions 下新建目录并把代码放入，写一个类继承 `./src/interfaces/processable.h` 中的类并实现virtual方法，并提供`create`方法得到功能`processable*`的指针（可以参考我其他功能的写法，一个简单示例是 `functions/getJapaneseImage/Anime_Img.cpp`）

2. 继承的 virtual 方法里只要 check() 成功了就会执行 process() 方法

3. 在`generate_cmake.py`中有CMake模版，用其编译你的功能，自动生成动态链接库至`./lib/functions/`

4. 启动bot。如果bot正在运行可以使用管理员账号（`op_list.json`）发送`bot.load [function|event] name` 加载。

#### 开发提示

1. 如果使用clang,根目录下`.clang-format`为你提供了自动格式化设置

2. `dev_tools`下的`sender.sh`可用于向shinxbot发送信息，`inspector`则截获shinxbot向go-cqhttp发送的数据。`inspector`不能与go-cqhttp同时运行。这两者在功能上等同于go-cqhttp，为无法运行go-cqhttp的开发者提供了便利。（你可能需要更改`inspector.cpp`与`sender.sh`中的相关路径指向`config`文件）
例如：
```bash
bash sender.sh "bot.help"
```

### TODO

- [x] 把所有的功能编译成动态库，方便更换和加载

- [ ] 实现bot间消息广播
