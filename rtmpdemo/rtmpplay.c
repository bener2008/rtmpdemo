#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "stdint.h"
#ifndef WIN32
#include <unistd.h>
#endif

#ifndef RTMPDUMP_VERSION
#define RTMPDUMP_VERSION "v2.4"
#endif 

#define USE_MYFLV 1

#include "librtmp/rtmp.h"
#include "myflv.h"

#if 0
int main(){
	const char* rtmpurl="rtmp://192.168.1.100:1935/live/test";//连接的URL
	const char* flvfilename="test2.flv";//保存的flv文件
	RTMP_LogLevel loglevel=RTMP_LOGINFO;//设置RTMP信息等级
	RTMP_LogSetLevel(loglevel);//设置信息等级
//	RTMP_LogSetOutput(FILE*fp);//设置信息输出文件
	RtmpPlayStream(rtmpurl,flvfilename,0);
	return 0;
}
#endif

void RtmpPlayStream(const char*flvfilename,const char*rtmpurl,int bLive){
	RTMP*rtmp=NULL;//rtmp应用指针
	RTMPPacket*packet=NULL;//rtmp包结构
	char url[256]={0};
	int buffsize=1024;
	char*buff=(char*)malloc(buffsize);
	double duration=-1;
	int nRead=0;
	long 	countbuffsize=0;
#if USE_MYFLV
	MyFLV*myflv=MyFlvCreate(flvfilename);
	if(myflv==NULL)
		return ;
#else
	FILE*fp=fopen(flvfilename,"wb");
	if(fp==NULL)
		return ;
#endif
	rtmp=RTMP_Alloc();//申请rtmp空间
	RTMP_Init(rtmp);//初始化rtmp设置
	rtmp->Link.timeout=25;//超时设置
	//由于crtmpserver是每个一段时间(默认8s)发送数据包,需大于发送间隔才行
	strcpy(url,rtmpurl);
	RTMP_SetupURL(rtmp,url);
	if (bLive){
		//设置直播标志
		rtmp->Link.lFlags|=RTMP_LF_LIVE;
	}
	RTMP_SetBufferMS(rtmp,100*1000);//10s
	if(!RTMP_Connect(rtmp,NULL)){
		printf("Connect Server Err\n");
		goto end;
	}
	if(!RTMP_ConnectStream(rtmp,0)){
		printf("Connect stream Err\n");
		goto end;
	}
#if USE_MYFLV
	packet=(RTMPPacket*)malloc(sizeof(RTMPPacket));//创建包
	memset(packet,0,sizeof(RTMPPacket));	
	RTMPPacket_Reset(packet);//重置packet状态
	while (RTMP_GetNextMediaPacket(rtmp,packet)){
		if(packet->m_packetType==0x09&&packet->m_body[0]==0x17){
			printf("TimeStamp:%u\n",packet->m_nTimeStamp);
		}	
		if(packet->m_packetType==0x09||packet->m_packetType==0x08){
			MyFrame myframe={0};
			myframe.type=packet->m_packetType;
			myframe.datalength=packet->m_nBodySize;
			myframe.timestamp=packet->m_nTimeStamp;
			MyFlvWriteFrame(myflv,myframe,packet->m_body,packet->m_nBodySize);

		}
		RTMPPacket_Free(packet);
		RTMPPacket_Reset(packet);//重置packet状态
	}
#else
	//它直接输出的就是FLV文件,包括FLV头,可对流按照flv格式解析就可提前音频,视频数据
	while(nRead=RTMP_Read(rtmp,buff,buffsize)){
		fwrite(buff,1,nRead,fp);
		countbuffsize+=nRead;
		if(buff[0]==0x09&&buff[11]==0x17){
			printf("DownLand...:%0.2fkB\n",countbuffsize*1.0/1024);
		}
	}
#endif
end:
#if USE_MYFLV
	myflv=MyFlvClose(myflv);
#else
	if(fp){
		fclose(fp);
		fp=NULL;
	}
#endif
	if(buff){
		free(buff);
		buff=NULL;
	}
	if(rtmp!=NULL){
		RTMP_Close(rtmp);//断开连接
		RTMP_Free(rtmp);//释放内存
		rtmp=NULL;
	}
}

