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

