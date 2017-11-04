#include "stdafx.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <string.h>

//����iphlpapi.lib��ws2_32.lib��
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")


//ip���ݰ�ѡ����е�һ��Ϊttl
IP_OPTION_INFORMATION IOI = { 0, 0, 0, 0, NULL };
//���շ��صĵ�ַ

struct in_addr ReplyAddr;
unsigned long ipaddr = INADDR_NONE;

/*************************************************
Function:       printEchoData
Description:    ��ӡtracert��Ϣ
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply Ϊ�õ�����Ӧ��Ϣ�Ĵ洢�ṹ
Output:         ��ӡpEchoReply�е������Ϣ��������ʱ��
Return:         false���Ѿ�����Ŀ�ĵ�ַ�������������ѭ����true����δ����Ŀ�ĵ�ַ������ѭ������ICMP����
*************************************************/
int  main(int argc, char **argv) {
	void tracert(const char*);
	char * hostname;
	
	//// ����main�����Ĳ����������������ʾ����ȷ�����ipaddr����
	if (argc != 2) {
		exit(0);
	}
	else
	{
		hostname = (char *)malloc(1024 * sizeof(char));
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

		const char * destAddr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
		printf("%s\n", destAddr);
		ipaddr = inet_addr(destAddr);
		printf("%d\n", ipaddr);

		if (ipaddr == INADDR_NONE) {
			printf("�����%s�Ǵ���ģ�\n", argv[0]);
			return 1;
		}
		tracert(destAddr);
	}


	return 0;
}

/*************************************************
Function:       printEchoData
Description:    ��ӡtracert��Ϣ
Calls:          inet_ntoa, strcmp, printf
Input:          PICMP_ECHO_REPLY pEchoReply Ϊ�õ�����Ӧ��Ϣ�Ĵ洢�ṹ
Output:         ��ӡpEchoReply�е������Ϣ��������ʱ��
Return:         false���Ѿ�����Ŀ�ĵ�ַ�������������ѭ����true����δ����Ŀ�ĵ�ַ������ѭ������ICMP����
*************************************************/
bool printEchoData(PICMP_ECHO_REPLY pEchoReply[], char * DestAddr)
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
		//��ʱ����*��ʾ ���������11010��IP_REQ_TIMED_OUT   (11010)   The request timed out
		else if (pEchoReply[i]->Status == IP_REQ_TIMED_OUT)
		{
			printf("*\t");
		}

		//����Ŀ�ĵ�ַ
		if (!strcmp(DestAddr, inet_ntoa(ReplyAddr)))
		{
			if (pEchoReply[i]->RoundTripTime == 0)
			{
				printf("<1ms\t");
			}
			else
			{
				printf("%ldms\t", pEchoReply[i]->RoundTripTime);
			}

			//�Ѿ�����Ŀ�ĵ�ַ��false�����������ѭ��
			result = false;
		}
	}
	if (pEchoReply[0]->Status == IP_REQ_TIMED_OUT)
		printf("\t*\n");
	else
		printf("\t%s\tdest:%s\n", inet_ntoa(ReplyAddr),DestAddr);

	//��δ����Ŀ�ĵ�ַ������ѭ������ICMP����
	return result;
}
/*************************************************
Function:       tracert
Description:    tracert����
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
		printf("\t�򿪾��ʧ�ܣ�\n");
		printf("IcmpCreatefile���صĴ����룺 %ld\n", GetLastError());
		return;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL) {
		printf("\t�����ڴ�ʧ�ܣ�\n");
		return;
	}

	printf("���ͨ��%d��Ծ�����\n��%s��·�ɣ�\n", maxTTL, destAddr);

	PICMP_ECHO_REPLY pEchoReply[3];
	bool over = false;
	char * DestAddr = (char *)malloc(1024 * sizeof(char));
	strcpy(DestAddr, destAddr);
	while (1)
	{
		if (over)
		{
			break;
		}
		IOI.Ttl++;
		//ÿ��Ŀ�귢�����α��ģ����Լ��ٳ�ʱ�����
		for (int i = 0; i < 3; i++)
		{
			if (IOI.Ttl == maxTTL + 1)
			{
				printf("�ﵽ���TTL����û�е���Ŀ��������\n");
				over = true;
			}

			dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
				&IOI, ReplyBuffer, ReplySize, 1000);
			pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
			//�������ص�ICMP���ģ�����ӡ�����Ϣ
			pEchoReply[i] = (PICMP_ECHO_REPLY)ReplyBuffer;
		}
		//print
		if (!printEchoData(pEchoReply,DestAddr))
			break;
	}
	printf("������ɡ�\n");
}


