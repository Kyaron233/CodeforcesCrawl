# C语言大作业--Codeforces爬虫
## 1.项目介绍
本项目使用C语言开发，加入了`libcurl`,`cJSON`库完成数据获取和数据解析，`vcpkg`和`cmake`作为依赖管理器和构建工具，使用简单的html网页和`eChart`进行数据可视化。
本项目提供两个脚本，分别用于windows和Linux平台的编译。
使用`cmake`进行编译后，在命令行执行`.\codeforcesCrawler.exe` （或`./build/codeforcesCrawler`如果使用的不是Windows平台）
程序执行后，在`./output`目录下生成数据
如果需要可视化展示，需要使用`server_win.exe`或者`server_linux`，你也可以使用`liveserver`之类的插件来挂载网页
##### 请不要*直接打开index.html*，否则会因为协议不同，导致网页无法获取到之前爬取到的数据

## 2.如何运行？
如果目录下已有codeforcesCrawler.exe或者codeforcesCrawler（linux的可执行文件），直接运行即可。（Linux执行前请赋予执行权限）
如果要从头编译，请根据你的电脑修改CMakeLists.txt中的宏（如DCMAKE_C_COMPILER）,然后再执行下面的命令
Windows:
```cmake
cmake --preset mingw-vcpkg
cmake --build --preset mingw-vcpkg-build
# Windows下由于使用vcpkg需要编译的内容较多，编译时间较长，建议使用
```
Linux:
```cmake 
cmake --preset linux-vcpkg
cmake --build --preset linux-vcpkg-build
```
## 3.项目结构
整个项目的目录如下
```
CodeforcesCrawler/
|—— .github 
|—— .vscode
|—— build/ *构建产物*
|—— cJSON/ *cJSON，用于解析JSON*
|—— cmake/ *编译时引用到的cmake文件，用于复制共享库等内容*
|—— triplets/ *vcpkg引用到的triplets内容，只在Windows环境使用*
|—— vcpkg/ *vcpkg，本项目使用的C语言库管理器*
|—— vcpkg.json
|—— src/ *C程序目录*
    |—— crawler *爬虫*
    |—— analyser *获取爬取到数据，并整合、重组*
    |—— include 
    |—— utils *工具类*
    |__ main.c
    |—— frontend *前端目录*
        |—— js
        |—— index.html *网页*
        |—— styles.css
|—— .gitignore
|—— .gitmodules *子模块*
|—— README.md *本说明文档*
```



## 4.项目逻辑
本项目后端部分进行了模块设计，`main`模块和`controller`模块负责管理启动和调度，`crawler`和`analyser`各司其职进行数据爬取和数据分析。`cJSON`作为json数据的处理工具，在`utils`中引入，供各个模块使用
逻辑如图所示：
项目爬取`contest.list`，`user.rating`，`user.status`三个api，最终生成`{handle}_contestList.json`,`{handle}_lateSubmissions.json`,`{handle}_ContestRecord.json`,`{handle}_userInfo.json`四个文件


## 5.各模块具体实现
1. 爬虫模块crawler
该模块维护一个全局的curl句柄 `static CURL *global_curl `，每次执行任务前都会检查`global_curl`是否为NULL，如果不为NULL则直接复用，否则NULL则调用`getCurl()`获取curl句柄并赋值给全局句柄
本模块的`write_callback()`函数在分配数组时动态配置大小，根据userdata.cap来配置大小，当需要扩容时分配新空间并复制（每次扩容当前需要额外写入的空间并加16b），然后使原来的数据指向新的空间。在任务完成后还会配合`shrink_data()`来回收没有使用的空间。
在执行请求前，先初始化Data数据数组和日志，然后获取curl，获取到之后先调用`set_custom_option()` 填充请求参数，然后填充请求地址等具体参数，然后调用`curl_easy_perform()`执行请求 。再根据curl状态进行日志填充，最后处理获取到的数据、返回数据
2. 分析模块analyser
首先为各个模块初始化cJSON*，统一放在一个列表`parsed_data`里面。通过枚举中的值来访问
然后根据各个api内容，获取对应key的值，并使用cJSON解析到parsed_data里面。在解析的同时还会统计一些后续需要的信息，比如~迟交~(该信息已不会在前端展示已经弃用)，180天内的提交信息等
在处理`user.rating`和`user.status`两个api的数据时，还定义了`Submission`,`RatingChange`，`Problem`等结构体。方便后续进行数据处理。
组织完数据之后，按照要求对数据进行排序，然后调用cJSON库等工具函数把数据写入json。



## 6.反思与展望 
项目虽然基本完成，但也存在如下的问题：
1. 冗余设计较多
    部分代码功能类似，比如爬虫模块的getXXXX，每个部分都单独写了一个函数，并交由controller统一调用，设计上有点冗杂。并且由于“传递式”的设计，导致通过传参来复用一个函数也较难实现
    此外，虽然整体上使用了模块化的设计，但存在部分函数位置乱放，比如`crawl_data()`函数并未放在crawl模块而是放在controller模块来调用
    还有变量命名也存在结构混用、命名不直观不易懂的问题
2. 结构较混乱
    对codeforces系统不了解（比如不知道用户名其实是handle，对“补题”的概念不太理解等）
3. 日志系统实用性差
    最初设计的第一部分就是日志系统，因为认为整个项目会比较复杂，需要打很多日志来debug，设计了三个等级，并指定了文件来保存日志。还规定了日志的规范，比如ERROR必须是程序执行到必须退出的地步时才能调用等。
    但是当我设计完日志系统开始写crawler部分之后才发现这部分设计还不够，我还专门写了一个crawler日志的结构体，写到后面的部分发现这样也不够，并且三个等级我也没有严格遵守....
    总之在设计一个好用的日志系统这一块，这个项目还远远不够。
总之这个程序还有很多可以优化的地方，比如日志系统、数据分析器的性能优化、GUI界面等。设计上也有很多欠妥，恳请老师提出指导。
此外在完成项目的过程中在各个模块都使用了ai进行辅助，善用AI的确为我提供了很多帮助。部分使用ai完成的场景在git的历史提交中也有注明。