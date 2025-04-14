#ifndef _HC_ISUP_IPS_H_
#define _HC_ISUP_IPS_H_

#include "HCISUPPublic.h"

#define   MAX_INFO_PUBLISH_LISTEN_NUM       10
#define   MAX_FILE_PATH  260

#define	NET_SDK_UPG_STATUS_SUCCESS		1000
#define NET_SDK_UPG_STATUS_PROCESSING	1001
#define	NET_SDK_UPG_STATUS_FAILED		1002
#define NET_SDK_UPG_EXTRACT_FAILED      1003
#define NET_SDK_UPG_APK_VERSION_FAILED  1004
#define NET_SDK_UPG_ROM_VERSION_FAILED  1005
#define NET_SDK_UPG_PKG_VER_FAIL        1006
#define NET_SDK_UPG_SIGN_FAILED         1007
#define NET_SDK_UPG_PKG_VER_SUCCESS     1008

#define	NET_SDK_INFO_PUBLISH_PROCESSING		 1
#define	NET_SDK_INFO_PUBLISH_FAILED		 2
#define NET_SDK_INFO_PUBLISH_NO_MEMORY   3
#define NET_SDK_INFO_PUBLISH_CANCEL      4
#define NET_SDK_INFO_PUBLISH_LEN_FAILED  6
#define NET_SDK_INFO_PUBLISH_DATA_FAILED 7
#define NET_SDK_INFO_PUBLISH_SEND_FAILED 8

typedef struct
{
    DWORD dwType;
    DWORD dwId;
    DWORD dwIndex;
    BYTE  byIdentifyCode[32];
    DWORD dwTimingPlanId;
    BYTE  byRes[16];
}NET_EHOME_SCHEDULE_PUBLISH_HEAD;

typedef struct
{
    DWORD dwFileType;
    DWORD dwId;
    DWORD dwPageId;
    DWORD dwIndex;
    DWORD dwFileLen;
    BYTE  byRes[12];
    char * pXMLData;
    char  szFilePath[MAX_FILE_PATH];
}NET_EHOME_PUBLISH_SINGLE_FILE;

typedef struct
{
    WORD wFileNumber;
    BYTE  byRes[2];
    char * pFilesData;
    BYTE  byRes1[32];
}NET_EHOME_SCHEDULE_PUBLISH_FILE;

typedef BOOL(CALLBACK * INFO_PUBLISH_CB)(LONG iLinkHandle,DWORD dwDataType, void *pOutBuffer, DWORD dwOutLen,
    void *pInBuffer, DWORD dwInLen, void *pUser);

typedef struct{
    NET_EHOME_IPADDRESS    struIPAddress;
    INFO_PUBLISH_CB        fnCB;
    void                   *pUser;
    BYTE                   byType;
    BYTE                   byRes[31];
}NET_EHOME_INFO_PUBLISH_PARAM, *LPNET_EHOME_INFO_PUBLISH_PARAM;

typedef struct{
    DWORD dwSeq;
    DWORD dwScreenPicLen;
    char *pScreenPic;
    BYTE                   byRes[32];
}NET_EHOME_SCREEN_SHOT_PARAM, *LPNET_EHOME_SCREEN_SHOT_PARAM;

typedef struct{
    char sUpgradeFilePath[MAX_FILE_PATH];
    BYTE byRes[32];
}NET_EHOME_UPGRADE_INFO, *LPNET_EHOME_UPGRADE_INFO;

typedef struct{
    DWORD dwSize;
    DWORD dwStatus;
    DWORD dwPercent;
}NET_EHOME_UPGRADE_STATUS, *LPNET_EHOME_UPGRADE_STATUS;

typedef enum tagNET_EHOME_INFO_PUBLISH_TYPE{
    ENUM_SCHEDULE_PUBLICH = 0,
    ENUM_SCREEN_SHOT,
    ENUM_DEV_UPGRADE,
}NET_EHOME_INFO_PUBLISH_TYPE;

NET_DVR_API BOOL  CALLBACK NET_EIPS_Init();
NET_DVR_API BOOL  CALLBACK NET_EIPS_Fini();

NET_DVR_API DWORD CALLBACK NET_EIPS_GetLastError();

NET_DVR_API BOOL CALLBACK NET_EIPS_SetLogToFile(LONG iLogLevel, const char *strLogDir, BOOL bAutoDel);

NET_DVR_API LONG CALLBACK NET_EIPS_InfoPublishListen(LPNET_EHOME_INFO_PUBLISH_PARAM lpInfoPublishPara);
NET_DVR_API BOOL CALLBACK NET_EIPS_StopInfoPublisListen(LONG lHandle);
NET_DVR_API BOOL NET_EIPS_GetProgress(LONG lHandle, LONG iLinkHandle, LPDWORD pProgress);
NET_DVR_API BOOL NET_EIPS_GetUploadState(LONG lHandle, LONG iLinkHandle, LPDWORD pStatus);

#endif //_HC_ISUP_IPS_H_
