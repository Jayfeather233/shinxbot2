# shinxBot2

一个基于[go-cqhttp](https://github.com/Mrs4s/go-cqhttp)的机器人。

是我原来项目[shinxBot](https://github.com/Jayfeather233/shinxBot)的C++重构版，

### 它能做什么？

- 恶臭数字论证器，来源：[itorr](https://github.com/itorr/homo)

- 来点二次元。来源：[dmoe](https://www.dmoe.cc)

- 首字母缩写识别，来源：[itorr](https://github.com/itorr/nbnhhsh)

- 在群或私聊中生成一个假的转发信息。

- 来点色图：随机色卡


### 关于源文件

./functions 所有功能的代码

./inc 功能头文件

./interfaces 接口，所有功能继承这个类

./utils 一些工具

### 如何运行这个机器人

需要几个支持库：curl / jsoncpp / openCV

这些库怎么装？问chatGPT。

linux: 运行build.sh，自动执行cmake和make编译并运行

windows：手动通过CMake编译，需要更改TCP连接的头文件（和可能的代码细微差别，包括不限于sleep,file:///）本人没有使用Windows编译过（

### 如何添加新功能

1. ./functions 下新建目录并把代码放入，写一个类继承./interfaces/processable.h中的类并实现virtual方法（可以参考我其他功能的写法）

2. ./functions/functions.h 中加入你的.h

3. main.cpp中main函数里加入functions.push_back(new your_name());