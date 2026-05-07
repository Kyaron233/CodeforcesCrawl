#include "utils/myTime.h"

#include <time.h>

long long timeDistance(long long timestamp) {
    time_t now = time(NULL);
    long long diff;

    if (now == (time_t)-1) {
        return 0;
    }

    diff = (long long)now - timestamp;
    if (diff <= 0) {
        return 0;
    }

    return diff / 86400;
}
