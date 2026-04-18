#define BASE_URL "https://codeforces.com/api"

void setup(); // 这里用来初始化timeout之类的参数，因为reset之后这些参数也会被抹掉
void getMatchList(); // 获取比赛列表