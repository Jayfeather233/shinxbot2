# shinxBot2

一个基于[Lagrange.Onebot](https://github.com/LagrangeDev/Lagrange.Core)的(多)机器人管理框架，重构自[shinxBot](https://github.com/Jayfeather233/shinxBot)。

本仓仅包含框架。功能（插件）在 [shinxbot2-plugins](https://github.com/Jayfeather233/shinxbot2-plugins)

[文档](./README_ZH.md)/[Documents](./README.md)

### 文件结构

````
./src/inc            # 功能头文件
./src/interfaces     # 接口，所有功能继承这个类
./src/utils          # 一些工具
./lib/functions/     # bot功能的动态链接库目录
./lib/events/        # bot功能的动态链接库目录
````

### 运行 Bot

**请注意，使用本节内容仅会运行 bot 框架，无任何功能**

本项目依赖以下软件包：

- libcurl
- openssl-dev / libssl-dev
- libjsoncpp
- libzip
- ImageMagick++ (version 7+)
- libmagick++-dev

对于非 Docker 启动的方法，请先安装这些依赖，可以参考 `Dockerfile`（在 Linux 上）如何安装。

无论用什么方法，运行前，请确保 Onebot 11 标准后端正在运行。运行 `shinxbot` 后，port 请分别输入 Onebot 11 后端的上报端口及其 API 端口，或者在 `./config/port.txt` 中写入发送/接收端口。（并且在 `docker-compose` 里加入映射端口）

以下是启动的三种方法（直接在本机跑不推荐，因为可能有潜在依赖包冲突）：

1. 克隆仓库，`git clone --recursive git@github.com:Jayfeather233/shinxbot2.git` 后于根目录执行 `docker compose up -d`，容器内会自动配置并编译为可执行文件 `./build/shinxbot`

   使用 `docker exec -it shinx-bot bash` 进入容器终端，在容器内继续配置插件功能。（容器只编译了框架）

   参考 **从源码编译** 章节，了解如何编译功能插件。

   在 `./config/port.txt` 中写入发送/接收端口。

   最后运行 `./start.sh` 开始运行。

2. 裸机编译后运行 `./build/shinxbot`，详见 **从源码编译**

3. 从 Release 中下载并解压，运行 `shinxbot`（已过时）

可以使用 `start.sh` 以后台形式运行 shinxbot。

### 从源码编译

由于包含子模块，克隆仓库请启用 `--recursive`

```sh
git clone --recursive git@github.com:Jayfeather233/shinxbot2.git
```

- **Linux**: 执行自动编译脚本 `build.sh main` 构建所有。（`build.sh simple` 跳过编译 `libutils`）

- **Windows**: 需要更改 TCP 连接头文件，以及包括但不限于 `sleep`、`file:///` 等操作系统层面差异（未经验证），推荐在容器中运行

以上只编译了框架，并未包含具体功能，具体功能的代码在 [shinxbot2-plugins](https://github.com/Jayfeather233/shinxbot2-plugins)

### 添加 Bot

具体实现可以参考 `./src/bots` 中的 `mybot`，它是已经接好 Onebot 11 的类

- 所有 bot 需要继承 `./src/interfaces/bot.h` 中的 `bot` 类
- 在 `./config/port.txt` 中写入发送/接收端口
- 在 `./src/main.cpp` 的 `main()` 中加入 `add_new_bot(new yourbot(receive_port[id], send_port[id]));`

### 添加功能

你可以参考：[shinxbot2-plugins](https://github.com/Jayfeather233/shinxbot2-plugins)。

### 开发提示

1. 如果使用 clang，根目录下 `.clang-format` 为你提供了自动格式化设置

2. `tools/dev_tools/sender.sh` 可用于向 shinxbot 发送信息，`inspector` 则截获 shinxbot 向 go-cqhttp 发送的数据。`inspector` 不能与 go-cqhttp 同时运行。这两者在功能上等同于 go-cqhttp，为无法运行 go-cqhttp 的开发者提供便利。（你可能需要更改 `inspector.cpp` 与 `sender.sh` 中的相关路径指向 `config` 文件）
   例如：

```bash
bash tools/dev_tools/sender.sh "bot.help"
```

兼容说明：`src/dev_tools/sender.sh` 仍保留为转发脚本。

## 许可证

本项目采用 [LGPLv3](./LICENSE) 许可。如果你修改了本项目的代码，也应当使用 LGPL 许可。

### 插件政策

对于仅编写 plugins 而不修改本项目代码的功能，可使用任何许可。
