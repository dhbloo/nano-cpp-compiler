# Nano Cpp Compiler - NCC

Nano Cpp Compiler（NCC）是一个C++子集的编译器，该子集涵盖了C++的部分基本特性。相比C++03标准，特别去除了以下的语言特性，以简化编译器的实现：

+ 异常处理
+ 多重继承
+ 虚基类
+ 命名空间
+ 模版
+ RTTI
+ C++ Style Cast（`static_cast`、`dynamic_cast`、`reinterpret_cast`、`const_cast`）

总体而言，语言保留的特性主要为C与C++共有的部分，加上C++的面向对象语法与一些C++常见的语言特性（如引用）。

