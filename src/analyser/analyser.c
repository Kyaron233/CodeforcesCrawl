//用cJSON处理数据
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "utils/data_compositor.h"
#include "utils/json_utils.h"
#include "utils/output.h"
#include "utils/logger.h"

// 性能优化：为 contestId 建立时间索引，减少重复扫描
typedef struct {
    int contestId;
    long startTimeSeconds;
    long durationSeconds;
} ContestTimeIndex;



static int compare_submission_by_contest_id(const void* lhs, const void* rhs);
static int compare_contest_time_by_id(const void* lhs, const void* rhs);
static int build_contest_time_index(cJSON* contestList, ContestTimeIndex** out_list, int* out_count);
static const ContestTimeIndex* find_contest_time_index(const ContestTimeIndex* list, int count, int contestId);
static int submission_list_init(SubmissionList* list);
static int submission_list_push_back(SubmissionList* list, const Submission* submission);
static void add_string_or_null(cJSON* obj, const char* key, const char* value);
static cJSON* build_problem_json(const Problem* problem);
static cJSON* build_submission_json(const Submission* submission);
static void append_submission_list_json(cJSON* array, const SubmissionList* list);
static cJSON* build_contest_record_json(const ContestRecord* record);
static void output_contest_records_json(const ContestRecord* records, int count, const char* username);
// static int check_ontime(long updateTime,long deadline); // 从时间上检查是否超时
static int participate_method(cJSON* item);


// 干脆就在这个函数处理得了，顶多返回个status啥的
void parse_and_output(Data* coredata, char* username) {
    cJSON* parsed_data[QueneCount] = {0};
    log_message(INFO,"开始运行data_compositor..");
    if (coredata == NULL || username == NULL) {
        log_message(ERROR,"获取用户名或爬虫数据时出错！");
        return;
    }

    for (int i = 0; i < QueneCount; ++i) {
        if (coredata[i].chunk == NULL || coredata[i].chunk[0] == '\0') {
            log_message(WARNING,"当前parsed_data数组为空");
            parsed_data[i] = NULL;
            continue;
        }
        parsed_data[i] = cJSON_Parse(coredata[i].chunk);
        log_message(INFO,"当前parsed_data数组正常");
    }

    

    //2.抓取比赛列表
    // 下面的contestlist已经直接打印了（在controller里面，直接打印原始字符串）
    cJSON* ContestList = cJSON_GetObjectItemCaseSensitive(parsed_data[ContestListData],"result");
    if (ContestList != NULL) {
        output_json_with_username(ContestList, username, "contestList.json");
    }
    // 性能优化：预先构建 contestId -> (startTime, duration) 索引
    ContestTimeIndex* contest_index = NULL;
    int contest_index_count = 0;
    build_contest_time_index(ContestList, &contest_index, &contest_index_count);

    
        

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
                // 性能优化：通过二分在索引中查找比赛时间
                if (contest_index != NULL && contest_index_count > 0) {
                    const ContestTimeIndex* info = find_contest_time_index(
                        contest_index,
                        contest_index_count,
                        rating_changes[i].contestId
                    );
                    if (info != NULL) {
                        rating_changes[i].startTimeSeconds = info->startTimeSeconds;
                        rating_changes[i].durationSeconds = info->durationSeconds;
                    }
                } 
                else if (cJSON_IsArray(ContestList)) {
                    // 索引构建失败时回退到线性扫描
                    int contestCount = cJSON_GetArraySize(ContestList);
                    for (int j = 0; j < contestCount; ++j) {
                        cJSON* contest = cJSON_GetArrayItem(ContestList, j);
                        int contestId = 0;

                        if (!cJSON_IsObject(contest)) {
                            continue;
                        }

                        if (!json_try_get_int(contest, "id", &contestId)) {
                            continue;
                        }

                        if (contestId != rating_changes[i].contestId) {
                            continue;
                        }

                        json_try_get_long(contest, "startTimeSeconds", &rating_changes[i].startTimeSeconds);
                        json_try_get_long(contest, "durationSeconds", &rating_changes[i].durationSeconds);
                        break;
                    }
                }
            }
        }
    }

    
    cJSON* UserStatusResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserStatusData],"result");
    int submissionCount = cJSON_IsArray(UserStatusResult) ? cJSON_GetArraySize(UserStatusResult):-1;

    //把所有的Submission映射到我们的list中
    Submission *submissionList = NULL;
    if(submissionCount>0){
        submissionList = (Submission*)calloc((size_t)submissionCount,sizeof(Submission));
        for(int i =0;i<submissionCount;i++){
            
            cJSON* item = cJSON_GetArrayItem(UserStatusResult, i);
            cJSON* problem = NULL;
            Submission* submission = &submissionList[i];
            int submission_contest_id = 0;
            int problem_contest_id = 0;

            if (!cJSON_IsObject(item)) {
                continue;
            }

            //TODO ： 把以下内容封装到函数里面
            // Resolve contest id: prefer submission.contestId, fallback to problem.contestId.
            submission->checked = 0;
            json_try_get_int(item, "id", &submission->id);
            json_try_get_int(item, "contestId", &submission_contest_id);
            json_try_get_long(item, "creationTimeSeconds", &submission->creationTimeSeconds);
            json_try_get_long(item, "relativeTimeSeconds", &submission->relativeTimeSeconds);
            submission->participateType = participate_method(item); // 参赛身份已经处理过了

            problem = cJSON_GetObjectItemCaseSensitive(item, "problem");
            if (cJSON_IsObject(problem)) {
                json_try_get_int(problem, "contestId", &problem_contest_id);
                submission->problem.contestId = problem_contest_id;
                json_try_get_string(problem, "index", &submission->problem.index);
                json_try_get_string(problem, "name", &submission->problem.name);
                json_try_get_string(problem, "type", &submission->problem.type);
                json_try_get_double(problem, "points", &submission->problem.points);
                json_try_get_int(problem, "rating", &submission->problem.rating);
            }


            if (submission_contest_id <= 0) {
                submission_contest_id = problem_contest_id;
            }
            submission->contestId = submission_contest_id;

            json_try_get_string(item, "programmingLanguage", &submission->programmingLanguage);
            json_try_get_string(item, "verdict", &submission->verdict);
            json_try_get_int(item, "passedTestCount", &submission->passedTestCount);
            json_try_get_long(item, "timeConsumedMillis", &submission->timeConsumedMillis);
            json_try_get_long(item, "memoryConsumedBytes", &submission->memoryConsumedBytes);
        }
        //TODO ： 把以上内容封装到函数
        char msg[1000];
        sprintf(msg,"%s有%d条提交！",username,submissionCount);
        log_message(WARNING,msg);
        if (submissionCount > 1) {
            qsort(submissionList, (size_t)submissionCount, sizeof(Submission), compare_submission_by_contest_id);
        }
    }

    ContestRecord* contestRecords = NULL;
    if (UserAttendCount >= 0) { // 加个等于，防止有人一场正式比赛不打
        contestRecords = (ContestRecord*)calloc((size_t)submissionCount, sizeof(ContestRecord));
        int nextSubmissionSearchIndex = 0;
        if (contestRecords != NULL) {
            for (int i = 0; i < UserAttendCount; ++i) {
                contestRecords[i].userRating = rating_changes[i];
                submission_list_init(&contestRecords[i].submissions);

                if (submissionList == NULL || submissionCount <= 0 || contestRecords[i].submissions.head == NULL) {
                    continue;
                }

                // 在这之前已经对submission根据contestId进行排序了 ContestRecord不记录不在比赛的提交
                for (int idx = nextSubmissionSearchIndex; idx < submissionCount; ++idx) {
                    // 跳过contestId异常或缺失的提交
                    if (submissionList[idx].contestId <= 0) {
                        continue;
                    }
                    if (submissionList[idx].contestId < rating_changes[i].contestId) {
                        // 性能优化：跳过已小于当前比赛的提交，避免下一轮重复扫描
                        nextSubmissionSearchIndex = idx + 1;
                        continue;
                    }
                    if (submissionList[idx].contestId > rating_changes[i].contestId) 
                        break;
                    
                    submissionList[idx].checked = 1;
                    contestRecords[i].userRating.participateType = submissionList[idx].participateType;
                    //下面判断是不是迟交 relativeTimeSeconds是从比赛开始到该代码提交经过的秒数
                    if (submissionList[idx].relativeTimeSeconds > contestRecords[i].userRating.durationSeconds){
                        submissionList[idx].onTime = 0;
                    }
                    else submissionList[idx].onTime=1;
                    submission_list_push_back(&contestRecords[i].submissions, &submissionList[idx]);
                }
            }
            output_contest_records_json(contestRecords, UserAttendCount, username); // 这里是因为我用的是自己modify的结构体
        }
        // 输出所有迟交和unchecked的提交记录
        if (submissionList != NULL && submissionCount > 0) {
            cJSON* late_submissions = cJSON_CreateArray();
            if (late_submissions != NULL) {
                for (int i = 0; i < submissionCount; ++i) {
                    if (submissionList[i].checked == 0 || submissionList[i].onTime == 0) {
                        cJSON* item = build_submission_json(&submissionList[i]);
                        if (item != NULL) {
                            cJSON_AddItemToArray(late_submissions, item);
                        }
                    }
                }
                output_json_with_username(late_submissions, username, "lateSubmissions.json");
                cJSON_Delete(late_submissions);
            }
        }
    }
    
    

    //先把简单部分做了 主页
    cJSON* UserInfoResult = cJSON_GetObjectItemCaseSensitive(parsed_data[UserInfoData],"result");
    output_json_with_username(UserInfoResult,username,"userInfo.json");

    if (contest_index != NULL) {
        free(contest_index);
    }

    for (int i = 0; i < QueneCount; ++i) {
        if (parsed_data[i] != NULL) {
            cJSON_Delete(parsed_data[i]);
        }
    }
}
// 首先按contestId把contest.list里面的数据排序
static int compare_contest_time_by_id(const void* lhs, const void* rhs) {
    const ContestTimeIndex* a = (const ContestTimeIndex*)lhs;
    const ContestTimeIndex* b = (const ContestTimeIndex*)rhs;

    if (a->contestId < b->contestId) {
        return -1;
    }
    if (a->contestId > b->contestId) {
        return 1;
    }
    return 0;
}

static int build_contest_time_index(cJSON* contestList, ContestTimeIndex** out_list, int* out_count) {
    int contestCount = 0;
    ContestTimeIndex* list = NULL;
    int filled = 0;

    if (out_list == NULL || out_count == NULL) {
        return 0;
    }

    *out_list = NULL;
    *out_count = 0;

    if (!cJSON_IsArray(contestList)) {
        return 0;
    }

    contestCount = cJSON_GetArraySize(contestList);
    if (contestCount <= 0) {
        return 0;
    }

    // 性能优化：一次性读取比赛信息并排序，后续可二分查找
    list = (ContestTimeIndex*)calloc((size_t)contestCount, sizeof(ContestTimeIndex));
    if (list == NULL) {
        return 0;
    }

    for (int i = 0; i < contestCount; ++i) {
        cJSON* contest = cJSON_GetArrayItem(contestList, i);
        int contestId = 0;
        long startTimeSeconds = 0;
        long durationSeconds = 0;

        if (!cJSON_IsObject(contest)) {
            continue;
        }

        if (!json_try_get_int(contest, "id", &contestId)) {
            continue;
        }

        json_try_get_long(contest, "startTimeSeconds", &startTimeSeconds);
        json_try_get_long(contest, "durationSeconds", &durationSeconds);

        list[filled].contestId = contestId;
        list[filled].startTimeSeconds = startTimeSeconds;
        list[filled].durationSeconds = durationSeconds;
        filled += 1;
    }

    if (filled == 0) {
        free(list);
        return 0;
    }

    qsort(list, (size_t)filled, sizeof(ContestTimeIndex), compare_contest_time_by_id);
    *out_list = list;
    *out_count = filled;
    return 1;
}

static const ContestTimeIndex* find_contest_time_index(const ContestTimeIndex* list, int count, int contestId) {
    int left = 0;
    int right = count - 1;

    if (list == NULL || count <= 0) {
        return NULL;
    }

    // 关键步骤：二分查找 contestId 对应的时间信息
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int midId = list[mid].contestId;

        if (midId == contestId) {
            return &list[mid];
        }
        if (midId < contestId) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL;
}

static int compare_submission_by_contest_id(const void* lhs, const void* rhs) {
    const Submission* a = (const Submission*)lhs;
    const Submission* b = (const Submission*)rhs;

    // Sort by resolved contest id to match contest records consistently.
    if (a->contestId < b->contestId) {
        return -1;
    }
    if (a->contestId > b->contestId) {
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
    cJSON_AddNumberToObject(obj,"paticipateType",submission->participateType);

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

static void output_contest_records_json(const ContestRecord* records, int count, const char* username ){
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
    output_json_with_username(root, username,"ContestRecord.json");
    cJSON_Delete(root);
}

static int participate_method(cJSON* item){
    cJSON* author = NULL;
    cJSON* participantType = NULL;
    const char* value = NULL;

    if (!cJSON_IsObject(item)) {
        return CONTESTANT;
    }

    author = cJSON_GetObjectItemCaseSensitive(item, "author");
    if (!cJSON_IsObject(author)) {
        return CONTESTANT;
    }

    participantType = cJSON_GetObjectItemCaseSensitive(author, "participantType");
    if (!cJSON_IsString(participantType) || participantType->valuestring == NULL) {
        return CONTESTANT;
    }

    value = participantType->valuestring;
    if (strcmp(value, "CONTESTANT") == 0) {
        return CONTESTANT;
    }
    if (strcmp(value, "PRACTICE") == 0) {
        return PRACTICE;
    }
    if (strcmp(value, "VIRTUAL") == 0) {
        return VIRTUAL;
    }
    if (strcmp(value, "MANAGER") == 0) {
        return MANAGER;
    }
    if (strcmp(value, "OUT_OF_COMPETITION") == 0) {
        return OUT_OF_COMPETITION;
    }

    return UNKNOWN;
}
