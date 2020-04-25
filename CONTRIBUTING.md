# 项目贡献指南

## 项目结构

项目结构参考了两个结构良好的 C++ 项目: [LLVM](https://github.com/llvm/llvm-project) 和 [SVF](https://github.com/SVF-tools/SVF). 主要包括以下八个部分:

```plain
- benchmark  # 测试评估基准
- docs       # 项目文档
- examples   # 基础平台使用示例代码
- include    # 项目头文件
  |- CFG
  |- SSA
  |- VFG
  |- UAF
  |- NPE
  |- ...
- lib        # 基础平台源文件
  |- CFG
  |- SSA
  |- VFG
  |- ...
- tools      # 在基础平台之上构建的分析, 检测, 修复等工具
  |- UAF
  |- NPE
  |- ...
- tests      # 回归测试
- unittests  # 单元测试
```

## 构建系统

项目基于 [CMake](https://cmake.org/) 和 [Ninja](https://github.com/ninja-build/ninja) 两个工具进行构建.

使用 CMake 时, **不建议直接在项目根目录下进行项目构建**. 一般会在项目根目录下创建子目录 `cmake-build-debug`, 然后在该子目录下进行项目构建 (这也是基于 CMake 的 CLion IDE 的默认构建行为).

Ninja 是一个小而精的构建系统, 作用与 Make 类似. 相对于 Make 而言, Ninja 更注重于编译速度. Make 在编译一些大项目时速度会比较慢, 现在 Chrome, LLVM 等大型项目都开始使用 Ninja 进行构建.

CMake 提供了 Ninja 的生成器选项 (`-G Ninja`), 可以自动为我们生成 Ninja 的配置文件, 所以不需要我们手动编写, 我们只需要知道如何使用 Ninja 命令即可. 以下是 CMake 和 Ninja 命令的使用示例:

```shell
# 当前目录为项目根目录
$ cd cmake-build-debug
# 指定 CMake 的生成器 (generator) 为 Ninja, '..' 表示项目根目录与当前目录的相对位置
$ cmake -G Ninja ..
# 使用 ninja 编译整个项目所有目标 (target)
$ ninja
# 使用 ninja 编译 (多个) 特定目标 (target)
$ ninja target1 [target2 ...]
```

### CMake 使用惯例

1. 根目录下的 `CMakeLists.txt` 文件为主入口. 一般会在这里设置库目录 `link_directories(...)`, 头文件目录 `include_directories(...)`, 需要添加的源码子目录 `add_subdirectory(...)`, 以及一些其他通用配置.
   1. `link_directories` 作用相当于编译命令中的 `-L` 参数;
   2. `include_directories` 作用相当于编译命令中的 `-I` 参数.
   3. CMake 支持多级目录嵌套. 嵌套的每一级子目录都需要一个 `CMakeLists.txt` 文件, 并在其父目录的 `CMakeLists.txt` 文件中以 `add_subdirectory(sub_dir)` 的方式引入, 其中 `sub_dir` 为子目录名.
2. 一般情况下, `include` 目录中不需要添加 `CMakeLists.txt` 文件, 只需要在根目录下的 `CMakeLists.txt` 中使用命令 `include_directories(include)` 即可.
3. `lib` 子目录为库文件源码目录, 在根目录下的 `CMakeLists.txt` 文件中以 `add_subdirectory(lib)` 引入.
   1. 一般不会直接在 `lib` 目录下写代码, 而是以库为单位创建不同的子目录, 每个子目录以 `add_subdirectory` 方式在 `lib/CMakeLists.txt` 中引入.
   2. 在每个库文件子目录的 `CMakeLists.txt` 中添加库文件: `add_library(lib_name ${src_files})`, 其中 `lib_name` 为库名, `${src_files}` 为编译库文件所需的所有源文件.
4. `tools` 子目录为可执行文件源码目录, 在根目录下的 `CMakeLists.txt` 文件中以 `add_subdirectory(tools)` 引入.
   1. 一般不会直接在 `tools` 目录下写代码, 而是以可执行文件为单位创建不同的子目录, 每个子目录以 `add_subdirectory` 方式在 `tools/CMakeLists.txt` 中引入.
   2. 在每个可执行文件子目录的 `CMakeLists.txt` 中添加可执行文件: `add_executable(exec_name ${src_files})`, 其中 `exec_name` 为可执行文件名, `${src_files}` 为编译可执行文件所需的所有源文件.
   3. 编译可执行文件时还需要指定链接库. 可以通过 `target_link_libraries(exec_name ${libs})` 的方式指定, 其中 `exec_name` 为可执行文件名, `${libs}` 为链接可执行文件所需的所有库. 需要注意的是 `${libs}` 中多个库的顺序. 链接时将按照这里指定的库顺序进行链接, 若顺序不当, 将可能导致链接错误.

对 CMake 不熟悉的同学可参考项目中已有的 `CMakeLists.txt` 文件, 也可参考 CMake 官方文档: [CMake Documentation](https://cmake.org/cmake/help/latest/).

## 编码风格

### 所有语言

1. 使用空格缩进, 不要使用 `<tab>`;
2. 使用 UTF-8 编码;
3. 使用 LF 换行符, 不要使用 CRLF;
4. 每行不超过 80 个字符.

### C/C++

使用 clang-format (参考项目根目录下的 [.clang-format](.clang-format) 配置文件).

.clang-format 配置文件遵循 [YAML](https://yaml.org/) 语法. 配置文件中各项含义可参考官方文档: [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html), 也可参考这篇博客: [Clang-Format 格式化选项介绍](https://blog.csdn.net/softimite_zifeng/article/details/78357898).

clang-format 支持 5 种标准代码规范: [LLVM](https://llvm.org/docs/CodingStandards.html), [Google](https://google.github.io/styleguide/cppguide.html), [Chromium](https://chromium.googlesource.com/chromium/src/+/master/styleguide/styleguide.md), [Mozilla](https://developer.mozilla.org/en-US/docs/Developer_Guide/Coding_Style) 和 [WebKit](https://www.webkit.org/coding/coding-style.html), 默认使用 LLVM 风格.

clang-format 也支持使用自定义代码规范. 自定义规范位于当前目录或任一父目录的文件 .clang-format 或 _clang-format 中. **一般会将 .clang-format 文件放在项目根目录下**.

clang-format 命令基本用法:

```shell
# 预览规范后的代码
$ clang-format main.cpp
# 直接在原文件上规范代码
$ clang-format -i main.cpp
# 批量规范某个子目录下的所有代码
$ clang-format -i lib/framework/*.cpp
# 显示指明代码规范, 默认为 LLVM
$ clang-format -style=google main.cpp
# 将指定风格的代码规范配置信息写入文件 .clang-format
$ clang-format -style=google -dump-config > .clang-format
```

完整用法可参考官方文档: [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html), 或使用 `clang-format --help`.

## 用户文档

请使用 [Markdown](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) 语法编写文档.

Markdown 编码规范可以参考 [markdownlint](https://github.com/DavidAnson/markdownlint/blob/master/doc/Rules.md).
