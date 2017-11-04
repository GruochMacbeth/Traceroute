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
unsigned long ipaddr = INADDR_NONE;

/*************************************************
Function:       printEchoData
Description:    打印tracert信息
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply 为得到的相应信息的存储结构
Output:         打印pEchoReply中的相关信息，包括超时等
Return:         false：已经到达目的地址，代表请求结束循环；true：还未到达目的地址，继续循环发送ICMP请求
*************************************************/
int  main(int argc, char **argv) {
	void tracert(const char*);
	char hostname[1024];
	char * destAddr;
	//// 处理main函数的参数，错误则给出提示，正确则存入ipaddr变量
	if (argc != 2) {
		exit(0);
	}
	else
	{
		strcpy(hostname, argv[1]);
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		struct hostent *host = gethostbyname(hostname);
		if (!host) {
			puts("Get IP address error!");
			system("pause");
			exit(0);
		}
		printf("IP addr: %s\n", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));

		destAddr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
		printf("%s\n", destAddr);
		ipaddr = inet_addr(destAddr);
		printf("%d\n", ipaddr);

		if (ipaddr == INADDR_NONE) {
			printf("输入的%s是错误的！\n", argv[0]);
			return 1;
		}
		tracert(destAddr);
	}


	return 0;
}

/*************************************************
Function:       printEchoData
Description:    打印tracert信息
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply 为得到的相应信息的存储结构
Output:         打印pEchoReply中的相关信息，包括超时等
Return:         false：已经到达目的地址，代表请求结束循环；true：还未到达目的地址，继续循环发送ICMP请求
*************************************************/
bool printEchoData(PICMP_ECHO_REPLY pEchoReply[], const char * destAddr)
{
	ReplyAddr.S_un.S_addr = pEchoReply[0]->Address;
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
		if (!strcmp(destAddr, inet_ntoa(ReplyAddr)))
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
		printf("\t%s\tdest:%s\n", inet_ntoa(ReplyAddr),destAddr);

	//还未到达目的地址，继续循环发送ICMP请求
	return result;
}
/*************************************************
Function:       tracert
Description:    tracert服务
Calls:          IcmpCreateFile, Sleep, IcmpSendEcho, printf, printEchoData
*************************************************/
void tracert(const char * destAddr)
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

			dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
				&IOI, ReplyBuffer, ReplySize, 1000);
			pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
			//解析返回的ICMP报文，并打印相关信息
			pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
		}
		//print
		if (!printEchoData(pEchoReply, destAddr))
			break;
	}
	printf("跟踪完成。\n");
}


