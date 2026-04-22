// 日志的话感觉先弄一个简单的文件输出
// 日志同时输出到文件和控制台 按照 [LEVEL] [TIME] MESSAGE 的格式输出日志
// 对外暴露一个接口就行，接收msg和级别
// 级别的话先定义几个常用的，DEBUG INFO WARNING ERROR
// 可以定义一个enum表示日志级别

// 定义不同的日志级别颜色（终端 ANSI 颜色码）
#define COLOR_RESET   "\x1b[0m" // 重置颜色
#define COLOR_INFO    "\x1b[32m" // 绿色
#define COLOR_WARN    "\x1b[33m" // 黄色
#define COLOR_ERROR   "\x1b[31m" // 红色

// 打印错误的宏，包含文件名和行号
// 对外还是暴露那个接口，但是接口内部用这个宏打印到控制台，文件输出的话就直接写到文件里，不需要颜色
#define LOG_INFO(fmt, ...)  fprintf(stdout, COLOR_INFO  "[INFO]  %s:%d: " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fprintf(stdout, COLOR_WARN  "[WARN]  %s:%d: " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, COLOR_ERROR "[ERROR] %s:%d: " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)


/*
TODO：1.首先我要重构一下项目结构了
我觉得在main.c和其他的业务中间应该再套一层controller，main.c就只管启动。controller负责调度
TODO: 2.把执行信息丢给controller，让controller来做出对应行为，或者写日志啥的
但是在这里又有几个问题
1.我的日志的msg怎么办，c好像对字符串的支持不是很好 一个想法是就直接用char*作为结构体的成员了，应该只要能把\0写好就行...（吧？）
2.日志又该有几层呢？
3.我的status结构体能不能在libcurl,cJSON和compositor直接复用呢（他们的字段肯定不完全一样）
# btw ， 这个compositor是跟前端交互的组件，我先这样叫（
TODO: 3.注意到curl本身是有很多关于错误处理的变量的定义的，也有errbuf之类的东西（详见“优化日志和错误处理逻辑”这一篇对话），得想想怎么利用好这玩意
*/

#include "core.h"
typedef enum {
    INFO = 0, // 正常运行时的输出
    WARNING = 1, // 可能会导致问题的输出
    ERROR = 2 // 发生错误时的输出，只能是程序直接崩溃了才能输出ERROR
    // 真的是“程序崩溃才输出ERROR"吗？。？=_= 改了一波之后发现好像不是
    // data里面有东西就不是error怎么样。。？
} LogLevel;

int logger_init(); // 初始化日志文件，主要是打开日志文件，如果失败了就只能输出到控制台了
void log_message(LogLevel level, const char* message);
void detailed_log(Status* status); // 用来详细打印日志
