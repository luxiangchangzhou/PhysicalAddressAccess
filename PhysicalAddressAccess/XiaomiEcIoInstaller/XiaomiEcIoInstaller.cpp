
#include <Windows.h>
#include <iostream>
#include <ShlObj.h>


#define countof(array) (sizeof(array) / sizeof(array[0]))

#define ECIO_DRIVER_FULL_PATH                                                 \
  L"%SystemRoot%\\system32\\drivers\\XiaomiEcIo.sys"

#define ECIO_DRIVER_SERVICE L"XiaomiEcIo"

#define ECIO_SERVICE_START 1
#define ECIO_SERVICE_STOP 2
#define ECIO_SERVICE_DELETE 3


#define GetOption(argc, argv, index)                                           \
  (((argc) > (index) && wcslen((argv)[(index)]) == 2 &&                        \
    (argv)[(index)][0] == L'/')                                                \
       ? towlower((argv)[(index)][1])                                          \
       : L'\0')


BOOL EcIOServiceControl(LPCWSTR ServiceName, ULONG Type) {
    SC_HANDLE controlHandle;
    SC_HANDLE serviceHandle;
    SERVICE_STATUS ss;
    BOOL result = TRUE;

    controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (controlHandle == NULL) {
        wprintf(L"EcIOServiceControl: Failed to open Service Control "
            L"Manager. error = %d\n",
            GetLastError());
        return FALSE;
    }

    serviceHandle =
        OpenService(controlHandle, ServiceName,
            SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

    if (serviceHandle == NULL) {
        wprintf(
            L"EcIOServiceControl: Failed to open Service (%s). error = %d\n",
            ServiceName, GetLastError());
        CloseServiceHandle(controlHandle);
        return FALSE;
    }

    if (QueryServiceStatus(serviceHandle, &ss) != 0) {
        if (Type == ECIO_SERVICE_DELETE) {
            if (DeleteService(serviceHandle)) {
                wprintf(L"EcIOServiceControl: Service (%s) deleted\n",
                    ServiceName);
                result = TRUE;
            }
            else {
                wprintf(
                    L"EcIOServiceControl: Failed to delete service (%s). error = %d\n",
                    ServiceName, GetLastError());
                result = FALSE;
            }
        }
        else if (ss.dwCurrentState == SERVICE_STOPPED &&
            Type == ECIO_SERVICE_START) {
            if (StartService(serviceHandle, 0, NULL)) {
                wprintf(L"EcIOServiceControl: Service (%s) started\n",
                    ServiceName);
                result = TRUE;
            }
            else {
                wprintf(
                    L"EcIOServiceControl: Failed to start service (%s). error = %d\n",
                    ServiceName, GetLastError());
                result = FALSE;
            }
        }
        else if (ss.dwCurrentState == SERVICE_RUNNING &&
            Type == ECIO_SERVICE_STOP) {
            if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &ss)) {
                wprintf(L"EcIOServiceControl: Service (%s) stopped\n",
                    ServiceName);
                result = TRUE;
            }
            else {
                wprintf(
                    L"EcIOServiceControl: Failed to stop service (%s). error = %d\n",
                    ServiceName, GetLastError());
                result = FALSE;
            }
        }
    }
    else {
        wprintf(
            L"EcIOServiceControl: QueryServiceStatus Failed (%s). error = %d\n",
            ServiceName, GetLastError());
        result = FALSE;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(controlHandle);

    Sleep(100);
    return result;
}

BOOL EcIOServiceInstall(LPCWSTR ServiceName, DWORD ServiceType, LPCWSTR ServiceFullPath)
{
    SC_HANDLE controlHandle;
    SC_HANDLE serviceHandle;

    controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (controlHandle == NULL) {
        wprintf(L"EcIOServiceInstall: Failed to open Service Control "
            L"Manager. error = %d\n",
            GetLastError());
        return FALSE;
    }

    serviceHandle =
        CreateService(controlHandle, ServiceName, ServiceName, 0, ServiceType,
            SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, ServiceFullPath,
            NULL, NULL, NULL, NULL, NULL);

    if (serviceHandle == NULL) {
        BOOL error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            wprintf(
                L"EcIOServiceInstall: Service (%s) is already installed\n",
                ServiceName);
        }
        else {
            wprintf(
                L"EcIOServiceInstall: Failed to install service (%s). error = %d\n",
                ServiceName, error);
        }
        CloseServiceHandle(controlHandle);
        return FALSE;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(controlHandle);

    wprintf(L"EcIOServiceInstall: Service (%s) installed\n", ServiceName);

    if (!EcIOServiceControl(ServiceName, ECIO_SERVICE_START)) {
        wprintf(L"EcIOServiceInstall: Service (%s) start failed\n",
            ServiceName);
        return FALSE;
    }
    wprintf(L"EcIOServiceInstall: Service (%s) started\n", ServiceName);

    return TRUE;
}


int InstallDriver(LPCWSTR driverFullPath) {
    fprintf(stdout, "Installing driver...\n");
    if (GetFileAttributes(driverFullPath) == INVALID_FILE_ATTRIBUTES) {
        fwprintf(stderr, L"Error the file '%ls' does not exist.\n", driverFullPath);
        return EXIT_FAILURE;
    }

    if (!EcIOServiceInstall(ECIO_DRIVER_SERVICE, SERVICE_KERNEL_DRIVER,
        ECIO_DRIVER_FULL_PATH)) {
        fprintf(stderr, "Driver install failed\n");
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Driver installation succeeded!\n");
    return EXIT_SUCCESS;
}

BOOL EcIOServiceExists(LPCWSTR ServiceName) {
    SC_HANDLE controlHandle;
    SC_HANDLE serviceHandle;

    controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (controlHandle == NULL) {
        wprintf(L"EcIOServiceExists: Failed to open Service Control Manager. error "
            L"= %d\n",
            GetLastError());
        return FALSE;
    }

    serviceHandle =
        OpenService(controlHandle, ServiceName,
            SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);

    if (serviceHandle == NULL) {
        wprintf(
            L"EcIOServiceExists: Failed to open Service (%s). error = %d\n",
            ServiceName, GetLastError());
        CloseServiceHandle(controlHandle);
        return FALSE;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(controlHandle);

    return TRUE;
}


BOOL EcIOServiceDelete(LPCWSTR ServiceName) {
    if (!EcIOServiceExists(ServiceName)) {
        return TRUE;
    }
    EcIOServiceControl(ServiceName, ECIO_SERVICE_STOP);
    if (!EcIOServiceControl(ServiceName, ECIO_SERVICE_DELETE)) {
        return FALSE;
    }
    return TRUE;
}

int DeleteEcIOService(LPCWSTR ServiceName) {
    fwprintf(stdout, L"Removing '%ls'...\n", ServiceName);
    if (!EcIOServiceDelete(ServiceName)) {
        fwprintf(stderr, L"Error removing '%ls'\n", ServiceName);
        return EXIT_FAILURE;
    }
    fwprintf(stdout, L"'%ls' removed.\n", ServiceName);
    return EXIT_SUCCESS;
}


int ShowUsage() {
    printf(
        "XiaomiEcIoInstaller /i                : Install driver\n"
        "XiaomiEcIoInstaller /r                : Remove driver\n");
    return EXIT_FAILURE;
}

int DefaultCaseOption() {
    printf("Unknown option - Use /? to show usage\n");
    return EXIT_FAILURE;
}



int __cdecl wmain(int argc, PWCHAR argv[])
{
    BOOL isAdmin;
    WCHAR driverFullPath[MAX_PATH] = { 0 };
    int result = EXIT_SUCCESS;

    isAdmin = IsUserAnAdmin();
    ExpandEnvironmentStringsW(ECIO_DRIVER_FULL_PATH, driverFullPath, MAX_PATH);
    wprintf(L"Driver path: '%ls'\n", driverFullPath);

    WCHAR option = GetOption(argc, argv, 1);
    if (option == L'\0' || option == L'?' || argc < 2) {
        return ShowUsage();
    }


    
    if (argc > 3)
    {
        return ShowUsage();
    }

    WCHAR * driverBinary = argv[2];
    
    WCHAR CurrentDirectory[MAX_PATH] = { 0 };
    GetCurrentDirectory(MAX_PATH, CurrentDirectory);


    if (!isAdmin &&
        (option == L'i' || option == L'r' || option == L'd' || option == L'u')) {
        fprintf(stderr, "Admin rights required to process this operation\n");
        return EXIT_FAILURE;
    }





    switch (option)
    {
        // Admin rights required
    case L'i':
    {
        BOOL copyResult = CopyFile(driverBinary/*L"XiaomiEcIo.sys"*/, driverFullPath, FALSE);//overwrites the existing file

        if (!copyResult)
        {
            wprintf(L"CopyFile XiaomiEcIo.sys failed, maybe you should stop the service\n");
            return EXIT_FAILURE;
        }
        else
        {
            wprintf(L"CopyFile XiaomiEcIo.sys success\n");
        }
        return InstallDriver(driverFullPath);
    }

    case L'r':
    {
        result = DeleteEcIOService(ECIO_DRIVER_SERVICE);

        WCHAR wzTempDirectory[MAX_PATH] = { };
        WCHAR wzTempPath[MAX_PATH] = { };

        // Attempt to delete the file
        if (DeleteFile(driverFullPath)) {
            std::cout << "File deleted successfully." << std::endl;
        }
        else
        {
            // If the file deletion failed, output the error code
            DWORD errorCode = GetLastError();
            std::cout << "Failed to delete file. Error code: " << errorCode << std::endl;

            GetTempPathW(countof(wzTempDirectory), wzTempDirectory);
            GetTempFileNameW(wzTempDirectory, L"DEL", 0, wzTempPath);
            // Try to move the file to the temp directory then schedule for delete,
                                // otherwise just schedule for delete.
            if (::MoveFileExW(driverFullPath, wzTempPath, MOVEFILE_REPLACE_EXISTING))
            {
                ::MoveFileExW(wzTempPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }
            else
            {
                ::MoveFileExW(driverFullPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }


        }


        return result;
    }

    default:
        return DefaultCaseOption();
    }

    return result;
}
