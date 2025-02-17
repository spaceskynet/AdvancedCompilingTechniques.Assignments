## 2023.10.15

初步搭建docker环境，使用[zhongruoyu/llvm-ports:12-jammy](https://hub.docker.com/r/zhongruoyu/llvm-ports)，编写build脚本和从文件输入到命令行参数的脚本。

由于LLVM编译时开启了`-fno-rtti`，不在`CMakeLists.txt`中加上`-fno-rtti`选项会导致链接错误。

两种方式：

1. 在`add_executable`前加上`add_compile_options(-fno-rtti)`
2. 加上`target_compile_options(ast-interpreter PRIVATE -fno-rtti)`

后面为了防止LLVM版本过高，自己fork了[zhongruoyu/llvm-ports](https://github.com/ZhongRuoyu/llvm-ports)进行LLVM 10的环境搭建。（只能说找到正确对应的CMake版本真折磨，Build镜像的时间也好久）

## 2023.10.16

折腾了很久，LLVM 10.0.1 的 docker 环境[spaceskynet/llvm-10-ports](https://github.com/spaceskynet/llvm-10-ports)终于搭建成功。

设置编译器为clang，此项必须在`project(...)`前。

## 2023.10.20

添加 4 个 extern 函数的C实现，方便测试testcase时直接链接`extern_func.c`生成ELF与ast-interpreter来对拍。

## 2023.10.21

理清 AST 的遍历方式和 LLVM 中各函数的作用，初步实现全局`int`变量的声明、初始化和`=`的整数赋值；通过 `test00`

## 2023.10.22

重新从入口开始设计，尽可能重写Visitor的成员函数，在Visitor完成对AST的遍历；处理全局变量时利用临时栈帧；实现了整数的二元（不包括复合赋值）、一元运算符（包括自增自减）和三目运算符，还有 If-else 语句的解释；通过 class 测试用例 `test03`、`test04`，自己的测试用例 `test00-test05`

## 2023.10.23

实现了 While 和 For 语句的解释，通过 class 测试用例 `test00-11`；实现了整数数组（指针数组实现部分），实现了整数的二元运算符（复合赋值），通过 class 测试用例 `test12`；实现了 Malloc，Free 函数，完成指针和整数的一元、二元运算符混合计算，实现了指针数组，通过 `test17-test19`

## 2023.10.24

完成自定义函数的Call与Return，利用try/catch异常处理实现Return（别忘了给main函数也加上），添加自动测试shell脚本，通过所有测试用例；顺便利用try/catch异常处理实现了循环体内的break和continue，修改命令行参数处理，可以通过-d/--debug开启标准错误流输出，通过-f/--file实现从文件中读取源码（方便通过`lldb`debug）。

## 2023.10.27

删除了不需要的函数，添加了对char变量的支持（将char视作int处理），改正一些错误。

## 2023.11.16

测试时因为助教是使用`docker exec`在容器外进行make，没有进入docker容器内部导致没有加载`.bashrc`，PATH中找不到`clang`和`clang++`，所以修改了`CMakeLists.txt`把`clang`和`clang++`的路径换成了绝对路径，这下才成功通过测试。