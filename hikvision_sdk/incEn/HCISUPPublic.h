#ifndef _HC_ISUP_PUBLIC_H_
#define _HC_ISUP_PUBLIC_H_

#ifndef _HC_NET_SDK_H_

//------------OS data type begin----------------------------------
#if (defined(_WIN32)) //windows
#define NET_DVR_API  extern "C" __declspec(dllimport)
typedef     unsigned __int64    UINT64;
#elif defined(__linux__) || defined(__APPLE__) //linux
typedef     unsigned int        DWORD;
typedef     unsigned short      WORD;
typedef     unsigned short      USHORT;
typedef     short               SHORT;
typedef     int                 LONG;
typedef      unsigned char      BYTE;
#define     BOOL int
typedef     unsigned int        UINT;
typedef     void*               LPVOID;
typedef     void*               HANDLE;
typedef     unsigned int*       LPDWORD;
typedef     unsigned long long  UINT64;

#ifndef    TRUE
#define    TRUE     1
#endif
#ifndef    FALSE
#define    FALSE    0
#endif
#ifndef    NULL
#define    NULL     0
#endif

#define __stdcall
#define CALLBACK

#define NET_DVR_API extern "C"
#endif //linux

#if defined(_WIN64)
#define OS_WINDOWS64    1
#endif

#if defined(__LP64__)
#define OS_POSIX64    1 
#endif

//------------OS data type end----------------------------------

//-------------Macro Define(HCNetSDK Defined)Begin--------
#define MAX_DEVNAME_LEN         32
#define MAX_DEVNAME_LEN_EX      64
#define NAME_LEN                32
#define MAX_TIME_LEN            32
//-------------Macro Define(HCNetSDK Defined)End--------

//-------------ErrorCode Define(HCNetSDK Defined)Begin--------
#define NET_DVR_NOERROR                         0   //No error.
#define NET_DVR_PASSWORD_ERROR                  1   //User name or password error.
#define NET_DVR_NOENOUGHPRI                     2   //Not authorized to do this operation.
#define NET_DVR_NOINIT                          3   //SDK is not initialized..
#define NET_DVR_CHANNEL_ERROR                   4   //Channel number error. There is no corresponding channel number on the device.
#define NET_DVR_OVER_MAXLINK                    5   //The number of connection with the device has exceeded the max limit.
#define NET_DVR_VERSIONNOMATCH                  6   //Version mismatch. SDK version is not matching with the device.
#define NET_DVR_NETWORK_FAIL_CONNECT            7   //Failed to connect to the device. The device is off-line, or connection timeout caused by network. .
#define NET_DVR_NETWORK_SEND_ERROR              8   //Failed to send data to the device.
#define NET_DVR_NETWORK_RECV_ERROR              9   //Failed to receive data from the device.
#define NET_DVR_NETWORK_RECV_TIMEOUT            10  //Timeout when receiving the data from the device.
#define NET_DVR_NETWORK_ERRORDATA               11  //The data sent to the device is illegal, or the data received from the device error. E.g. The input data is not supported by the device for remote configuration.
#define NET_DVR_ORDER_ERROR                     12  //API calling order error.
#define NET_DVR_OPERNOPERMIT                    13  //Not authorized for this operation.
#define NET_DVR_COMMANDTIMEOUT                  14  //Executing command on the device is timeout.
#define NET_DVR_PARAMETER_ERROR                 17  //Parameter error. Input or output parameters in the SDK API is NULL, or the value or format of the parameters does not match with the requirement.
#define NET_DVR_NOSUPPORT                       23  //Device does not support this function.
#define NET_DVR_DVROPRATEFAILED                 29  //Device operation failed.
#define NET_DVR_FILEOPENFAIL                    35  //Open File Failed
#define NET_DVR_FILEFORMAT_ERROR                39  //Invalid File Format
#define NET_DVR_DIR_ERROR                       40  //File directory error.
#define NET_DVR_ALLOC_RESOURCE_ERROR            41  //Resource allocation error.
#define NET_DVR_AUDIO_MODE_ERROR                42  //Sound adapter mode error. Currently opened sound playing mode does not match with the set mode.
#define NET_DVR_NOENOUGH_BUF                    43  //Buffer is not enough.
#define NET_DVR_CREATESOCKET_ERROR              44  //Create SOCKET error.
#define NET_DVR_SETSOCKET_ERROR                 45  //Set SOCKET error.
#define NET_DVR_MAX_NUM                         46  //The number of login or preview connections has exceeded the SDK limitation.
#define NET_DVR_USERNOTEXIST                    47  //User doest not exist. The user ID has been logged out or unavailable. 
#define NET_DVR_GETLOCALIPANDMACFAIL            53  //Failed to get the IP address or physical address of local PC.
#define NET_DVR_VOICEMONOPOLIZE                 69  //Sound adapter has been monopolized.
#define NET_DVR_JOINMULTICASTFAILED             70
#define NET_DVR_CREATEDIR_ERROR                 71  //Failed to create log file directory.
#define NET_DVR_BINDSOCKET_ERROR                72  //Failed to bind socket.
#define NET_DVR_SOCKETCLOSE_ERROR               73  //Socket disconnected. It is caused by network disconnection or destination unreachable.
#define NET_DVR_USERID_ISUSING                  74  //The user ID is operating when logout..
#define NET_DVR_SOCKETLISTEN_ERROR              75  //Failed to listen.
#define NET_DVR_CONVERT_SDK_ERROR               85  //Load SystemTransform.dll failed
#define NET_DVR_FUNCTION_NOT_SUPPORT_OS         98  //This function don't support this OS.
#define NET_DVR_INTERCOM_SDK_ERROR              100
#define NET_DVR_USE_LOG_SWITCH_FILE             103 //Using the log switch file
#define NET_DVR_PACKET_TYPE_NOT_SUPPORT         105 //Packet type is not supported
#define NET_DVR_STREAM_ENCRYPT_CHECK_FAIL       130 //stream encrypt check fail
#define NET_DVR_CERTIFICATE_FILE_ERROR          147
#define NET_DVR_LOAD_SSL_LIB_ERROR              148
#define NET_DVR_SSL_VERSION_NOT_MATCH           149
#define NET_DVR_LOAD_LIBEAY32_DLL_ERROR         156
#define NET_DVR_LOAD_SSLEAY32_DLL_ERROR         157
#define NET_ERR_LOAD_LIBICONV                   158
#define NET_ERR_SSL_CONNECT_FAILED              159
#define NET_ERR_LOAD_ZLIB                       161
#define NET_ERR_OPENSSL_NO_INIT                 162

#define  NET_PREVIEW_ERR_CHANNEL_BUSY               165
#define  NET_PREVIEW_ERR_CLIENT_BYSY                166
#define  NET_PREVIEW_ERR_STREAM_UNSUPPORT           167
#define  NET_PREVIEW_ERR_TRANSPORT_UNSUPPORT        168
#define  NET_PREVIEW_ERR_CONNECT_SERVER_FAIL        169
#define  NET_PREVIEW_ERR_QUERY_WLAN_INFO_FAIL       170
#define  NET_PREVIEW_ERR_NO_VIDEO_FAIL              171
#define  NET_PREVIEW_ERR_SET_ENCODE_PARAM_FAIL      172
#define  NET_PREVIEW_ERR_SET_PACK_TYPE_FAIL         173
#define  NET_PREVIEW_ERR_NOW_IN_PREVIEW_FAIL        174
#define  NET_PREVIEW_ERR_NOW_IN_PRESTREAM_FAIL      175
#define  NET_PREVIEW_ERR_BREAKOFF_PRESTREAM_FAIL    176
#define  NET_PREVIEW_ERR_P2P_NOT_FOUND              177
#define  NET_PREVIEW_ERR_SLEEP_STREAM_UNSUPPORT     178

#define NET_SDK_ERR_CREATE_PORT_MULTIPLEX       184
#define NET_SDK_ERR_MAX_PORT_MULTIPLEX          187
//Error Code of Voice Talk Library 
#define  NET_AUDIOINTERCOM_OK                   600 //No error.
#define  NET_AUDIOINTECOM_ERR_NOTSUPORT         601 //Not support.
#define  NET_AUDIOINTECOM_ERR_ALLOC_MEMERY      602 //Memory allocation error.
#define  NET_AUDIOINTECOM_ERR_PARAMETER         603 //Parameter error.
#define  NET_AUDIOINTECOM_ERR_CALL_ORDER        604 //API calling order error.
#define  NET_AUDIOINTECOM_ERR_FIND_DEVICE       605 //No audio device
#define  NET_AUDIOINTECOM_ERR_OPEN_DEVICE       606 //Failed to open the audio device
#define  NET_AUDIOINTECOM_ERR_NO_CONTEXT        607 //Context error.
#define  NET_AUDIOINTECOM_ERR_NO_WAVFILE        608 //WAV file error.
#define  NET_AUDIOINTECOM_ERR_INVALID_TYPE      609 //The type of WAV parameter is invalid
#define  NET_AUDIOINTECOM_ERR_ENCODE_FAIL       610 //Failed to encode data
#define  NET_AUDIOINTECOM_ERR_DECODE_FAIL       611 //Failed to decode data
#define  NET_AUDIOINTECOM_ERR_NO_PLAYBACK       612 //Failed to play audio
#define  NET_AUDIOINTECOM_ERR_DENOISE_FAIL      613 //Failed to denoise
#define  NET_AUDIOINTECOM_ERR_UNKOWN            619 //Unknown
//-------------ErrorCode Define(HCNetSDK Defined)End--------

#endif //_HC_NET_SDK_H_

//-------------Macro Define(HCISUPSDK Only)Begin--------
#define MAX_DEVICE_ID_LEN           256     //the lenght of device ID
#define NET_EHOME_SERIAL_LEN        12
#define MAX_FULL_SERIAL_NUM_LEN     64
#define MAX_MASTER_KEY_LEN          16
#define MAX_FIRMWARE_IDENT_CODE_LEN 128

#define REGISTER_LISTEN_MODE_ALL    0
#define REGISTER_LISTEN_MODE_UDP    1
#define REGISTER_LISTEN_MODE_TCP    2
//-------------Macro Define(HCISUPSDK Only)End--------

//-------------ErrorCode Define(HCISUPSDK Only)Begin--------
#define NET_ERR_INFOPUBLISH_APP_NOTRUN          1065 //info publish app not run
#define NET_ERR_ATTENDANCE_APP_NOTRUN           1066 //attendance app not run

#define    NET_ERR_FILE_NOT_EXIST                        1352    
//Large file transfer error code
#define NET_DVR_LF_INTERFACE_REPEAT_CALL               2400  //Interface invocation
#define NET_DVR_LF_INTERFACE_REPEAT_FIRST_PACK         2401  //Repeat the first packet
#define NET_DVR_LF_INTERFACE_NO_FIRST_PACK             2402  //Unmarked first package
#define NET_DVR_READFILE_FAILED                        2403  //Read file failed
#define NET_DVR_FILE_TOO_LARGE                         2404  
#define NET_DVR_FILE_ILLEGAL                           2405  
#define NET_DVR_INVALID_URL                            2406  

#define NET_ERR_TERM_NAME_REPEAT                1313 //Terminal name duplication
#define NET_ERR_TERM_SERIAL_REPEAT              1314 //Terminal Sequence Number Repetition

#define NET_DVR_LOAD_SQLITE_ERROR               254  //load sqlite.dll fail
#define NET_DVR_SQLITE_VERSION_NOT_MATCH        255  //sqlite version mismatch 

#define NET_SS_CLIENT_ERR_KMS_TOKEN_FAIL        3601 //KMS picture upload error, get token fail
#define NET_SS_CLIENT_ERR_KMS_UPLOAD_FAIL       3602 //KMS picture upload error, upload fail
#define NET_SS_CLIENT_ERR_CLOUD_POOLIST_FAIL    3603 //cloud storage, get poollist fail
#define NET_SS_CLIENT_ERR_CLOUD_BESTNODE_FAIL   3604 //cloud storage, get bestnode fail
#define NET_SS_CLIENT_ERR_DOWNLOAD_PIC_FAIL     3605 //EHome5.0 storage protocol failed to download image
#define NET_SS_CLIENT_ERR_DELETE_PIC_FAIL       3606 //EHome5.0storage protocol failed to delete image
#define NET_SS_CLIENT_ERR_PROTO_UNSAFE          3607 //EHome5.0, storage server in security mode, nonsupport unsafe protocol VRB/Tomcat 
#define NET_SS_CLIENT_ERR_FILE_INEXISTED        3608 // EHome5.0, delete or download file not existed
#define NET_SS_CLIENT_ERR_AUTH_FAILED           3609 // authentication failed
#define NET_SS_CLIENT_ERR_UPLOAD_FAIL           3610 // picture upload failed, Clound\VRB\Tomcat
#define NET_SS_CLIENT_ERR_MAX_FILE_LEN          3611 //picture upload failed, exceed max file len 50M

#define NET_SS_CLIENT_ERR_DOWNLOAD_VIDEOFILE_FAIL     3612 //central storage protocol failed to download video file
#define NET_SS_CLIENT_ERR_DELETE_VIDEOFILE_FAIL       3613 //central storage protocol failed to delete video file

#define NET_DVR_ERR_GENERAL_UNKNOW_ERROR                                3701
#define NET_DVR_ERR_GENERAL_PARSE_FAILED                                3702
#define NET_DVR_ERR_GENERAL_SYSTEM_ERROR                                3703
#define NET_DVR_ERR_GENERAL_COMMAND_UNKNOW                              3704
#define NET_DVR_ERR_GENERAL_COMMAND_NO_LONGER_SUPPORTED                 3705
#define NET_DVR_ERR_GENERAL_COMMAND_NOT_SUITABLE                        3706
#define NET_DVR_ERR_GENERAL_COMMAND_NOT_ALLOW                           3707
#define NET_DVR_ERR_GENERAL_CHECKSUM_ERROR                              3708
#define NET_DVR_ERR_GENERAL_HEADER_INVALID                              3709
#define NET_DVR_ERR_GENERAL_LENGTH_INVALID                              3710
#define NET_DVR_ERR_GENERAL_PU_BUSY                                     3711
#define NET_DVR_ERR_GENERAL_OPERATION_FAILED                            3712
#define NET_DVR_ERR_GENERAL_PU_NO_CRYPTO_FOUND                          3713
#define NET_DVR_ERR_GENERAL_PU_REFUSED                                  3714
#define NET_DVR_ERR_GENERAL_PU_NO_RESOURCE                              3715
#define NET_DVR_ERR_GENERAL_PU_CHANNEL_ERROR                            3716
#define NET_DVR_ERR_GENERAL_SYSTEM_COMMAND_PU_COMMAND_UNSUPPORTED       3717
#define NET_DVR_ERR_GENERAL_SYSTEM_COMMAND_PU_NO_RIGHTS_TO_DO_COMMAND   3718
#define NET_DVR_ERR_GENERAL_NO_SESSION_FOUND                            3719
#define NET_DVR_ERR_GENERAL_PU_NO_VALID_PRELINK                         3720
#define NET_DVR_ERR_GENERAL_PU_NO_INNER_RESOURCE                        3721
#define NET_DVR_ERR_GENERAL_PU_NO_P2P_RESOURCE                          3722
#define NET_DVR_ERR_GENERAL_PU_NO_UESR                                  3723
#define NET_DVR_ERR_GENERAL_TICKET_EXPIRED                              3724
#define NET_DVR_ERR_GENERAL_TICKET_INVALID                              3725
#define NET_DVR_ERR_GENERAL_NO_P2PSERVER_RESOURCE                       3726
#define NET_DVR_ERR_GENERAL_PU_NOT_FOUND                                3727
#define NET_DVR_ERR_GENERAL_SESSION_FREED                               3728
#define NET_DVR_ERR_RECORD_SEARCH_START_TIME_ERROR                      3729
#define NET_DVR_ERR_RECORD_SEARCH_STOP_TIME_ERROR                       3730
#define NET_DVR_ERR_RECORD_SEARCH_FAIL                                  3731
#define NET_DVR_ERR_RECORD_NO_RESOURCE                                  3732
#define NET_DVR_ERR_CAPTURE_PIC_LOCAL_FAILED                            3733
#define NET_DVR_ERR_CAPTURE_PIC_APPLY_CACHE_FAILED                      3734
#define NET_DVR_ERR_CAPTURE_PIC_PARSE_PMS_DOMAIN_FAILED                 3735
#define NET_DVR_ERR_CAPTURE_PIC_CONNECT_PMS_FAILED                      3736
#define NET_DVR_ERR_CAPTURE_PIC_CREATE_PMS_PACKET_FAILED                3737
#define NET_DVR_ERR_CAPTURE_PIC_SEND_PMS_FAILED                         3738
#define NET_DVR_ERR_CAPTURE_PIC_RECV_PMS_FAILED                         3739
#define NET_DVR_ERR_CAPTURE_PIC_PARSE_PMS_RESPONSE_FAILED               3740
#define NET_DVR_ERR_CAPTURE_PIC_GET_URL_FAILED                          3741

#define NET_DVR_DATABASE_OPEN_ERROR             8001
#define NET_DVR_DATABASE_INSERT_ERROR           8002
#define NET_DVR_DATABASE_TABLE_ERROR            8003

//Sleep wake error code segment   8301~8330  
#define NET_ERR_DEV_SLEEP                       8301    //Device is sleep
#define NET_ERR_DEV_GOINGTOSLEEP                8302    //Device is going to sleep


//-------------ErrorCode Define(HCISUPSDK Only)End--------

typedef struct tagNET_EHOME_IPADDRESS
{
    char szIP[128];
    WORD wPort;     //port
    char byRes[2];
}NET_EHOME_IPADDRESS, *LPNET_EHOME_IPADDRESS;

typedef struct tagNET_EHOME_ZONE
{
    DWORD dwX;        //X
    DWORD dwY;        //Y
    DWORD dwWidth;  //Width
    DWORD dwHeight;    //Height
}NET_EHOME_ZONE, *LPNET_EHOME_ZONE;

//local config
typedef enum tagNET_EHOME_LOCAL_CFG_TYPE
{
    UNDEFINE = -1,
    ACTIVE_ACCESS_SECURITY = 0,
    AMS_ADDRESS = 1,
    SEND_PARAM = 2,
    SET_REREGISTER_MODE = 3,
    LOCAL_CFG_TYPE_GENERAL = 4,
    COM_PATH = 5,
    SESSIONKEY_REQ_MOD = 6,
    DEV_DAS_PINGREO_CALLBACK = 7,
    REGISTER_LISTEN_MODE = 8,
    STREAM_PLAYBACK_PARAM = 9,
    RAPID_WAKEUP_MOD = 10 
}NET_EHOME_LOCAL_CFG_TYPE, *LPNET_EHOME_LOCAL_CFG_TYPE;

typedef struct tagNET_EHOME_LOCAL_ACCESS_SECURITY
{
    DWORD   dwSize;
    BYTE    byAccessSecurity;
    BYTE    byRes[127];
}NET_EHOME_LOCAL_ACCESS_SECURITY, *LPNET_EHOME_LOCAL_ACCESS_SECURITY;

typedef struct tagNET_EHOME_AMS_ADDRESS
{
    DWORD dwSize;
    BYTE  byEnable;  //0-disable CMS receive alarm,1-enable CMS receive alarm
    BYTE  byRes1[3];
    NET_EHOME_IPADDRESS  struAddress;    //AMS loop back address
    BYTE byRes2[32];
}NET_EHOME_AMS_ADDRESS, *LPNET_EHOME_AMS_ADDRESS;

typedef struct tagNET_EHOME_SEND_PARAM
{
    DWORD dwSize;
    DWORD dwRecvTimeOut;    //recv timeout
    BYTE  bySendTimes;      //send times
    BYTE  byRes2[127];
}NET_EHOME_SEND_PARAM, *LPNET_EHOME_SEND_PARAM;

typedef struct tagNET_EHOME_DEV_SESSIONKEY
{
    BYTE   sDeviceID[MAX_DEVICE_ID_LEN];    //device ID/*256*/
    BYTE   sSessionKey[MAX_MASTER_KEY_LEN]; //Sessionkey/*16*/
} NET_EHOME_DEV_SESSIONKEY, *LPNET_EHOME_DEV_SESSIONKEY;


typedef struct tagNET_EHOME_LOCAL_GENERAL_CFG
{
    BYTE byAlarmPictureSeparate;
    BYTE byRes[127];
}NET_EHOME_LOCAL_GENERAL_CFG, *LPNET_EHOME_LOCAL_GENERAL_CFG;

typedef struct tagNET_EHOME_LOCAL_DEV_PINGREO
{
    DWORD   dwSize;
    BYTE    byEnablePingReoCallback;
    BYTE    byRes[63];
}NET_EHOME_LOCAL_DEV_PINGREO, *LPNET_EHOME_LOCAL_DEV_PINGREO;

#endif //_HC_ISUP_PUBLIC_H_
