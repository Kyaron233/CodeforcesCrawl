#pragma once

#include <stddef.h>
#include "core.h"

#define BASE_URL "https://codeforces.com/api"


Data getContestList(Status* status); // 获取比赛列表
Data getUserAttendedContestList(Status* status, char* username); // 获取用户参加过的比赛的列表
Data getUserStatus(Status* status, char* username); // 获取用户提交记录列表
Data getUserInfo(Status* status, char* username); // 获取用户基本信息
Data getUserRating(Status* status, char* username); // 获取用户参赛评分记录