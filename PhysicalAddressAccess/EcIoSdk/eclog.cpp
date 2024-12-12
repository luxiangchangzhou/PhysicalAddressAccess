
#define _WIN32_DCOM
#include <iostream>
#include <thread>
#include <comdef.h>
#include <Wbemidl.h>
#include <Setupapi.h>
#include <Ndisguid.h>
#include "eclog.h"
#include "eventsink.h"
#include "../XiaomiEcIo/PhysicalAddressAccess.h"

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib,"Setupapi.lib")

#define MAX_CHECK_COUNT 10
#define SLEEPTIME 20


static const GUID GUID_DEVINTERFACE_ECIO =
{ 0x981EF630, 0x72B2, 0x11d2, { 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x61, 0x71 } };


bool IsECIdle(HANDLE hDev) {
    unsigned char buf[8] = { 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
    accessRequest.accessCount = 8;
    //判断EC IDLE
    BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                               buf, 8, &bytesAccessed, 0);

    if (ret) {
        //EC IDLE
        if (*(PULONG)buf == 0x0) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }

}

bool IsErrorOccur(HANDLE hDev) {
    unsigned char buf[8] = { 0 };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
    accessRequest.accessCount = 8;
    //判断EC IDLE
    BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                               buf, 8, &bytesAccessed, 0);
    if (ret) {
        //EC IDLE
        if (*(PULONG)buf == 0x011f1f52) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }

}

bool ErrorAck(HANDLE hDev) {
    unsigned char buf[8] = { 0x52, 0x1f, 0x1f, 0x02, 0xff, 0xff, 0xff, 0xff };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    *(PULONG)buf = 0x01010152;
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
    accessRequest.accessCount = 4;
    BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                               buf + 4, 4, &bytesAccessed, 0);
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
    accessRequest.accessCount = 4;
    ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                          buf, 4, &bytesAccessed, 0);
    if (ret) {
        return true;
    } else {
        return false;
    }

}


bool SetECIdle(HANDLE hDev) {
    unsigned char buf[8] = { 0 };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
    accessRequest.accessCount = 4;
    BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                               buf + 4, 4, &bytesAccessed, 0);
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
    accessRequest.accessCount = 4;
    ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                          buf, 4, &bytesAccessed, 0);
    if (ret) {
        return true;
    } else {
        return false;
    }

}


bool IsLogPageReady(UCHAR page, HANDLE hDev) {
    unsigned char buf[8] = { 0, 0, 0, 0, 0, 0xff, 0xff, 0xff };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
    accessRequest.accessCount = 8;

    BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                               buf, 8, &bytesAccessed, 0);

    if (ret) {
        //log ready
        if (*(PULONG)buf == 0x02020252 && buf[4] == page) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }

}

bool ECInformedShouldRead() {
    return true;
}

void FreeECLog(std::vector<std::vector<UCHAR>>& pVecECLog) {
    pVecECLog.clear();
}


HANDLE GetDeviceViaInterface(GUID* pGuid, DWORD instance)
{
    // Get handle to relevant device information set
    HDEVINFO info = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (info == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    // Get interface data for the requested instance
    SP_INTERFACE_DEVICE_DATA ifData;
    ifData.cbSize = sizeof(ifData);
    if (!SetupDiEnumDeviceInterfaces(info, NULL, pGuid, instance, &ifData))
    {
        SetupDiDestroyDeviceInfoList(info);
        return NULL;
    }

    // Get size of symbolic link name
    DWORD reqLen;
    SetupDiGetDeviceInterfaceDetail(info, &ifData, NULL, 0, &reqLen, NULL);
    PSP_INTERFACE_DEVICE_DETAIL_DATA ifDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)(new char[reqLen]);
    if (ifDetail == NULL)
    {
        SetupDiDestroyDeviceInfoList(info);
        return NULL;
    }

    // Get symbolic link name
    ifDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(info, &ifData, ifDetail, reqLen, NULL, NULL))
    {
        SetupDiDestroyDeviceInfoList(info);
        delete ifDetail;
        return NULL;
    }

    // Open file
    HANDLE hDev = CreateFile(ifDetail->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev == INVALID_HANDLE_VALUE) hDev = NULL;

    delete ifDetail;
    SetupDiDestroyDeviceInfoList(info);
    return hDev;
}

int GetECLogPageCount()
{
    HANDLE hDev = CreateFile(L"\\\\.\\PhysicalAddressAccess-D2DEBE83-AA54-4DFC-AA7E-2160938BEB88", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    int status = -1;
    unsigned char logBuf[256] = { 0 };
    unsigned char buf[8] = { 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    bool ret;

    if (hDev != INVALID_HANDLE_VALUE) {
        //SetECIdle(hDev);
        for (int checkECIdleCounter = 0; checkECIdleCounter < MAX_CHECK_COUNT; checkECIdleCounter++) {
            if (IsECIdle(hDev)) {
                checkECIdleCounter = MAX_CHECK_COUNT;//关闭循环
                //此命令为通知EC将读取PageCount
                *(PULONG)buf = 0x01010152;
                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                accessRequest.accessCount = 4;
                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                    buf + 4, 4, &bytesAccessed, 0);
                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                accessRequest.accessCount = 4;
                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                    buf, 4, &bytesAccessed, 0);
                for (int checkPageCountReadyCounter = 0; checkPageCountReadyCounter < MAX_CHECK_COUNT; checkPageCountReadyCounter++) {
                    //读取PageCount_recorded，current_block，current_page，current_index
                    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                    accessRequest.accessCount = 8;
                    ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                        buf, 8, &bytesAccessed, 0);
                    if (*(PULONG)buf == 0x02010152) {
                        checkPageCountReadyCounter = MAX_CHECK_COUNT;//关闭循环

                        UCHAR pageCountRecorded = buf[4];
                        UCHAR currentBlock = buf[5];
                        UCHAR currentPage = buf[6];
                        UCHAR currentIndex = buf[7];

                        //通知EC读取PageCount完成
                        *(PULONG)buf = 0x03010152;
                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                        accessRequest.accessCount = 4;
                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                            buf + 4, 4, &bytesAccessed, 0);
                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                        accessRequest.accessCount = 4;
                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                            buf, 4, &bytesAccessed, 0);


                        status = pageCountRecorded;



                    }
                    else { //checked
                        if (IsErrorOccur(hDev)) {
                            ErrorAck(hDev);
                            break;
                        }
                        Sleep(SLEEPTIME);
                        continue;
                    }
                }
            }
            else { //checked
                Sleep(SLEEPTIME);
                continue;
            }

        }
        CloseHandle(hDev);
    }
    else {
        status = -1;
    }
    return status;
}



EC_LOG_STATE GetECLog( std::vector<std::vector<UCHAR>>& pVecECLog, bool isPassive) {
    //一次最多读4k


    //HANDLE hDev = GetDeviceViaInterface((GUID*)&GUID_DEVINTERFACE_ECIO, 0);//WDM 
    HANDLE hDev = CreateFile(L"\\\\.\\PhysicalAddressAccess-D2DEBE83-AA54-4DFC-AA7E-2160938BEB88", 0/*GENERIC_ALL*/, 0, NULL, OPEN_EXISTING, 0, NULL);
    EC_LOG_STATE status = EC_FAILED;
    unsigned char logBuf[256] = { 0 };
    unsigned char buf[8] = { 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff };
    DWORD bytesAccessed = 0;
    PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
    bool ret;

    if (hDev != INVALID_HANDLE_VALUE) {
        //SetECIdle(hDev);
        for (int checkECIdleCounter = 0; checkECIdleCounter < MAX_CHECK_COUNT; checkECIdleCounter++) {
            if (IsECIdle(hDev)) {
                checkECIdleCounter = MAX_CHECK_COUNT;//关闭循环
                //此命令为通知EC将读取PageCount
                *(PULONG)buf = 0x01010152;
                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                accessRequest.accessCount = 4;
                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                      buf + 4, 4, &bytesAccessed, 0);
                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                accessRequest.accessCount = 4;
                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                      buf, 4, &bytesAccessed, 0);
                for (int checkPageCountReadyCounter = 0; checkPageCountReadyCounter < MAX_CHECK_COUNT; checkPageCountReadyCounter++) {
                    //读取PageCount_recorded，current_block，current_page，current_index
                    accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                    accessRequest.accessCount = 8;
                    ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                          buf, 8, &bytesAccessed, 0);
                    if (*(PULONG)buf == 0x02010152) {
                        checkPageCountReadyCounter = MAX_CHECK_COUNT;//关闭循环

                        UCHAR pageCountRecorded = buf[4];
                        UCHAR currentBlock = buf[5];
                        UCHAR currentPage = buf[6];
                        UCHAR currentIndex = buf[7];

                        //通知EC读取PageCount完成
                        *(PULONG)buf = 0x03010152;
                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                        accessRequest.accessCount = 4;
                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                              buf + 4, 4, &bytesAccessed, 0);
                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                        accessRequest.accessCount = 4;
                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                              buf, 4, &bytesAccessed, 0);

                        //如果PageCount_recorded >=4，需要读取eFlash log。(旧的逻辑，这里修改了)
                        if (   (pageCountRecorded < 4 && !isPassive) || (isPassive&& pageCountRecorded < 1) || (pageCountRecorded > 16)) { //checked
                            status = EC_NEED_NOT_READ;
                        } else {
                            for (int checkECIdleCounter_1 = 0; checkECIdleCounter_1 < MAX_CHECK_COUNT; checkECIdleCounter_1++) {
                                if (IsECIdle(hDev)) {
                                    checkECIdleCounter_1 = MAX_CHECK_COUNT;//关闭循环
                                    for (int pageIndex = 1, pageRetryed = FALSE; pageIndex <= pageCountRecorded; ) {
                                        //此命令为通知EC将读取特定[Page]( 1到 PageCount_recorded)的256 bytes eFlash log数据。
                                        buf[0] = 0x52;
                                        buf[1] = 0x02;
                                        buf[2] = 0x02;
                                        buf[3] = 0x01;
                                        buf[4] = pageIndex;
                                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                                        accessRequest.accessCount = 4;
                                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                              buf + 4, 4, &bytesAccessed, 0);
                                        accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                                        accessRequest.accessCount = 4;
                                        ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                              buf, 4, &bytesAccessed, 0);

                                        //读一个page
                                        for (int checkLogReadyCounter = 0; checkLogReadyCounter < MAX_CHECK_COUNT; checkLogReadyCounter++) {
                                            if (IsLogPageReady(pageIndex, hDev)) {
                                                checkLogReadyCounter = MAX_CHECK_COUNT;
                                                //此时APP可以去Memory的0xFE0B0E00位置读走eFlash log数据
                                                accessRequest.physicalAddress.QuadPart = 0xFE0B0E00;
                                                accessRequest.accessCount = 256;
                                                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                                      logBuf, 256, &bytesAccessed, 0);

                                                if (ret) {
                                                    pVecECLog.push_back(std::vector<UCHAR>(logBuf, logBuf + 256));
                                                    /*FILE* fp = fopen("log.bin", "wb");
                                                    fwrite(logBuf, 256, 1, fp);
                                                    fclose(fp);*/
                                                }
                                                break;

                                            } else {
                                                Sleep(SLEEPTIME);
                                                continue;
                                            }
                                        }

                                        if (pVecECLog.size() < pageIndex) { //本次读取失败
                                            if (pageRetryed == FALSE) {
                                                pageRetryed = TRUE;
                                                continue;//给一次重新读取的机会
                                            } else {
                                                status = EC_FAILED;
                                                FreeECLog(pVecECLog);
                                                break;//关闭循环
                                            }

                                        } else {
                                            //此命令为通知EC，APP读取特定[Page]的256 bytes eFlash log数据完成
                                            buf[0] = 0x52;
                                            buf[1] = 0x02;
                                            buf[2] = 0x02;
                                            buf[3] = 0x03;
                                            buf[4] = pageIndex;
                                            accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                                            accessRequest.accessCount = 4;
                                            ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                                  buf + 4, 4, &bytesAccessed, 0);
                                            accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                                            accessRequest.accessCount = 4;
                                            ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                                  buf, 4, &bytesAccessed, 0);
                                            Sleep(SLEEPTIME);
                                            if (pageIndex == pageCountRecorded) {
                                                Sleep(SLEEPTIME);
                                                //此命令为通知EC，APP读取所有的eFlash log数据完成。
                                                buf[0] = 0x52;
                                                buf[1] = 0x0f;
                                                buf[2] = 0x0f;
                                                buf[3] = 0x01;
                                                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8 + 4;
                                                accessRequest.accessCount = 4;
                                                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                                      buf + 4, 4, &bytesAccessed, 0);
                                                accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
                                                accessRequest.accessCount = 4;
                                                ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_WRITE, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
                                                                      buf, 4, &bytesAccessed, 0);
                                                for (int checkECIdleCounter_3 = 0; checkECIdleCounter_3 < MAX_CHECK_COUNT; checkECIdleCounter_3++) {
                                                    if (IsECIdle(hDev)) {
                                                        status = EC_SUCCESS;
                                                        break;
                                                    } else {
                                                        Sleep(SLEEPTIME);
                                                        continue;
                                                    }
                                                }


                                                break;
                                            } else {
                                                int checkECIdleCounter_2 = 0;
                                                for (; checkECIdleCounter_2 < MAX_CHECK_COUNT; checkECIdleCounter_2++) {
                                                    if (IsECIdle(hDev)) {
                                                        break;
                                                    } else {
                                                        Sleep(SLEEPTIME);
                                                        continue;
                                                    }
                                                }
                                                if (checkECIdleCounter_2 == MAX_CHECK_COUNT) {
                                                    break;//EC 出错，直接退出
                                                }
                                            }

                                            pageIndex++;
                                            pageRetryed = FALSE;
                                        }

                                    }

                                } else { //checked
                                    Sleep(SLEEPTIME);
                                    continue;
                                }
                            }
                        }



                    } else { //checked
                        if (IsErrorOccur(hDev)) {
                            ErrorAck(hDev);
                            break;
                        }
                        Sleep(SLEEPTIME);
                        continue;
                    }
                }
            } else { //checked
                Sleep(SLEEPTIME);
                continue;
            }

        }
        CloseHandle(hDev);
    } else {
        status = EC_CREATE_FILE_FAILED;
    }
    return status;
}


void ThreadFuncEventMonitor(std::function<void()> callBack) {
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
               NULL,
               -1,                          // COM negotiates service
               NULL,                        // Authentication services
               NULL,                        // Reserved
               RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
               RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
               NULL,                        // Authentication info
               EOAC_NONE,                   // Additional capabilities
               NULL                         // Reserved
           );


    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return;                      // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
               CLSID_WbemLocator,
               0,
               CLSCTX_INPROC_SERVER,
               IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return;                 // Program has failed.
    }

    // Step 4: ---------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Connect to the local root\cimv2 namespace
    // and obtain pointer pSvc to make IWbemServices calls.
    hres = pLoc->ConnectServer(
               _bstr_t(L"root\\wmi"),
               NULL,
               NULL,
               0,
               NULL,
               0,
               0,
               &pSvc
           );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return;                // Program has failed.
    }

    std::cout << "Connected to root\\wmi WMI namespace" << std::endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
               pSvc,                        // Indicates the proxy to set
               RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
               RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
               NULL,                        // Server principal name
               RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
               RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
               NULL,                        // client identity
               EOAC_NONE                    // proxy capabilities
           );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;               // Program has failed.
    }

    // Step 6: -------------------------------------------------
    // Receive event notifications -----------------------------

    // Use an unsecured apartment for security
    IUnsecuredApartment* pUnsecApp = NULL;

    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
                            CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
                            (void**)&pUnsecApp);

    EventSink* pSink = new EventSink(callBack);
    pSink->AddRef();

    IUnknown* pStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

    IWbemObjectSink* pStubSink = NULL;
    pStubUnk->QueryInterface(IID_IWbemObjectSink,
                             (void**)&pStubSink);


    int thid = GetCurrentThreadId();

    // The ExecNotificationQueryAsync method will call
    // The EventQuery::Indicate method when an event occurs
    hres = pSvc->ExecNotificationQueryAsync(
               _bstr_t("WQL"),
               _bstr_t("SELECT * "
                       "FROM HID_EVENT20"),
               WBEM_FLAG_SEND_STATUS,
               NULL,
               pStubSink);
    //with 32 bytes of data

    /*WBEM_E_NOT_EVENT_CLASS
    2147749977 （0x80041059）*/

    /*WBEM_E_UNPARSABLE_QUERY
    2147749976 （0x80041058）*/

    // Check for errors.
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        pUnsecApp->Release();
        pStubUnk->Release();
        pSink->Release();
        pStubSink->Release();
        CoUninitialize();
        return;
    }

    // Wait for the event
    HANDLE hEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, L"ECLogEvent");
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);


    hres = pSvc->CancelAsyncCall(pStubSink);

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pUnsecApp->Release();
    pStubUnk->Release();
    pSink->Release();
    pStubSink->Release();
    CoUninitialize();

    return;   // Program successfully completed.

}

//set callBack to NULL,means close eclog ready check thread,if you want to save some resources,just do it
bool RegisterECLogReadyNotifier(std::function<void()> callBack) {
    static std::thread* pThread = NULL;
    static HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, L"ECLogEvent"); //manual non-signaled

    if (pThread != NULL) {
        SetEvent(hEvent);
        pThread->join();
        delete pThread;
        pThread = 0;
    }
    if (callBack == NULL) {
        return true;
    }
    ResetEvent(hEvent);//non-signaled
    pThread = new std::thread{ ThreadFuncEventMonitor, callBack };

    return pThread->joinable();
}