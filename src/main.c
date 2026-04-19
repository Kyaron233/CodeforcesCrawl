#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils/logger.h"
#include "crawler/crawler.h"
#include <stdlib.h>
#include <getopt.h>
#include <curl/curl.h>
#include "utils/arg_utils.h"

// 处理选项这一块让gemini干的
static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"output", required_argument, 0, 'O'},
    {"username", required_argument, 0, 'U'},
    {0, 0, 0, 0}
};
// 我把用户名和文件名拿出来了 感觉不能放在栈上
char username[200]; // 200应该够了吧。。
char output_filename[200]; // 也是200。注意这个文件名可能是存在的也可能是不存在的
int main(int argc, char *argv[]) {
    int opt;

    while((opt = getopt_long(argc, argv, "hO:U:", long_options, NULL)) != -1) {
        switch(opt) {
            case 'h':
                printhelp();
                return 0;
            case 'O':
                // 处理输出文件参数
                strncpy(output_filename, optarg, sizeof(output_filename) - 1);
                output_filename[sizeof(output_filename) - 1] = '\0';
                break;
            case 'U':
                strncpy(username, optarg, sizeof(username) - 1);
                username[sizeof(username) - 1] = '\0';
                break;
            default:
                fprintf(stderr, "未知选项：%c\n。使用--help查看帮助信息", opt);
                return 1;
        }
    }
    // 这里的logger_init主要是能否成功打开日志文件，失败了就只能输出到控制台了
    // 日志放在log/log.txt，按照规范输出日志。使用a模式打开文件，所有的内容都追加到文件末尾
    if(logger_init()==NULL) {
        fprintf(stderr, "日志文件初始化失败，使用控制台输出\n");
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

}