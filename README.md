# Nano Cpp Compiler - NCC

### 简介

Nano Cpp Compiler（NCC）是一个C++子集的编译器，该子集涵盖了C++的部分基本特性。相比C++03，去除了许多C++语言特性，以简化编译器的实现。总体而言，语言保留的特性主要为C与C++共有的部分，加上C++的面向对象语法与一些C++常见的语言特性（如引用）。

### 使用

目前有两个程序：

1. 词法分析测试（`./bin/lextest`）
2. 语法分析测试（`./bin/parsetest`）

使用make编译：

```
make lextest
make parsetest
```

这两个程序的输入均为标准输入流。程序的正常运行输出为标准输出流、错误信息输出在标准错误流，输出的中文字符编码为UTF-8（在windows命令行查看时需要先切换代码页`chcp 65001`）

