# shinxBot2

一个基于[Lagrange.Onebot](https://github.com/LagrangeDev/Lagrange.Core)的(多)机器人管理框架，重构自[shinxBot](https://github.com/Jayfeather233/shinxBot)。

[文档](./README_ZH.md)/[Documents](./README.md)

### 功能支持

- 恶臭数字论证器，来源：[itorr](https://github.com/itorr/homo)

- 来点二次元。来源：[dmoe](https://www.dmoe.cc)

- 首字母缩写识别，来源：[itorr](https://github.com/itorr/nbnhhsh)

- 在群或私聊中生成一个假的转发信息

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

### 文件结构

````
./src/functions		# 所有功能的代码
./src/inc			# 功能头文件
./src/interfaces	# 接口，所有功能继承这个类
./src/utils			# 一些工具
./lib/functions/	# bot功能的动态链接库目录
./lib/events/ 		# bot功能的动态链接库目录
````

### 运行Bot

1. 从Release中下载并解压，运行`shinxbot`

2. 克隆仓库，于根目录下执行`docker compose up -d`进行容器内自动配置并编译为可执行文件`./build/shinxbot`

   使用`docker exec -it shinx-bot bash`以进入容器终端，在容器内运行

3. 裸机编译后运行`./build/shinxbot`，详见下一节

运行前，请确保Onebot 11标准后端正在运行，运行`shinxbot`后port行请分别输入Onebot 11后端的上报端口及其API端口

可以使用`start.sh`以后台形式运行shinxbot

### 从源码编译

由于包含`base64`子模块，克隆仓库请启用`--recursive`

```sh
git clone --recursive git@github.com:Jayfeather233/shinxbot2.git
```

本项目依赖以下软件包：

- libcurl
- openssl-dev / libssl-dev
- libjsoncpp
- libzip
- ImageMagick++ (version 7+)
- libmagick++-dev

具体安装方式可参考`entrypoint.sh`。

- **Linux**: 执行自动编译脚本`build.sh`
- **Windows**: 需要更改TCP连接头文件，以及包括但不限于sleep, file:///等操作系统层面差异（未经验证），推荐在容器中运行

### 添加Bot

具体实现可以参考`./src/bots`中的`mybot`，是已经接好Onebot 11的类

- 所有bot需要继承`./src/interfaces/bot.h`中的`bot`类

- 在`./config/port.txt`中写入发送/接收端口
- 在`./src/main.cpp`的`main()`中加入`add_new_bot(new yourbot(receive_port[id], send_port[id]));`

### 添加功能

以`class mybot`为例，functions传参仅为消息相关，events传入事件的JSON数据，在收到聊天消息的时候调用 functions 里的方法，在其他提示性消息的时候用 events 的方法。

1. ./src/functions 下新建目录并把代码放入，写一个类继承 `./src/interfaces/processable.h` 中的类并实现virtual方法，并提供`create`方法得到功能`processable*`的指针（可以参考`functions/getJapaneseImage/Anime_Img.cpp`）

2. 继承的 virtual 方法里只要 check() 成功了就会执行 process() 方法

3. 在`generate_cmake.py`中有CMake模版，用其编译你的功能，自动生成动态链接库至`./lib/functions/`

4. 启动bot。如果bot正在运行可以使用管理员账号（`op_list.json`）发送`bot.load [function|event] name` 加载。

在运行bot之前，请确保 `./lib/functions | ./lib/events` 里放入了功能的动态链接库，或者通过 `generate_cmake.py & make_all.sh` 来生成。

### 开发提示

1. 如果使用clang,根目录下`.clang-format`为你提供了自动格式化设置

2. `dev_tools`下的`sender.sh`可用于向shinxbot发送信息，`inspector`则截获shinxbot向go-cqhttp发送的数据。`inspector`不能与go-cqhttp同时运行。这两者在功能上等同于go-cqhttp，为无法运行go-cqhttp的开发者提供了便利。（你可能需要更改`inspector.cpp`与`sender.sh`中的相关路径指向`config`文件）
   例如：

```bash
bash sender.sh "bot.help"
```