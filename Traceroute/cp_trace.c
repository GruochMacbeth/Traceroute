#include "stdafx.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <string.h>

//引入iphlpapi.lib与ws2_32.lib库
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")


//ip数据包选项，其中第一项为ttl
IP_OPTION_INFORMATION IOI = { 0, 0, 0, 0, NULL };
//接收返回的地址
struct in_addr ReplyAddr;
//默认baidu.com的ip
const char* destAddr = "119.75.217.56";
unsigned long ipaddr = INADDR_NONE;

/*************************************************
Function:       printEchoData
Description:    打印tracert信息
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply 为得到的相应信息的存储结构
Output:         打印pEchoReply中的相关信息，包括超时等
Return:         false：已经到达目的地址，代表请求结束循环；true：还未到达目的地址，继续循环发送ICMP请求
*************************************************/
bool printEchoData(PICMP_ECHO_REPLY pEchoReply[])
{
	ReplyAddr.S_un.S_addr = pEchoReply[0]->Address;
	char *addrc = inet_ntoa(ReplyAddr);
	bool result = true;

	printf("%d\t", IOI.Ttl);
	for (int i = 0; i < 3; i++)
	{
		if (pEchoReply[i]->Status == IP_TTL_EXPIRED_TRANSIT || pEchoReply[i]->Status == IP_TTL_EXPIRED_REASSEM)
		{
			if (pEchoReply[i]->RoundTripTime == 0)
			{
				printf("<1ms\t");
			}
			else
			{
				printf("%ldms\t", pEchoReply[i]->RoundTripTime);
			}
		}
		//超时，用*表示 错误代码是11010：IP_REQ_TIMED_OUT   (11010)   The request timed out
		else if (pEchoReply[i]->Status == IP_REQ_TIMED_OUT)
		{
			printf("*\t");
		}

		//到达目的地址
		if (!strcmp(inet_ntoa(ReplyAddr), destAddr))
		{
			if (pEchoReply[i]->RoundTripTime == 0)
			{
				printf("<1ms\t");
			}
			else
			{
				printf("%ldms\t", pEchoReply[i]->RoundTripTime);
			}

			//已经到达目的地址，false代表请求结束循环
			result = false;
		}
	}
	if (pEchoReply[0]->Status == IP_REQ_TIMED_OUT)
		printf("\t*\n");
	else
		printf("\t%s\n", inet_ntoa(ReplyAddr));

	//还未到达目的地址，继续循环发送ICMP请求
	return result;
}
/*************************************************
Function:       tracert
Description:    tracert服务
Calls:          IcmpCreateFile, Sleep, IcmpSendEcho, printf, printEchoData
*************************************************/
void tracert()
{
	HANDLE hIcmpFile;
	DWORD dwRetVal = 0;
	char SendData[32] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;
	u_char maxTTL = 30;

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		printf("\t打开句柄失败！\n");
		printf("IcmpCreatefile返回的错误码： %ld\n", GetLastError());
		return;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL) {
		printf("\t分配内存失败！\n");
		return;
	}



	printf("最多通过%d个跃点跟踪\n到%s的路由：\n", maxTTL, destAddr);

	PICMP_ECHO_REPLY pEchoReply[3];
	bool over = false;
	while (1)
	{
		if (over)
		{
			break;
		}
		IOI.Ttl++;
		//每个目标发送三次报文，可以减少超时的情况
		for (int i = 0; i < 3; i++)
		{
			if (IOI.Ttl == maxTTL + 1)
			{
				printf("达到最大TTL，但没有到达目的主机！\n");
				over = true;
			}
			//Sleep(2000);
			//发送ICMP报文
			/*if (i)
			{
			Sleep(2000);
			}*/
			dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
				&IOI, ReplyBuffer, ReplySize, 1000);
			pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
			//解析返回的ICMP报文，并打印相关信息
			if (dwRetVal == 0) {
				pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
			}

			else {
				pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
				//printf("%d\t*\t*\n", IOI.Ttl);
			}
		}
		//print
		if (!printEchoData(pEchoReply))
			break;
	}
	printf("跟踪完成。\n");
}


/*************************************************
Function:       printEchoData
Description:    打印tracert信息
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply 为得到的相应信息的存储结构
Output:         打印pEchoReply中的相关信息，包括超时等
Return:         false：已经到达目的地址，代表请求结束循环；true：还未到达目的地址，继续循环发送ICMP请求
*************************************************/
int __cdecl main(int argc, char **argv) {
	ipaddr = inet_addr(destAddr);//默认baidu.com的ip

								 //// 处理main函数的参数，错误则给出提示，正确则存入ipaddr变量
	if (argc != 2) {
		printf("未输入IP地址，将使用程序%s的\n默认tracertIP地址： %s\n", argv[0], destAddr);
	}
	else
	{
		destAddr = argv[1];
		ipaddr = inet_addr(argv[1]);
		if (ipaddr == INADDR_NONE) {
			printf("您输入到程序%s的IP地址是错误的！\n", argv[0]);
			return 1;
		}
	}

	tracert();
	system("pause");
	return 0;
}