```cmake
cmake -S . -B build-cmd -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=mingw32-make -DCMAKE_TOOLCHAIN_FILE="g:/Documents/Projects/codeforce/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_MANIFEST_MODE=ON -DVCPKG_OVERLAY_TRIPLETS="g:/Documents/Projects/codeforce/triplets" -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic-vista -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic-vista
cmake --build build-cmd -j 4
```
# C语言大作业--Codeforces Rating爬虫
## 1.项目介绍
本项目使用C语言开发，加入了`libcurl`,`cJSON`库完成数据获取和数据解析，`vcpkg`和`cmake`作为依赖管理器和构建工具，使用`eChart`进行数据可视化。
本项目提供两个脚本，分别用于windows和Linux平台的编译。
使用`cmake`进行编译后，在命令行执行`./build/codeforce.exe` （或`./build/codeforce`如果使用的不是Windows平台）
程序执行后，在`./resource/`目录下存放我们获取到的资源。

## 2.项目结构
整个项目的目录如下
```
CodeforcesCrawler/
|—— build/ *生成构建产物*
|—— cJSON/ *cJSON，用于解析JSON*
|—— cmake/ *编译时引用到的cmake文件，用于复制共享库等内容*
|—— triplets/ *编译时引用到的cmake文件，用于标志架构*
|—— vcpkg/ *vcpkg，用于管理项目使用到的依赖*
|—— vcpkg.json
|—— src/ *C语言程序目录*
    |—— crawler *爬虫*
    |—— parser *解析json*
    |—— utils *工具类*
    |__ main.c
|—— log/ *日志文件*
|—— .gitignore
|—— .gitmodules
|—— README.md *本说明文档*
|—— run.bat *Windows下的运行脚本*
|—— run.sh *Linux下的运行脚本*

```