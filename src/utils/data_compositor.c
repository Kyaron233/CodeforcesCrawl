//用cJSON处理数据
#include "cJSON.h"
#include <stdlib.h>
#include <time.h>
#include "core.h"
#include "utils/json_utils.h"
#include "utils/output.h"



static int compare_submission_by_contest_id(const void* lhs, const void* rhs);
static int submission_list_init(SubmissionList* list);
static int submission_list_push_back(SubmissionList* list, const Submission* submission);
static void add_string_or_null(cJSON* obj, const char* key, const char* value);
static cJSON* build_problem_json(const Problem* problem);
static cJSON* build_submission_json(const Submission* submission);
static void append_submission_list_json(cJSON* array, const SubmissionList* list);
static cJSON* build_contest_record_json(const ContestRecord* record);
static void output_contest_records_json(const ContestRecord* records, int count, const char* username);
static int check_ontime(long updateTime,long deadline);


// 干脆就在这个函数处理得了，顶多返回个status啥的
void parse_and_output(cJSON** parsed_data,char* username) {

    

    //2.抓取比赛列表
    cJSON* ContestList = cJSON_GetObjectItemCaseSensitive(parsed_data[ContestListData],"result");

    if (ContestList != NULL) 
        output_json_with_username(ContestList, username, "contestList.json");

    //2.(..)和用户参加的比赛的列表。 3.抓取用户参加比赛的排名，获得分数,各题目的分数
    cJSON* UserAttendResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserRatingData],"result");
    int UserAttendCount = cJSON_IsArray(UserAttendResult) ? cJSON_GetArraySize(UserAttendResult) : -1; //用户参加的比赛的计数

    RatingChange* rating_changes = NULL;

    if (UserAttendCount > 0) {
        rating_changes = (RatingChange*)calloc((size_t)UserAttendCount, sizeof(RatingChange)); // calloc相当于malloc加上自动初始化
        if (rating_changes != NULL) {
            for (int i = 0; i < UserAttendCount; ++i) {
                cJSON* item = cJSON_GetArrayItem(UserAttendResult, i);
                if (!cJSON_IsObject(item)) {
                    continue;
                }
                json_try_get_int(item, "contestId", &rating_changes[i].contestId);
                json_try_get_string(item,"contestName",&rating_changes[i].contestName);
                json_try_get_int(item, "rank", &rating_changes[i].rank);
                json_try_get_long(item, "ratingUpdateTimeSeconds", &rating_changes[i].ratingUpdateTimeSeconds);
                json_try_get_long(item, "oldRating", &rating_changes[i].oldRating);
                json_try_get_long(item, "newRating", &rating_changes[i].newRating);
            }
        }
    }

    
    cJSON* UserStatusResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserStatusData],"result");
    int submissionCount = cJSON_IsArray(UserStatusResult) ? cJSON_GetArraySize(UserStatusResult):-1;
    Submission *submissionList = NULL;
    if(submissionCount>0){
        submissionList = (Submission*)calloc((size_t)submissionCount,sizeof(Submission));
        for(int i =0;i<submissionCount;i++){
            cJSON* item = cJSON_GetArrayItem(UserStatusResult, i);
            cJSON* problem = NULL;
            Submission* submission = &submissionList[i];

            if (!cJSON_IsObject(item)) {
                continue;
            }

            submission->onTime = check_ontime();
            json_try_get_int(item, "id", &submission->id);
            json_try_get_int(item, "contestId", &submission->contestId);
            json_try_get_long(item, "creationTimeSeconds", &submission->creationTimeSeconds);
            json_try_get_long(item, "relativeTimeSeconds", &submission->relativeTimeSeconds);

            problem = cJSON_GetObjectItemCaseSensitive(item, "problem");
            if (cJSON_IsObject(problem)) {
                json_try_get_int(problem, "contestId", &submission->problem.contestId);
                json_try_get_string(problem, "index", &submission->problem.index);
                json_try_get_string(problem, "name", &submission->problem.name);
                json_try_get_string(problem, "type", &submission->problem.type);
                json_try_get_int(problem, "points", &submission->problem.points);
                json_try_get_int(problem, "rating", &submission->problem.rating);
            }

            json_try_get_string(item, "programmingLanguage", &submission->programmingLanguage);
            json_try_get_string(item, "verdict", &submission->verdict);
            json_try_get_int(item, "passedTestCount", &submission->passedTestCount);
            json_try_get_long(item, "timeConsumedMillis", &submission->timeConsumedMillis);
            json_try_get_long(item, "memoryConsumedBytes", &submission->memoryConsumedBytes);
        }

        if (submissionCount > 1) {
            qsort(submissionList, (size_t)submissionCount, sizeof(Submission), compare_submission_by_contest_id);
        }
    }

    ContestRecord* contestRecords = NULL;
    if (UserAttendCount > 0) {
        contestRecords = (ContestRecord*)calloc((size_t)UserAttendCount, sizeof(ContestRecord));
        if (contestRecords != NULL) {
            for (int i = 0; i < UserAttendCount; ++i) {
                contestRecords[i].userRating = rating_changes[i];
                submission_list_init(&contestRecords[i].submissions);

                if (submissionList == NULL || submissionCount <= 0 || contestRecords[i].submissions.head == NULL) {
                    continue;
                }

                for (int idx = 0; idx < submissionCount; ++idx) {
                    if (submissionList[idx].problem.contestId > rating_changes[i].contestId) 
                        break;
                    if (submissionList[idx].problem.contestId != rating_changes[i].contestId) 
                        continue;

                    submission_list_push_back(&contestRecords[i].submissions, &submissionList[idx]);
                }
            }

            output_contest_records_json(contestRecords, UserAttendCount, username);
        }
    }
    
    

    //先把简单部分做了 主页
    cJSON* UserInfoResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserInfoData],"result");
    output_json_with_username(UserInfoResult,username,"userInfo.json");
}

static int compare_submission_by_contest_id(const void* lhs, const void* rhs) {
    const Submission* a = (const Submission*)lhs;
    const Submission* b = (const Submission*)rhs;

    if (a->problem.contestId < b->problem.contestId) {
        return -1;
    }
    if (a->problem.contestId > b->problem.contestId) {
        return 1;
    }
    return 0;
}

static int submission_list_init(SubmissionList* list) {
    if (list == NULL) {
        return 0;
    }

    list->head = (SubmissionNode*)calloc(1, sizeof(SubmissionNode));
    if (list->head == NULL) {
        list->size = 0;
        return 0;
    }

    list->head->next = list->head;
    list->head->prev = list->head;
    list->size = 0;
    return 1;
}

static int submission_list_push_back(SubmissionList* list, const Submission* submission) {
    SubmissionNode* node = NULL;

    if (list == NULL || list->head == NULL || submission == NULL) {
        return 0;
    }

    node = (SubmissionNode*)calloc(1, sizeof(SubmissionNode));
    if (node == NULL) {
        return 0;
    }

    node->submission = *submission;
    node->next = list->head;
    node->prev = list->head->prev;
    list->head->prev->next = node;
    list->head->prev = node;
    list->size += 1;
    return 1;
}

static void add_string_or_null(cJSON* obj, const char* key, const char* value) {
    // 统一处理字符串字段缺失的情况
    if (obj == NULL || key == NULL) {
        return;
    }

    if (value != NULL) {
        cJSON_AddStringToObject(obj, key, value);
    } else {
        cJSON_AddNullToObject(obj, key);
    }
}

static cJSON* build_problem_json(const Problem* problem) {
    // 按字段生成Problem对象
    cJSON* obj = NULL;

    if (problem == NULL) {
        return NULL;
    }

    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "contestId", problem->contestId);
    add_string_or_null(obj, "index", problem->index);
    add_string_or_null(obj, "name", problem->name);
    add_string_or_null(obj, "type", problem->type);
    cJSON_AddNumberToObject(obj, "points", problem->points);
    cJSON_AddNumberToObject(obj, "rating", problem->rating);
    return obj;
}

static cJSON* build_submission_json(const Submission* submission) {
    // 按字段生成Submission对象
    cJSON* obj = NULL;
    cJSON* problem = NULL;

    if (submission == NULL) {
        return NULL;
    }

    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "onTime", submission->onTime);
    cJSON_AddNumberToObject(obj, "id", submission->id);
    cJSON_AddNumberToObject(obj, "contestId", submission->contestId);
    cJSON_AddNumberToObject(obj, "creationTimeSeconds", submission->creationTimeSeconds);
    cJSON_AddNumberToObject(obj, "relativeTimeSeconds", submission->relativeTimeSeconds);
    add_string_or_null(obj, "programmingLanguage", submission->programmingLanguage);
    add_string_or_null(obj, "verdict", submission->verdict);
    cJSON_AddNumberToObject(obj, "passedTestCount", submission->passedTestCount);
    cJSON_AddNumberToObject(obj, "timeConsumedMillis", submission->timeConsumedMillis);
    cJSON_AddNumberToObject(obj, "memoryConsumedBytes", submission->memoryConsumedBytes);

    // 嵌套problem结构
    problem = build_problem_json(&submission->problem);
    if (problem != NULL) {
        cJSON_AddItemToObject(obj, "problem", problem);
    } else {
        cJSON_AddNullToObject(obj, "problem");
    }

    return obj;
}

static void append_submission_list_json(cJSON* array, const SubmissionList* list) {
    // 遍历循环双链表并输出Submission数组
    SubmissionNode* node = NULL;

    if (array == NULL || list == NULL || list->head == NULL) {
        return;
    }

    for (node = list->head->next; node != list->head; node = node->next) {
        cJSON* item = build_submission_json(&node->submission);
        if (item != NULL) {
            cJSON_AddItemToArray(array, item);
        }
    }
}

static cJSON* build_contest_record_json(const ContestRecord* record) {
    // 将ContestRecord转换为JSON对象
    cJSON* obj = NULL;
    cJSON* submissions = NULL;

    if (record == NULL) {
        return NULL;
    }

    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "contestId", record->userRating.contestId);
    add_string_or_null(obj, "contestName", record->userRating.contestName);
    cJSON_AddNumberToObject(obj, "rank", record->userRating.rank);
    cJSON_AddNumberToObject(obj, "ratingUpdateTimeSeconds", record->userRating.ratingUpdateTimeSeconds);
    cJSON_AddNumberToObject(obj, "oldRating", record->userRating.oldRating);
    cJSON_AddNumberToObject(obj, "newRating", record->userRating.newRating);

    // 追加Submission列表
    submissions = cJSON_CreateArray();
    if (submissions != NULL) {
        append_submission_list_json(submissions, &record->submissions);
        cJSON_AddItemToObject(obj, "Submission", submissions);
    } else {
        cJSON_AddNullToObject(obj, "Submission");
    }

    return obj;
}

static void output_contest_records_json(const ContestRecord* records, int count, const char* username) {
    // 生成ContestRecord列表并输出为JSON文件
    cJSON* root = NULL;

    if (records == NULL || count < 0 || username == NULL) {
        return;
    }

    // 创建数组根节点
    root = cJSON_CreateArray();
    if (root == NULL) {
        return;
    }

    // 逐个添加ContestRecord对象
    for (int i = 0; i < count; ++i) {
        cJSON* item = build_contest_record_json(&records[i]);
        if (item != NULL) {
            cJSON_AddItemToArray(root, item);
        }
    }

    // 输出并清理
    output_json_with_username(root, username, "contestRecords.json");
    cJSON_Delete(root);
}
static int check_ontime(){
    time_t now = time(NULL);
    if()
}