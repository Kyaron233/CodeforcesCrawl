#include<stdio.h>

void utiltest(){
    printf("learing strcuturing....testing utils from src/utils/utils.c \n");
}

int printhelp(){
    printf("Usage: ./crawler [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help            显示帮助信息\n");
    printf("  -U, --username USER   指定Codeforces用户名\n");
    printf("  -M                    多用户，提供一个json列表或者每行一个用户名的列表\n");
    return 0;
}
