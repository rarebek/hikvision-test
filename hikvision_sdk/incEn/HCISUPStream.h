#ifndef _HC_EHOME_STREAM_H_
#define _HC_EHOME_STREAM_H_

#include "HCISUPPublic.h"

typedef struct tagNET_EHOME_PREVIEW_CB_MSG
{
    BYTE     byDataType;       //Data type:NET_DVR_SYSHEAD(1)- Stream header, NET_DVR_STREAMDATA(2)- Stream data 
    BYTE     byRes1[3];
    void    *pRecvdata;      //Data buffer, for saving stream header or data. 
    DWORD   dwDataLen;      //Data length 
    BYTE     byRes2[128];
}NET_EHOME_PREVIEW_CB_MSG, *LPNET_EHOME_PREVIEW_CB_MSG;

typedef void(CALLBACK *PREVIEW_DATA_CB)(LONG  iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData);

typedef struct tagNET_EHOME_NEWLINK_CB_MSG
{
    BYTE    szDeviceID[MAX_DEVICE_ID_LEN];
    LONG    iSessionID;
    DWORD   dwChannelNo;
    BYTE    byStreamType;
    BYTE    byRes1[2];
    BYTE    byStreamFormat;    
    char    sDeviceSerial[NET_EHOME_SERIAL_LEN];
#if (defined(OS_WINDOWS64) || defined(OS_POSIX64))
    PREVIEW_DATA_CB    fnPreviewDataCB;   
    void               *pUserData;        
#else
    PREVIEW_DATA_CB    fnPreviewDataCB;   
    BYTE               byRes2[4];
    void               *pUserData;         
    BYTE               byRes3[4];
#endif 
    BYTE               byRes[96];
}NET_EHOME_NEWLINK_CB_MSG, *LPNET_EHOME_NEWLINK_CB_MSG;


typedef BOOL (CALLBACK *PREVIEW_NEWLINK_CB)(LONG iLinkHandle,NET_EHOME_NEWLINK_CB_MSG *pNewLinkCBMsg, void *pUserData);

typedef struct tagNET_EHOME_LISTEN_PREVIEW_CFG
{
    NET_EHOME_IPADDRESS struIPAdress; //Local listening information, if the IP address is 0.0.0.0, it is considered as the local address. 
    PREVIEW_NEWLINK_CB    fnNewLinkCB; //The callback function of requiring live view 
    void*               pUser;        // User data 
    BYTE                byLinkMode;   //The listening connection mode, 0- TCP,1- UDP 2-HRUDP
    BYTE                byLinkEncrypt;  // Whether link encryption is enabled, TCP is transmitted through TLS, UDP (including NPQ) uses dtls transmission, 0-not enabled, 1-enabled
    BYTE                byRes[126];
}NET_EHOME_LISTEN_PREVIEW_CFG, *LPNET_EHOME_LISTEN_PREVIEW_CFG;


typedef struct tagNET_EHOME_PREVIEW_DATA_CB_PARAM
{
    PREVIEW_DATA_CB    fnPreviewDataCB;    //Live view data callback function 
    void       *pUserData;         //User data 
    BYTE       byStreamFormat; //0- PS
    BYTE       byRes[127];
}NET_EHOME_PREVIEW_DATA_CB_PARAM, *LPNET_EHOME_PREVIEW_DATA_CB_PARAM;


typedef enum tagNET_EHOME_ESTREAM_INIT_CFG_TYPE
{
    NET_EHOME_ESTREAM_INIT_CFG_LIBEAY_PATH = 0,
    NET_EHOME_ESTREAM_INIT_CFG_SSLEAY_PATH = 1,
    NET_EHOME_ESTREAM_INIT_CFG_USERCERTIFICATE_PATH = 2,
    NET_EHOME_ESTREAM_INIT_CFG_USERPRIVATEKEY_PATH = 3
}NET_EHOME_ESTREAM_INIT_CFG_TYPE;

typedef struct tagNET_EHOME_LOCAL_PLAYBACK_PARAM
{
    DWORD dwSize;
    BYTE  byPlayBackSync;
    BYTE  byRes[131];
}NET_EHOME_LOCAL_PLAYBACK_PARAM, *LPNET_EHOME_LOCAL_PLAYBACK_PARAM;

NET_DVR_API BOOL CALLBACK NET_ESTREAM_Init();

NET_DVR_API BOOL CALLBACK NET_ESTREAM_Fini();

NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetSDKInitCfg(NET_EHOME_ESTREAM_INIT_CFG_TYPE enumType, void* const lpInBuff);

NET_DVR_API DWORD CALLBACK NET_ESTREAM_GetLastError();

NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetExceptionCallBack(DWORD dwMessage, HANDLE hWnd, void (CALLBACK* fExceptionCallBack)(DWORD dwType, LONG iUserID, LONG iHandle, void* pUser), void* pUser );

NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetLogToFile( LONG iLogLevel, char *strLogDir, BOOL bAutoDel );

NET_DVR_API LONG CALLBACK NET_ESTREAM_SendRealStreamData(LONG lUserID, BYTE byDataType, char *pSendBuf, DWORD dwDataLen);
//get build version
NET_DVR_API DWORD CALLBACK NET_ESTREAM_GetBuildVersion();

NET_DVR_API LONG CALLBACK NET_ESTREAM_StartListenPreview(LPNET_EHOME_LISTEN_PREVIEW_CFG pListenParam);

NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopListenPreview(LONG iListenHandle);

NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopPreview(LONG iPreviewHandle);

NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetPreviewDataCB(LONG iHandle, LPNET_EHOME_PREVIEW_DATA_CB_PARAM pStruCBParam);

NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetStandardPreviewDataCB(LONG iHandle, LPNET_EHOME_PREVIEW_DATA_CB_PARAM pStruCBParam);

#define    NET_EHOME_DEVICEID_LEN        256 //the length of device ID


typedef struct tagNET_EHOME_PLAYBACK_DATA_CB_INFO
{
    DWORD   dwType;                    //type 0-header 1-stream data 15-hls 16-timelapse
    BYTE     *pData;                    //data
    DWORD     dwDataLen;                //data length
    BYTE    byRes[128];
}NET_EHOME_PLAYBACK_DATA_CB_INFO, *LPNET_EHOME_PLAYBACK_DATA_CB_INFO;


typedef BOOL(CALLBACK *PLAYBACK_DATA_CB)(LONG iPlayBackLinkHandle, NET_EHOME_PLAYBACK_DATA_CB_INFO *pDataCBInfo, void* pUserData);

typedef struct tagNET_EHOME_PLAYBACK_NEWLINK_CB_INFO
{
    char         szDeviceID[NET_EHOME_DEVICEID_LEN];  
    LONG         lSessionID;    
    DWORD        dwChannelNo;    
    char         sDeviceSerial[NET_EHOME_SERIAL_LEN/*12*/];
    BYTE         byStreamFormat;         
    BYTE         byRes1[3];
#if (defined(OS_WINDOWS64) || defined(OS_POSIX64))
    PLAYBACK_DATA_CB   fnPlayBackDataCB;   
    void*              pUserData;        
#else
    PLAYBACK_DATA_CB   fnPlayBackDataCB;   
    BYTE               byRes2[4];
    void*              pUserData;       
    BYTE               byRes3[4];
#endif
    BYTE               byRes[88];
}NET_EHOME_PLAYBACK_NEWLINK_CB_INFO, *LPNET_EHOME_PLAYBACK_NEWLINK_CB_INFO;

typedef BOOL (CALLBACK *PLAYBACK_NEWLINK_CB)(LONG lPlayBackLinkHandle, NET_EHOME_PLAYBACK_NEWLINK_CB_INFO *pNewLinkCBInfo, void* pUserData);


typedef struct tagNET_EHOME_PLAYBACK_LISTEN_PARAM
{
    NET_EHOME_IPADDRESS struIPAdress;   //Local listening information, if the IP address is *.0.0.0, it is considered as the local address. 
    PLAYBACK_NEWLINK_CB fnNewLinkCB;    //The callback function of requiring playback 
    void*               pUserData;        //User data 
    BYTE                byLinkMode;     //The listening connection mode,0- TCP,1- UDP(UDP reserved) 
    BYTE                byLinkEncrypt;  //
    BYTE                byRes[126];
}NET_EHOME_PLAYBACK_LISTEN_PARAM, *LPNET_EHOME_PLAYBACK_LISTEN_PARAM;


typedef struct tagNET_EHOME_PLAYBACK_DATA_CB_PARAM
{
    PLAYBACK_DATA_CB    fnPlayBackDataCB;        //data callback function
    void*                pUserData;                //user data
    BYTE                byStreamFormat;            //Stream format,0-PS 1-RTP 
    BYTE                 byRes[127];            
}NET_EHOME_PLAYBACK_DATA_CB_PARAM, *LPNET_EHOME_PLAYBACK_DATA_CB_PARAM;

#define    EHOME_PREVIEW_EXCEPTION        0x102     //preview exception
#define    EHOME_PLAYBACK_EXCEPTION       0x103     //playback exception
#define    EHOME_AUDIOTALK_EXCEPTION         0x104     //audio talk exception

#define NET_EHOME_SYSHEAD           1    //stream header
#define NET_EHOME_STREAMDATA        2    //stream data
#define NET_EHOME_STREAMEND         3    //stream end
#define NET_EHOME_HLS               15   //hls
#define NET_EHOME_TIMELAPSE         16   //timelapse

NET_DVR_API LONG CALLBACK NET_ESTREAM_StartListenPlayBack(LPNET_EHOME_PLAYBACK_LISTEN_PARAM pListenParam);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetPlayBackDataCB(LONG iPlayBackLinkHandle, NET_EHOME_PLAYBACK_DATA_CB_PARAM *pDataCBParam);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopPlayBack(LONG iPlayBackLinkHandle);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopListenPlayBack(LONG iPlaybackListenHandle);

//--------------------------------------------------------------------------------------------------------------
#define NET_EHOME_DEVICEID_LEN      256
#define NET_EHOME_SERIAL_LEN        12

typedef struct tagNET_EHOME_VOICETALK_DATA_CB_INFO
{
    BYTE         *pData;
    DWORD        dwDataLen;
    BYTE         byRes[128];
}NET_EHOME_VOICETALK_DATA_CB_INFO, *LPNET_EHOME_VOICETALK_DATA_CB_INFO;

typedef BOOL(CALLBACK *VOICETALK_DATA_CB)(LONG lHandle, NET_EHOME_VOICETALK_DATA_CB_INFO *pDataCBInfo, void* pUserData);

typedef struct tagNET_EHOME_VOICETALK_NEWLINK_CB_INFO
{
    BYTE    szDeviceID[NET_EHOME_DEVICEID_LEN/*256*/];  
    DWORD   dwEncodeType; 
    char    sDeviceSerial[NET_EHOME_SERIAL_LEN/*12*/];  
    DWORD   dwAudioChan; 
    LONG    lSessionID; 
    BYTE    byToken[64];
#if (defined(OS_WINDOWS64) || defined(OS_POSIX64))
    VOICETALK_DATA_CB  fnVoiceTalkDataCB;  
    void               *pUserData;        
#else
    VOICETALK_DATA_CB  fnVoiceTalkDataCB;  
    BYTE               byRes1[4];
    void               *pUserData;        
    BYTE               byRes2[4];
#endif 
    BYTE               byRes[48];
} NET_EHOME_VOICETALK_NEWLINK_CB_INFO, *LPNET_EHOME_VOICETALK_NEWLINK_CB_INFO;


typedef BOOL (CALLBACK *VOICETALK_NEWLINK_CB)(LONG lHandle, NET_EHOME_VOICETALK_NEWLINK_CB_INFO *pNewLinkCBInfo, void* pUserData);

typedef struct tagNET_EHOME_LISTEN_VOICETALK_CFG
{
    NET_EHOME_IPADDRESS struIPAdress;
    VOICETALK_NEWLINK_CB     fnNewLinkCB;
    void*          pUser;
    BYTE           byLinkEncrypt;  
    BYTE           byRes[127];
}NET_EHOME_LISTEN_VOICETALK_CFG, *LPNET_EHOME_LISTEN_VOICETALK_CFG;

typedef struct tagNET_EHOME_VOICETALK_DATA_CB_PARAM
{
    VOICETALK_DATA_CB    fnVoiceTalkDataCB;
    void       *pUserData;
    BYTE       byRes[128];
}NET_EHOME_VOICETALK_DATA_CB_PARAM, *LPNET_EHOME_VOICETALK_DATA_CB_PARAM;

typedef struct tagNET_EHOME_VOICETALK_DATA
{
    BYTE     *pSendBuf;
    DWORD     dwDataLen;
    DWORD     dwTimeout;  //0-(default)5000ms
    BYTE      byRes[124];
}NET_EHOME_VOICETALK_DATA, *LPNET_EHOME_VOICETALK_DATA;

NET_DVR_API LONG CALLBACK NET_ESTREAM_StartListenVoiceTalk(LPNET_EHOME_LISTEN_VOICETALK_CFG pListenParam);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopListenVoiceTalk(LONG lListenHandle);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetVoiceTalkDataCB(LONG lHandle, LPNET_EHOME_VOICETALK_DATA_CB_PARAM pStruCBParam);
NET_DVR_API LONG CALLBACK NET_ESTREAM_SendVoiceTalkData (LONG lHandle, LPNET_EHOME_VOICETALK_DATA pVoicTalkData);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_StopVoiceTalk(LONG lHandle);



NET_DVR_API BOOL CALLBACK NET_ESTREAM_SetSDKLocalCfg(NET_EHOME_LOCAL_CFG_TYPE enumType, void* const lpInBuff);
NET_DVR_API BOOL CALLBACK NET_ESTREAM_GetSDKLocalCfg(NET_EHOME_LOCAL_CFG_TYPE enumType, void *lpOutBuff);


#endif //_HC_EHOME_STREAM_H_
