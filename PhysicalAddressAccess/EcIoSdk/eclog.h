#pragma once
#include <vector>
#include <functional>
#include <windows.h>

#ifdef ECLOG_EXPORTS
#define MI_DLL_API __declspec(dllexport)
#else
#define MI_DLL_API __declspec(dllimport)
#endif




enum EC_LOG_STATE {
    EC_SUCCESS,
    EC_FAILED,
    EC_NEED_NOT_READ,
    EC_CREATE_FILE_FAILED
};



MI_DLL_API int GetECLogPageCount();
MI_DLL_API EC_LOG_STATE GetECLog(std::vector<std::vector<UCHAR>>& pVecECLog,bool isPassive = false);
MI_DLL_API void FreeECLog(std::vector<std::vector<UCHAR>>& pVecECLog);

//set callBack to NULL,means close eclog ready check thread,if you want to save some resources,just do it
MI_DLL_API bool RegisterECLogReadyNotifier(std::function<void()> callBack);