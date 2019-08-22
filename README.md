# ANGEL
一门动态脚本语言引擎

项目介绍：
本项目由c语言编写

项目结构：
1、AngelRunner：angel脚本运行引擎
2、angel3：angel脚本运行时环境，主要分成引擎核心（AngelCore）、语言扩展（Extension）、对象（Object)和工具（Tools）

环境：
目前支持windows环境，编译环境vs2010

编译：
用vs2010打开angel3.vcxproj并build上面两个项目，输出文件在项目根目录下的Debug文件夹下的Angel.dll和angel5.exe

运行命令是
angel5 [-sh/-c] [angel脚本文件名] -sh运行后进入交互，-c表示只编译生成字节码


具体的语法可以参照example文件夹中的angel脚本测试代码
