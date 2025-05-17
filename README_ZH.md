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

本项目依赖以下软件包：

- libcurl
- openssl-dev / libssl-dev
- libjsoncpp
- libzip
- ImageMagick++ (version 7+)
- libmagick++-dev

对于非docker启动的方法，请先安装这些依赖，可以参考 `Dockerfile` 来看（在linux上）如何安装。

无论用什么方法，运行前，请确保Onebot 11标准后端正在运行，运行`shinxbot`后 port 请分别输入Onebot 11后端的上报端口及其API端口，或者在`./config/port.txt`中写入发送/接收端口。（并且 `docker-compose` 里加入映射端口）

以下是启动的三种方法（直接在本机跑不推荐，因为可能有潜在依赖包冲突）：

1. 克隆仓库，`git clone --recursive git@github.com:Jayfeather233/shinxbot2.git` 于根目录下执行`docker compose up -d`进行容器内自动配置并编译为可执行文件`./build/shinxbot`

   使用`docker exec -it shinx-bot bash`以进入容器终端，在容器内接着配置插件功能。（因为容器只编译了框架）

   参考 **从源码编译** 章节来看如何编译功能插件。

   在`./config/port.txt`中写入发送/接收端口。

   最后 `./start.sh` 来开始运行

2. 裸机编译后运行`./build/shinxbot`，详见 **从源码编译**

3. 从Release中下载并解压，运行`shinxbot`（已过时）

可以使用`start.sh`以后台形式运行shinxbot

### 从源码编译

由于包含子模块，克隆仓库请启用`--recursive`

```sh
git clone --recursive git@github.com:Jayfeather233/shinxbot2.git
```

- **Linux**: 执行自动编译脚本`build.sh main`构建所有。(`build.sh simple`跳过编译`libutils`)

- **Windows**: 需要更改TCP连接头文件，以及包括但不限于sleep, file:///等操作系统层面差异（未经验证），推荐在容器中运行

以上只编译了框架，并未包含具体功能，具体功能的代码在 `./src/[functions|events]/`，以下是编译的方法：（`./src/functions/`, `./src/events/` 是两种功能，请都编译）

为编译所有功能，导航至目录 `./src/[functions|events]`，执行 `python generate_cmake.py` 来为所有功能生成 `CMakeLists.txt`，然后执行 `bash ./make_all.sh` 来编译所有功能。

在此之后，编译出的功能动态库应该会在`./lib/[functions|events]/`。

### 添加Bot

具体实现可以参考`./src/bots`中的`mybot`，是已经接好Onebot 11的类

- 所有bot需要继承`./src/interfaces/bot.h`中的`bot`类

- 在`./config/port.txt`中写入发送/接收端口
- 在`./src/main.cpp`的`main()`中加入`add_new_bot(new yourbot(receive_port[id], send_port[id]));`

### 添加功能

你可以参考一个最小开发模块：[shinxbot2\_dev\_module](https://github.com/Jayfeather233/shinxbot2_dev_module)。

或者在本仓库基础上开发：

请确保已经通过执行 `./build.sh` 生成了最新的 `libutils.so`。

以 `class shinxbot` 为例，函数参数仅与消息相关，事件则接受 JSON 数据。当接收到聊天消息时应调用函数中的方法，处理其他提示消息时使用事件中的方法。

开发步骤如下：

1. 在 `./src/functions` 中创建一个新目录，将你的代码放入其中，编写一个继承自 `./src/interfaces/processable.h` 中类的类，实现其中的虚函数，并提供一个 `create` 方法以返回一个 `processable*` 类型的指针（可以参考 `functions/getJapaneseImage/Anime_Img.cpp`）。

2. 如果你继承类中的 `check()` 方法返回成功，则会执行 `process()` 方法。

3. 项目提供了一个 CMake 模板，在 `generate_cmake.py` 中，你可以使用它来编译你的功能模块，动态库将自动生成到 `./lib/functions/` 中。

4. 启动机器人。如果机器人已经在运行，你可以使用管理账户（在 `op_list.json` 中配置）发送命令 `bot.load [function|event] name` 来加载功能模块。

在运行机器人前，请确保你编写的功能动态库已放置在 `./lib/functions` 或 `./lib/events` 中，或使用 `generate_cmake.py` 和 `make_all.sh` 自动生成。

当机器人加载一个库时，会调用 `extern "C" processable *create()` 来获取函数指针。通常只需返回 `new YourFunc()` 即可。

当机器人卸载一个库时，会调用 `extern "C" processable *close(processable* p)` 来表明该库需要被卸载。请正确释放所有通过 `new` 获得的全局指针，包括参数 `p`。


### 开发提示

1. 如果使用clang,根目录下`.clang-format`为你提供了自动格式化设置

2. `dev_tools`下的`sender.sh`可用于向shinxbot发送信息，`inspector`则截获shinxbot向go-cqhttp发送的数据。`inspector`不能与go-cqhttp同时运行。这两者在功能上等同于go-cqhttp，为无法运行go-cqhttp的开发者提供了便利。（你可能需要更改`inspector.cpp`与`sender.sh`中的相关路径指向`config`文件）
   例如：

```bash
bash sender.sh "bot.help"
```
