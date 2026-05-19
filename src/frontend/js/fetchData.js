const OUTPUT_BASE_CANDIDATES = [
    "../../output",
    "../output",
    "/output"
];

async function fetchJsonFromOutput(fileName) {
    let lastError;

    for (const base of OUTPUT_BASE_CANDIDATES) {
        const url = `${base}/${fileName}`;
        try {
            const response = await fetch(url);

            if (response.ok) {
                return await response.json();
            }

            lastError = new Error(`HTTP error! status: ${response.status}`);
        } catch (error) {
            lastError = error;
        }
    }

    throw lastError || new Error("无法获取数据");
}

async function getUsersList(){
    try{
        return await fetchJsonFromOutput("User.json");
    } catch (error) {
        console.error('获取用户列表失败:', error);
        throw error;
    }
}

async function getUserInfo(username){
    try{
        const safeName = encodeURIComponent(username);
        return await fetchJsonFromOutput(`${safeName}_userInfo.json`);
    } catch (error) {
        console.error('获取用户信息失败:', error);
        throw error;
    }
}

async function getContestList(username){
    try{
        const safeName = encodeURIComponent(username);
        return await fetchJsonFromOutput(`${safeName}_contestList.json`);
    } catch (error) {
        console.error('获取比赛列表失败:', error);
        throw error;
    }
}

async function getContestRecord(username){
    try{
        const safeName = encodeURIComponent(username);
        return await fetchJsonFromOutput(`${safeName}_ContestRecord.json`);
    } catch (error) {
        console.error('获取比赛记录失败:', error);
        throw error;
    }
}

async function getLateSubmissions(username){
    try{
        const safeName = encodeURIComponent(username);
        return await fetchJsonFromOutput(`${safeName}_lateSubmissions.json`);
    } catch (error) {
        console.error('获取补交记录失败:', error);
        throw error;
    }
}
