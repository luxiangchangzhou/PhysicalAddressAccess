
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include "../PhysicalAddressAccess/PhysicalAddressAccess.h"


#define MAX_CHECK_COUNT 3


enum EC_LOG_STATE
{
	EC_SUCCESS,
	EC_FAILED,
	EC_NEED_NOT_READ,
	EC_CREATE_FILE_FAILED
};

void FreeECLog(std::vector<UCHAR*>* pVecECLog);


bool IsECIdle(HANDLE hDev)
{
	unsigned char buf[8] = { 0x0,0x0,0x0,0x0,0xff,0xff,0xff,0xff };
	DWORD bytesAccessed = 0;
	PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
	accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
	accessRequest.accessCount = 8;
	//判断EC IDLE
	BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
		buf, 8, &bytesAccessed, 0);
	if (ret)
	{
		//EC IDLE
		if (*(PULONG)buf == 0x0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

}

bool IsErrorOccur(HANDLE hDev)
{
	unsigned char buf[8] = { 0 };
	DWORD bytesAccessed = 0;
	PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
	accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
	accessRequest.accessCount = 8;
	//判断EC IDLE
	BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
		buf, 8, &bytesAccessed, 0);
	if (ret)
	{
		//EC IDLE
		if (*(PULONG)buf == 0x011f1f52)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

}

bool ErrorAck(HANDLE hDev)
{
	unsigned char buf[8] = { 0x52,0x1f,0x1f,0x02,0xff,0xff,0xff,0xff };
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
	if (ret)
	{
		return true;
	}
	else
	{
		return false;
	}

}


bool SetECIdle(HANDLE hDev)
{
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
	if (ret)
	{
		return true;
	}
	else
	{
		return false;
	}

}


bool IsLogPageReady(UCHAR page, HANDLE hDev)
{
	unsigned char buf[8] = { 0,0,0,0,0,0xff,0xff,0xff };
	DWORD bytesAccessed = 0;
	PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
	accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
	accessRequest.accessCount = 8;

	BOOL ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
		buf, 8, &bytesAccessed, 0);
	if (ret)
	{
		//log ready
		if (*(PULONG)buf == 0x02020252 && buf[4] == page)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

}

bool ECInformedShouldRead()
{
}

EC_LOG_STATE GetECLog(std::vector<UCHAR*>* pVecECLog)
{
	//一次最多读4k
	HANDLE hDev = CreateFile(L"\\\\.\\PhysicalAddressAccess", GENERIC_ALL, 0, NULL, OPEN_EXISTING, 0, NULL);
	EC_LOG_STATE status = EC_FAILED;
	unsigned char logBuf[256] = { 0 };
	unsigned char buf[8] = { 0x0,0x0,0x0,0x0,0xff,0xff,0xff,0xff };
	DWORD bytesAccessed = 0;
	PHYSICAL_ADDRESS_ACCESS_REQUEST accessRequest;
	bool ret;

	if (hDev != INVALID_HANDLE_VALUE)
	{
		printf("CreateFile OK\n");

		SetECIdle(hDev);

		for (int checkECIdleCounter = 0; checkECIdleCounter < MAX_CHECK_COUNT; checkECIdleCounter++)
		{
			if (IsECIdle(hDev))
			{
				checkECIdleCounter = MAX_CHECK_COUNT;//关闭循环
				printf("EC 处于idle状态\n");
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


				for (int checkPageCountReadyCounter = 0; checkPageCountReadyCounter < MAX_CHECK_COUNT; checkPageCountReadyCounter++)
				{
					//读取PageCount_recorded，current_block，current_page，current_index
					accessRequest.physicalAddress.QuadPart = 0xFE0B0AB8;
					accessRequest.accessCount = 8;
					ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
						buf, 8, &bytesAccessed, 0);
					if (*(PULONG)buf == 0x02010152)
					{
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

						//如果PageCount_recorded >=4，需要读取eFlash log。
						if (pageCountRecorded < 4)//checked
						{
							printf("pageCountRecorded = %d < 4  need not to read \n", pageCountRecorded);
							status = EC_NEED_NOT_READ;
						}
						else
						{
							for (int checkECIdleCounter_1 = 0; checkECIdleCounter_1 < MAX_CHECK_COUNT; checkECIdleCounter_1++)
							{
								if (IsECIdle(hDev))
								{
									checkECIdleCounter_1 = MAX_CHECK_COUNT;//关闭循环
									for (int pageIndex = 1, pageRetryed = FALSE; pageIndex <= pageCountRecorded; )
									{
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


										for (int checkLogReadyCounter = 0; checkLogReadyCounter < MAX_CHECK_COUNT; checkLogReadyCounter++)
										{
											if (IsLogPageReady(pageIndex, hDev))
											{
												checkLogReadyCounter = MAX_CHECK_COUNT;
												//此时APP可以去Memory的0xFE0B0E00位置读走eFlash log数据
												accessRequest.physicalAddress.QuadPart = 0xFE0B0E00;
												accessRequest.accessCount = 256;
												ret = DeviceIoControl(hDev, IOCTL_PHYSICALADDRESS_READ, &accessRequest, sizeof(PHYSICAL_ADDRESS_ACCESS_REQUEST),
													logBuf, 256, &bytesAccessed, 0);

												if (ret)
												{
													PUCHAR p = new UCHAR[256];
													pVecECLog->push_back(p);
													memcpy(p, logBuf, 256);

													FILE* fp = fopen("log.bin", "wb");
													fwrite(logBuf, 256, 1, fp);
													fclose(fp);
												}
												break;

											}
											else
											{
												Sleep(1000);
												continue;
											}
										}

										if (pVecECLog->size() < pageIndex)//有部分读取失败
										{
											if (pageRetryed == FALSE)
											{
												pageRetryed = TRUE;
												continue;//给一次重新读取的机会
											}
											else
											{
												status = EC_FAILED;
												FreeECLog(pVecECLog);
												break;//关闭循环
											}



										}
										else
										{
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


											if (pageIndex == pageCountRecorded)
											{
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



												for (int checkECIdleCounter_3 = 0; checkECIdleCounter_3 < MAX_CHECK_COUNT; checkECIdleCounter_3++)
												{
													if (IsECIdle(hDev))
													{
														status = EC_SUCCESS;
														printf("ec log read over success!!!!!\n");
														break;
													}
													else
													{
														printf("EC is not idle just may continue retry,checkECIdleCounter_3 = %d\n", checkECIdleCounter_3);
														Sleep(1000);
														continue;
													}
												}


												break;
											}
											else
											{
												int checkECIdleCounter_2 = 0;
												for (; checkECIdleCounter_2 < MAX_CHECK_COUNT; checkECIdleCounter_2++)
												{
													if (IsECIdle(hDev))
													{
														break;
													}
													else
													{
														printf("EC is not idle just may continue retry,checkECIdleCounter_2 = %d\n", checkECIdleCounter_2);
														Sleep(1000);
														continue;
													}
												}
												if (checkECIdleCounter_2 == MAX_CHECK_COUNT)
												{
													break;//EC 出错，直接退出
												}
											}

											pageIndex++;
											pageRetryed = FALSE;
										}

									}

								}
								else//checked
								{
									printf("EC is not idle just may continue retry,checkECIdleCounter_1 = %d\n", checkECIdleCounter_1);
									Sleep(1000);
									continue;
								}
							}
						}
					}
					else //checked
					{
						if (IsErrorOccur(hDev))
						{
							printf("Read EC PageCount ErrorOccur\n");
							ErrorAck(hDev);
							break;
						}

						printf("EC PageCount is not ready may continue retry,checkPageCountReadyCounter = %d\n", checkPageCountReadyCounter);
						Sleep(1000);
						continue;
					}
				}
			}
			else //checked
			{
				printf("EC is not idle just may continue retry,checkECIdleCounter = %d\n", checkECIdleCounter);
				Sleep(1000);
				continue;
			}

		}

		CloseHandle(hDev);
	}
	else
	{
		printf("CreateFile failed Error = %d", GetLastError());
		status = EC_CREATE_FILE_FAILED;
	}


	return status;
}

void FreeECLog(std::vector<UCHAR*>* pVecECLog)
{
}




int main()
{

	//std::vector<UCHAR*> vecECLog;

	//EC_LOG_STATE logState = GetECLog(&vecECLog);
	//if (logState == EC_SUCCESS)
	//{

	//	//need to free
	//}
	//else
	//{
	//	//no need to free
	//}











}
