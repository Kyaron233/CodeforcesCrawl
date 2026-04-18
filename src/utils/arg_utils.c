#include<stdio.h>

void utiltest(){
    printf("learing strcuturing....testing utils from src/utils/utils.c \n");
}

int printhelp(){
    printf("Usage: ./crawler [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help            显示帮助信息\n");
    printf("  -O, --output FILE     指定输出文件名\n");
    printf("  -U, --username USER   指定Codeforces用户名\n");
    printf("  --mutiple-users FILE  从文件中读取多个用户名，要求每行一个用户名\n");
    return 0;
}
