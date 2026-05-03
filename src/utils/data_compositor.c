//用cJSON处理数据
#include "cJSON.h"
#include "core.h"
#include <stdlib.h>

void deleteAuthor(cJSON* root){
    int size = cJSON_GetArraySize(root);
    for(int i =0;i<size;i++){
        cJSON* item = cJSON_GetArrayItem(root,i);
        cJSON_DeleteItemFromObject(item, "author");
    }
}

cJSON* parse_data_tree(cJSON** raw_objs) {
    int data_amount; // TODO:现在好像还不是很清楚amount的大小
    cJSON** parsed_data = malloc(sizeof(cJSON*)*data_amount);

    //2.抓取比赛列表
    cJSON* ContestList = cJSON_GetObjectItemCaseSensitive(parsed_data[ContestListData],"result");
    //2.(..)和用户参加的比赛的列表。 3.抓取用户参加比赛的排名，获得分数,各题目的分数
    cJSON* UserAttendResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserRatingData],"result");
    //我直接用这个user.rating的数据，然后在列表里面的每一个比赛中append比赛的详情
    //在这之前，先把user.status爬一下
    cJSON* UserStatusResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserStatusData],"result");
    deleteAuthor(UserAttendResult);

    /*感觉这一部分好难啊，，，数据解析还要自己搓的*/


    //我感觉可以先根据前面的那个UC点Ranking，然后你在user Renton里面读取出来的那个数据去从里面找到看test ID，然后再根据这个contest ID去那个state里面一个一个找
}