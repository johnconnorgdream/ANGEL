# ANGEL


## 一门动态脚本语言引擎

## 项目介绍：
本项目由c语言编写，目前只支持windows32/64位系统

## 项目结构：
1、AngelRunner：angel脚本运行引擎
2、angel3：angel脚本运行时环境，主要分成引擎核心（AngelCore）、语言扩展（Extension）、对象（Object)和工具（Tools）

### 编译环境：
vs2010 用vs2010打开angel3.vcxproj并build上面两个项目（右击build），输出文件在项目根目录下的Debug文件夹下的Angel.dll和angel5.exe
在非debug下最好用o2优化选项


#### 运行测试
angel5 [-sh/-c] [angel脚本文件名的全路径] -sh运行后进入交互，-c表示只编译生成字节码不运行，进入shell交互界面可以查看文件中全局变量的值，同时-code可以查看编译的字节码（有些bug）
比如：
angel -sh/-c G:\Youku Files\ANGEL\Example\test.angel

#### 编写ANGEL脚本
具体的例子在example，angel语言语法简洁，易于上手，例子中提供了一些虚拟机的测试脚本和一些经典算法的实现。
