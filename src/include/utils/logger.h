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

typedef enum {
    INFO, // 正常运行时的输出
    WARNING, // 可能会导致问题的输出
    ERROR // 发生错误时的输出，只能是程序直接崩溃了才能输出ERROR
} LogLevel;

void log_message(LogLevel level, const char* message);