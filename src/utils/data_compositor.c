//用cJSON处理数据
#include "cJSON.h"
#include <stdlib.h>
#include <time.h>
#include "core.h"
#include "utils/json_utils.h"
#include "utils/output.h"

time_t now = 0;  // 当前时间戳（秒），使用标准库 time() 函数赋值


// 干脆就在这个函数处理得了，顶多返回个status啥的
void parse_data(cJSON** parsed_data) {

    now = time(NULL);

    //2.抓取比赛列表
    cJSON* ContestList = cJSON_GetObjectItemCaseSensitive(parsed_data[ContestListData],"result");
    
    //2.(..)和用户参加的比赛的列表。 3.抓取用户参加比赛的排名，获得分数,各题目的分数
    cJSON* UserAttendResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserRatingData],"result");
    int UserAttendCount = cJSON_IsArray(UserAttendResult) ? cJSON_GetArraySize(UserAttendResult) : 0; //用户参加的比赛的计数

    RatingChange* rating_changes = NULL;

    if (UserAttendCount > 0) {
        rating_changes = (RatingChange*)calloc((size_t)UserAttendCount, sizeof(RatingChange));
        if (rating_changes != NULL) {
            for (int i = 0; i < UserAttendCount; ++i) {
                cJSON* item = cJSON_GetArrayItem(UserAttendResult, i);
                if (!cJSON_IsObject(item)) {
                    continue;
                }
                json_try_get_int(item, "contestId", &rating_changes[i].contestId);
                rating_changes[i].contestName = json_dup_string(item, "contestName");
                json_try_get_int(item, "rank", &rating_changes[i].rank);
                json_try_get_long(item, "ratingUpdateTimeSeconds", &rating_changes[i].ratingUpdateTimeSeconds);
                json_try_get_long(item, "oldRating", &rating_changes[i].oldRating);
                json_try_get_long(item, "newRating", &rating_changes[i].newRating);
            }
        }
    }

    
    cJSON* UserStatusResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserStatusData],"result");
    

    //先把简单部分做了 主页
    cJSON* UserInfoResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserInfoData],"result");


}