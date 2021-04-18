#pragma once
//心知天气可能无限免费用，但无法显示历史5天信息，信息量少
//极速数据需要花钱，但能显示5天历史信息
//因经济考虑，两个都留下,不合一. 前者用于显示文字串的天气，后者用于多天，华丽表格式天气。
const String seniverse_key="";    //心知天气 key获得地址:  https://www.seniverse.com/
const String seniverse_city="beijing"; 
const String jisuapi_key="";      //key获得地址: https://www.jisuapi.com/ 极速数据API
const String jisuapi_city="1";    //1 北京

// => Hardware select
#define LILYGO_WATCH_2019_WITH_TOUCH         // To use T-Watch2019 with touchscreen, please uncomment this line
// #define LILYGO_WATCH_2019_NO_TOUCH           // To use T-Watch2019 Not touchscreen , please uncomment this line
// #define LILYGO_WATCH_BLOCK                   // To use T-Watch Block , please uncomment this line
// #define LILYGO_WATCH_BLOCK_V1                // To use T-Watch Block V1 , please uncomment this line
// #define LILYGO_WATCH_2020_V1                 // To use T-Watch2020 V1, please uncomment this line
// #define LILYGO_WATCH_2020_V2                 // To use T-Watch2020 V2, please uncomment this line
// #define LILYGO_WATCH_2020_V3                 // To use T-Watch2020 V3, please uncomment this line

#include <LilyGoWatch.h>
