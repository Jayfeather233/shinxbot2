# shinxBot2

一个基于[go-cqhttp](https://github.com/Mrs4s/go-cqhttp)的机器人。

是我原来项目shinxBot的C++重构版，

### 它能做什么？

- 恶臭数字论证器，来源：[itorr](https://github.com/itorr/homo)

- 来点二次元。来源：[dmoe](https://www.dmoe.cc)

- 首字母缩写识别，来源：[itorr](https://github.com/itorr/nbnhhsh)


### 关于源文件

./functions 所有功能的代码

./inc 功能头文件

./interfaces 接口，所有功能继承这个类

./utils 一些工具

### 如何运行这个机器人

linux: 运行build.sh，自动执行cmake和make编译并运行

windows：手动通过CMake编译

### 如何添加新功能

1. ./functions 下新建目录并把代码放入，写一个类继承./interfaces/processable.h中的类并实现virtual方法（可以参考我其他功能的写法）

2. ./functions/functions.h 中加入你的.h

3. CMake中加入 aux_source_directory(./functions/your_name SOURCES)