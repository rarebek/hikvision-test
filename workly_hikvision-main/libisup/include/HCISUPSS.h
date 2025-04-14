#ifndef _HC_EHOME_SS_H_
#define _HC_EHOME_SS_H_

#include "HCISUPPublic.h"

#define MAX_URL_LEN_SS          4096
#define MAX_KMS_USER_LEN        512
#define MAX_KMS_PWD_LEN         512
#define MAX_CLOUD_AK_SK_LEN     64

#define SS_CLIENT_FILE_PATH_PARAM_NAME  "File-Path"
#define SS_CLIENT_VRB_FILENAME_CODE     "Filename-Code"
#define SS_CLIENT_KMS_USER_NAME         "KMS-Username"
#define SS_CLIENT_KMS_PASSWIRD          "KMS-Password"
#define SS_CLIENT_CLOUD_AK_NAME         "Access-Key"
#define SS_CLIENT_CLOUD_SK_NAME         "Secret-Key"
#define SS_CLIENT_CLOUD_POOL_ID         "Pool-Id"
#define SS_CLIENT_CLOUD_SERIAL_ID       "Serial-Id"
#define SS_CLIENT_CENTRAL_AK_NAME       "Central-Access-Key"
#define SS_CLIENT_CENTRAL_SK_NAME       "Central-Secret-Key"
#define SS_CLIENT_CENTRAL_POOL_ID       "Central-Pool-Id"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

enum NET_EHOME_SS_MSG_TYPE
{
    NET_EHOME_SS_MSG_TOMCAT = 1,       //Tomcat callback
    NET_EHOME_SS_MSG_KMS_USER_PWD,     //KMS's username&password callback
    NET_EHOME_SS_MSG_CLOUD_AK          //EHome5.0 storage protocol's AK callback
};

enum NET_EHOME_SS_CLIENT_TYPE
{
    NET_EHOME_SS_CLIENT_TYPE_TOMCAT = 1,       //Tomcat client
    NET_EHOME_SS_CLIENT_TYPE_VRB,              //VRB client
    NET_EHOME_SS_CLIENT_TYPE_KMS,              //KMS client
    NET_EHOME_SS_CLIENT_TYPE_CLOUD,             //EHome5.0 storage protocol client
    NET_EHOME_SS_CLIENT_TYPE_CENTRAL           //Central storage protocol client
};

//picture server callback param
typedef struct tagNET_EHOME_SS_TOMCAT_MSG
{
    char    szDevUri[MAX_URL_LEN_SS];
    DWORD   dwPicNum;
    char*   pPicURLs;
    BYTE    byRes[64];
}NET_EHOME_SS_TOMCAT_MSG, *LPNET_EHOME_SS_TOMCAT_MSG;

typedef struct tagNET_EHOME_SS_KMS_MSG
{
    char* strUserName;                      
    char* strPassword;                   
    BYTE  byRes[64];
}NET_EHOME_SS_KMS_MSG, *LPNET_EHOME_SS_KMS_MSG;

typedef struct tagNET_EHOME_SS_RW_PARAM
{
    const char* pFileName;
    void* pFileBuf;
    DWORD* dwFileLen;
    const char* pFileUrl;
    void* pUser;
    BYTE byAct;
    BYTE byUseRetIndex;
    BYTE  byRes1[2];
    char *pRetIndex;
    BYTE  byRes2[56];
}NET_EHOME_SS_RW_PARAM, *LPNET_EHOME_SS_RW_PARAM;

//central storage
typedef struct tagNET_EHOME_SS_CENTRAL_PARAM
{
    const char*  pPoolId;             //poolId 
    const char*  pSerialID;           //serial ID
    const char*  pSerialIDUUID;       //SerialID+UUID
    const char*  pBeginTime;          //begin time
    const char*  pEndTime;            //end time
    BYTE         byPoolIdLength;      //poolid length
    BYTE         bySerialIDLength;    //SerialID length
    BYTE         bySerialIDUUIDLength;//SerialID+UUID length
    BYTE         byBeginTimeLength;   //begin time length
    BYTE         byEndTimeLength;     //end time length
    BYTE         byTransform;         //transform£º0 - no£¬1-yes
    BYTE         byRes1[2];
    DWORD          dwRecordType;        //record type
    DWORD          dwSourceDataType;    //video type
    DWORD          dwHeadSize;           //video head length
    DWORD          dwErrorCode;         //error code
    BYTE         byRes[468];
}NET_EHOME_SS_CENTRAL_PARAM, *LPNET_EHOME_SS_CENTRAL_PARAM;

typedef struct tagNET_EHOME_SS_CLOUD_PARAM
{
    const char*  pPoolId;
    BYTE byPoolIdLength;
    int dwErrorCode;
    BYTE  byRes[503];
}NET_EHOME_SS_CLOUD_PARAM, *LPNET_EHOME_SS_CLOUD_PARAM;


typedef struct tagNET_EHOME_SS_KMS_PARAM
{
    BYTE  byRes[512];
}NET_EHOME_SS_KMS_PARAM, *LPNET_EHOME_SS_KMS_PARAM;


typedef struct tagNET_EHOME_SS_TOMCAT_PARAM
{
    BYTE  byRes[512];
}NET_EHOME_SS_TOMCAT_PARAM, *LPNET_EHOME_SS_TOMCAT_PARAM;


typedef struct tagNET_EHOME_SS_VRB_PARAM
{
    BYTE  byRes[512];
}NET_EHOME_SS_VRB_PARAM, *LPNET_EHOME_SS_VRB_PARAM;

typedef struct tagNET_EHOME_SS_EX_PARAM
{
    BYTE byProtoType;
    BYTE byRes[23];
    union
    {
        NET_EHOME_SS_CLOUD_PARAM struCloud;
        NET_EHOME_SS_TOMCAT_PARAM struTomcat;
        NET_EHOME_SS_KMS_PARAM struKms;
        NET_EHOME_SS_VRB_PARAM struVrb;
        NET_EHOME_SS_CENTRAL_PARAM struCentral;
    }unionStoreInfo;

}NET_EHOME_SS_EX_PARAM, *LPNET_EHOME_SS_EX_PARAM;

//message callback function
typedef BOOL(CALLBACK *EHomeSSMsgCallBack)(LONG iHandle, NET_EHOME_SS_MSG_TYPE enumType
    , void *pOutBuffer, DWORD dwOutLen, void *pInBuffer, DWORD dwInLen, void *pUser);

//storage callback function
typedef BOOL(CALLBACK *EHomeSSStorageCallBack)(LONG iHandle, const char* pFileName, void *pFileBuf, DWORD dwFileLen, char *pFilePath, void *pUser);

//read write callback function byAct 0-write 1-read 2-delete
typedef BOOL(CALLBACK *EHomeSSRWCallBack)(LONG iHandle, BYTE byAct, const char* pFileName
    , void *pFileBuf, LONG* dwFileLen, const char* pFileUrl, void *pUser);

//read write callback function EX byAct 0-write 1-read 2-delete 
typedef BOOL(CALLBACK *EHomeSSRWCallBackEx)(LONG iHandle, NET_EHOME_SS_RW_PARAM* pRwParam, NET_EHOME_SS_EX_PARAM* pExStruct);


typedef struct tagNET_EHOME_SS_LISTEN_PARAM
{
    NET_EHOME_IPADDRESS struAddress;
    char szKMS_UserName[MAX_KMS_USER_LEN];
    char szKMS_Password[MAX_KMS_PWD_LEN];
    EHomeSSStorageCallBack  fnSStorageCb;
    EHomeSSMsgCallBack      fnSSMsgCb;
    char szAccessKey[MAX_CLOUD_AK_SK_LEN];
    char szSecretKey[MAX_CLOUD_AK_SK_LEN];
    void* pUserData;
    BYTE  byHttps;
    BYTE  byRes1[3];
    EHomeSSRWCallBack   fnSSRWCb;
    EHomeSSRWCallBackEx fnSSRWCbEx;
    BYTE  bySecurityMode; // [add] by yangzheng 2020/03/13 storage server safety switch£¬0-open£¬1-close
    BYTE  byRes[51];
}NET_EHOME_SS_LISTEN_PARAM, *LPNET_EHOME_SS_LISTEN_PARAM;


//listen Https param
typedef struct tagNET_EHOME_SS_LISTEN_HTTPS_PARAM
{
    BYTE byHttps;      
    BYTE byVerifyMode;
    BYTE byCertificateFileType;
    BYTE byPrivateKeyFileType; 
    char szUserCertificateFile[MAX_PATH]; 
    char szUserPrivateKeyFile[MAX_PATH];  
    DWORD dwSSLVersion; //0 - SSL23, 1 - SSL2, 2 - SSL3, 3 - TLS1.0, 4 - TLS1.1, 5 - TLS1.2
    BYTE byRes3[360];
}NET_EHOME_SS_LISTEN_HTTPS_PARAM, *LPNET_EHOME_SS_LISTEN_HTTPS_PARAM;

//client param
typedef struct tagNET_EHOME_SS_CLIENT_PARAM
{
    NET_EHOME_SS_CLIENT_TYPE enumType; //client type
    NET_EHOME_IPADDRESS struAddress;    
    BYTE  byHttps;                 
    BYTE  byRes[63];
}NET_EHOME_SS_CLIENT_PARAM, *LPNET_EHOME_SS_CLIENT_PARAM;

enum NET_EHOME_SS_INIT_CFG_TYPE
{
    NET_EHOME_SS_INIT_CFG_SDK_PATH = 1,     //Image storage database path Settings, path with double slash \\ end
    NET_EHOME_SS_INIT_CFG_CLOUD_TIME_DIFF = 2,
    NET_EHOME_SS_INIT_CFG_PUBLIC_IP_PORT = 3,
    NET_EHOME_SS_INIT_CFG_LIBEAY_PATH = 4,
    NET_EHOME_SS_INIT_CFG_SSLEAY_PATH = 5,
    NET_EHOME_SS_INIT_CFG_SQLITE3_PATH = 6
};

typedef struct tagNET_EHOME_SS_LOCAL_SDK_PATH{
    char    sPath[MAX_PATH];
    BYTE    byRes[128];
}NET_EHOME_SS_LOCAL_SDK_PATH, *LPNET_EHOME_SS_LOCAL_SDK_PATH;

enum NET_EHOME_SS_TYPE
{
    NET_EHOME_SS_TYPE_TOMCAT = 1,       //Tomcat
    NET_EHOME_SS_TYPE_VRB,              //VRB
    NET_EHOME_SS_TYPE_KMS,              //KMS
    NET_EHOME_SS_TYPE_CLOUD,            //Cloud
    NET_EHOME_SS_TYPE_CENTRAL           //Central storage
};

typedef struct tagNET_EHOME_SS_STORAGE_URI
{
    NET_EHOME_SS_TYPE enumType;
    char  szFilename[256];
    char  szUri[MAX_URL_LEN_SS/*4096*/];
    BYTE  byRes[64];
}NET_EHOME_SS_STORAGE_URI, *LPNET_EHOME_SS_STORAGE_URI;

#define SS_DB_KEY_MAX_LEN 32

typedef struct tagNET_EHOME_SS_INIT_PARAM
{
    char   szKey[SS_DB_KEY_MAX_LEN/*32*/]; //DataBase Key
    BYTE   byRes[224];
}NET_EHOME_SS_INIT_PARAM, *LPNET_EHOME_SS_INIT_PARAM;


//SS Library Initialization
NET_DVR_API BOOL  CALLBACK NET_ESS_Init();
NET_DVR_API BOOL  CALLBACK NET_ESS_Init_V11(NET_EHOME_SS_INIT_PARAM *pParam);
NET_DVR_API BOOL  CALLBACK NET_ESS_Fini();

//Return the error code.
NET_DVR_API DWORD CALLBACK NET_ESS_GetLastError();

//log
NET_DVR_API BOOL CALLBACK NET_ESS_SetLogToFile(LONG iLogLevel, const char *strLogDir, BOOL bAutoDel);

//Get the SS library version
NET_DVR_API DWORD CALLBACK NET_ESS_GetBuildVersion();

NET_DVR_API BOOL CALLBACK NET_ESS_SetListenHttpsParam(NET_EHOME_SS_LISTEN_HTTPS_PARAM* pSSHttpsParam);

NET_DVR_API BOOL CALLBACK NET_ESS_GenerateStorageUri(NET_EHOME_SS_STORAGE_URI* pSSStorageUri);

NET_DVR_API LONG CALLBACK NET_ESS_StartListen(NET_EHOME_SS_LISTEN_PARAM* pSSListenParam);

NET_DVR_API BOOL  CALLBACK NET_ESS_StopListen(LONG lListenHandle);

NET_DVR_API BOOL CALLBACK NET_ESS_SetSDKInitCfg(NET_EHOME_SS_INIT_CFG_TYPE enumType, void* const lpInBuff);

NET_DVR_API LONG CALLBACK NET_ESS_CreateClient(NET_EHOME_SS_CLIENT_PARAM* pClientParam);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientSetTimeout(LONG lHandle, DWORD dwSendTimeout, DWORD dwRecvTimeout);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientSetParam(LONG lHandle, const char* strParamName, const char* strParamVal);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientDoUpload(LONG lHandle, char* strUrl, LONG dwUrlLen);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientDoDownload(LONG lHandle, char* strUrl, void** pFileContent, DWORD& dwContentLen);

NET_DVR_API BOOL CALLBACK NET_ESS_DestroyClient(LONG lHandle);

NET_DVR_API BOOL CALLBACK NET_ESS_HAMSHA256(const char* pSrc, const char* pSecretKey, char* pSingatureOut, DWORD dwSingatureLen);

NET_DVR_API BOOL CALLBACK NET_ESS_GENERATE_SECRETKEY(const char* pSrc, const char* pKey, char* pSecretKeyOut, DWORD dwSecretKeyLen);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientDoDelete(LONG lHandle, char* strUrl);

NET_DVR_API BOOL CALLBACK NET_ESS_ClientDoUploadBuffer(LONG lHandle, char* strUrl, DWORD dwUrlLen, void*pFileContent, DWORD dwContentLen);
#endif //_HC_EHOME_SS_H_

