

#include <vector>
#include <iostream>
#include "LX_Timer.h"
#include"../EcIoSdk/eclog.h"

void lxCallBack() {


    int count = GetECLogPageCount();
    std::cout << "lxCallBack called   page count = "<<count << std::endl;
}

int main() {
    //RegisterECLogReadyNotifier(lxCallBack);
    //RegisterECLogReadyNotifier(lxCallBack);
    //RegisterECLogReadyNotifier(0);
    //RegisterECLogReadyNotifier(lxCallBack);


    //std::cin.get();

    //return 0;

    //失败5次就结束
    for (int failcount = 0, i  = 0; ; i++) {
        LX_Timer timer;
        std::vector<std::vector<UCHAR>> vecECLog;

        timer.start_ms(0);
        EC_LOG_STATE logState = GetECLog(vecECLog,false);
        if (logState == EC_SUCCESS) {


            //FILE* fp = fopen("eclog.bin", "wb");
            //fwrite(vecECLog[0].data(), 256, 1, fp);
            //fclose(fp);

            std::cout << "read success count : " << i << "  ms : " << timer.get_ms() << std::endl;
            std::cout << "logstate = " << logState << std::endl;
            FreeECLog(vecECLog);
            //need to free
        } else {
            failcount++;
            std::cout << "read failed count : " << i << "   ms : " << timer.get_ms() << std::endl;
            std::cout << "logstate = " << logState << std::endl;
            //no need to free
        }
        Sleep(0);
    }

    std::cin.get();

}
