#pragma once
#define ll long long
#include <curl/curl.h>

typedef struct {
    char* chunk;
    size_t size;
    size_t cap;
} Data;


typedef enum {
    STATUS_OK = 0,
    STATUS_CURL_ERROR,
    STATUS_HTTP_ERROR,
    STATUS_INTERNAL_ERROR
    
} StatusCode;

typedef struct {
    long http_code;
    long redirect_count;
    double total_time;
    curl_off_t size_download;
} Response;

typedef struct {
    StatusCode status;
    CURLcode curl_code;
    const char* msg;
    char curl_error[CURL_ERROR_SIZE];
    Response resp;
} Status;

typedef enum {
    ContestListData = 0,
    UserRatingData,
    UserStatusData,
    UserInfoData,
    QueneCount
} quene;

typedef enum {
    CONTESTANT = 0,
    PRACTICE,
    VIRTUAL,
    MANAGER,
    OUT_OF_COMPETITION,
    UNKNOWN
} ParticipateType;

typedef struct {
    int contestId;
    char* contestName;
    int rank;
    long ratingUpdateTimeSeconds;
    long oldRating;
    long newRating;
    long startTimeSeconds;
    long durationSeconds;
} RatingChange; // userRating就是这个

typedef struct {
    int contestId;
    char* index;
    char* name;
    char* type;
    double points;
    int rating;
} Problem;

typedef struct {
    int onTime; // 等于0就是补交的 这个不属于原来的字段
    int id;
    int contestId;
    long creationTimeSeconds;
    long relativeTimeSeconds;
    Problem problem;
    char* programmingLanguage;
    char* verdict;
    int passedTestCount;
    long timeConsumedMillis;
    long memoryConsumedBytes;
    double points; // API可能为空，未提供时保持默认值
    int participateType;// 参赛类型的一个枚举
} Submission;

typedef struct SubmissionNode {
    Submission submission;
    struct SubmissionNode* prev;
    struct SubmissionNode* next;
} SubmissionNode;

typedef struct {
    SubmissionNode* head;
    int size;
} SubmissionList;

typedef struct {
    int id;
    char* name;
    char* type;
    char* phase;
    int frozen;
    long durationSeconds;
    long startTimeSeconds;
    long relativeTimeSeconds;
} Contest;

typedef struct{
    RatingChange userRating;
    SubmissionList submissions;
} ContestRecord;