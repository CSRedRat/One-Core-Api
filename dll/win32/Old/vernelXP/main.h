#define WIN32_NO_STATUS

/* INCLUDES **********************************************************/
#include <k32.h>
#include <kefuncs.h>
#include <exfuncs.h>
#include <strsafe.h>
#include <iofuncs.h>
#include <mmfuncs.h>
#include <ntdllnew.h>
#include <base.h>
#include <pofuncs.h>
#include <cmfuncs.h>
#include <kernelfuncs.h>
#include <basemsg.h>
#include <strsafe.h>
#include <windef.h>
#include <cmfuncs.h>
#include <iofuncs.h>
#include <pseh2.h>
#include <config.h>
#include <port.h>
#include <ntdllbase.h>

/* DEFINITIONS **********************************************************/

#define CSTR_LESS_THAN 1
#define CSTR_GREATER_THAN 3
#define SYMBOLIC_LINK_FLAG_DIRECTORY  0x1
#define INIT_ONCE_STATIC_INIT {0}
#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#define VOLUME_NAME_DOS 0x0
#define VOLUME_NAME_GUID 0x1
#define VOLUME_NAME_NONE 0x4
#define VOLUME_NAME_NT 0x2
#define FILE_NAME_NORMALIZED 0x0
#define FILE_NAME_OPENED 0x8
#define get_char_typeW(x) iswctype((x) >> 8, (x) & 0xFF)
#define FileInformationReserved33 33
#define FileInformationReserved32 32
#define GET_HISTORY_INFO (0x46)
#define CSR_CONSOLE    0x0001
#define CONDITION_VARIABLE_INIT RTL_CONDITION_VARIABLE_INIT
#define CONDITION_VARIABLE_LOCKMODE_SHARED RTL_CONDITION_VARIABLE_LOCKMODE_SHARED
#define DIV 1024
#define SystemDeleteSession 48
#define KEY_LENGTH 1024
#define BINARY_TYPE_EXE   0x01
#define BINARY_TYPE_COM   0x02
#define BINARY_TYPE_PIF   0x03
#define BINARY_TYPE_WOW   0x40
#define SystemProcessorTimes 8
#define SystemInstructionEmulationCounts 19 
#define SystemLoadAndCallImage 38 
#define SystemLoadImage 26
#define MUI_LANGUAGE_ID 1
#define MUI_LANGUAGE_NAME 2
#define IDN_ALLOW_UNASSIGNED        0x1
#define IDN_USE_STD3_ASCII_RULES    0x2
#define FIND_DATA_SIZE      0x4000

/* TYPE DEFINITIONS **********************************************************/

typedef UNICODE_STRING LSA_UNICODE_STRING;
typedef PVOID LPPROC_THREAD_ATTRIBUTE_LIST;	 
typedef DWORD WINAPI RTL_RUN_ONCE_INIT_FN(PRTL_RUN_ONCE, PVOID, PVOID*);
typedef RTL_RUN_ONCE_INIT_FN *PRTL_RUN_ONCE_INIT_FN;
typedef RTL_RUN_ONCE  INIT_ONCE;
typedef PRTL_RUN_ONCE PINIT_ONCE;
typedef PRTL_RUN_ONCE LPINIT_ONCE;
typedef BOOL (WINAPI *PINIT_ONCE_FN)(PINIT_ONCE,PVOID,PVOID*);
typedef DWORD (WINAPI *APPLICATION_RECOVERY_CALLBACK)(PVOID);
typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;
typedef NTSTATUS(NTAPI * PBASEP_APPCERT_PLUGIN_FUNC)(IN LPWSTR ApplicationName, IN ULONG CertFlag);
typedef NTSTATUS(NTAPI *PBASEP_APPCERT_EMBEDDED_FUNC)(IN LPWSTR ApplicationName);
typedef UINT MMRESULT;

/* GLOBAL VARIABLES **********************************************************/

extern int get_decomposition(WCHAR src, WCHAR *dst, unsigned int dstlen);

extern const unsigned int collation_table[];

PBASE_STATIC_SERVER_DATA BaseStaticServerData;

LIST_ENTRY BasepAppCertDllsList;

static PINIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;

PINIT_ONCE_FN TimeInit;

PRTL_SRWLOCK lock;

volatile long TzSpecificCache;

static ULONG BaseDllTag = 0;

static ULONG globalPeriod = 0;

static ULONG timerData = 0;

static ULONG MinimumTime = 0;

static ULONG otherTime = 0;

static ULONG TDD_MAXRESOLUTION = 0; 

static __int16 vector[30];

static PRTL_SRWLOCK BaseSearchPathModeLock;

static int BaseSearchPathMode  = 0; 

static ULONG KernelBaseGlobalData = 0;

static BOOL BasepAllowResourceConversion = TRUE;

static PCEVENT_DESCRIPTOR UACUnhandledErrorElevationRequired = NULL;

static LPCGUID MS_Windows_UAC_Provider = 0;

/* STRUCTURES AND ENUMERATIONS **************************************************/

typedef enum _COPYFILE2_MESSAGE_TYPE { 
  COPYFILE2_CALLBACK_NONE             = 0,
  COPYFILE2_CALLBACK_CHUNK_STARTED    = 1,
  COPYFILE2_CALLBACK_CHUNK_FINISHED   = 2,
  COPYFILE2_CALLBACK_STREAM_STARTED   = 3,
  COPYFILE2_CALLBACK_STREAM_FINISHED  = 4,
  COPYFILE2_CALLBACK_POLL_CONTINUE    = 5,
  COPYFILE2_CALLBACK_ERROR            = 6,
  COPYFILE2_CALLBACK_MAX
} COPYFILE2_MESSAGE_TYPE;

typedef enum _DEP_SYSTEM_POLICY_TYPE
{
	OptOut = 1
}DEP_SYSTEM_POLICY_TYPE, *PDEP_SYSTEM_POLICY_TYPE;

typedef struct _OVERLAPPED_ENTRY {
  ULONG_PTR    lpCompletionKey;
  LPOVERLAPPED lpOverlapped;
  ULONG_PTR    Internal;
  DWORD        dwNumberOfBytesTransferred;
} OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;

typedef struct timecaps_tag {
  UINT wPeriodMin;
  UINT wPeriodMax;
} TIMECAPS, *PTIMECAPS, *NPTIMECAPS, *LPTIMECAPS;

typedef struct mmtime_tag {
  UINT  wType;
  union {
    DWORD  ms;
    DWORD  sample;
    DWORD  cb;
    DWORD  ticks;
    struct {
      BYTE hour;
      BYTE min;
      BYTE sec;
      BYTE frame;
      BYTE fps;
      BYTE dummy;
      BYTE pad[2];
    } smpte;
    struct {
      DWORD songptrpos;
    } midi;
  } u;
} MMTIME, *PMMTIME, *LPMMTIME;
 
typedef struct _CREATEFILE2_EXTENDED_PARAMETERS {
  DWORD                 dwSize;
  DWORD                 dwFileAttributes;
  DWORD                 dwFileFlags;
  DWORD                 dwSecurityQosFlags;
  LPSECURITY_ATTRIBUTES lpSecurityAttributes;
  HANDLE                hTemplateFile;
} CREATEFILE2_EXTENDED_PARAMETERS, *PCREATEFILE2_EXTENDED_PARAMETERS, *LPCREATEFILE2_EXTENDED_PARAMETERS;

typedef enum _PROCESS_MITIGATION_POLICY { 
  ProcessDEPPolicy                  = 0,
  ProcessASLRPolicy                 = 1,
  ProcessReserved1MitigationPolicy  = 2,
  ProcessStrictHandleCheckPolicy    = 3,
  ProcessSystemCallDisablePolicy    = 4,
  MaxProcessMitigationPolicy        = 5
} PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;

typedef enum _VDM_ENTRY_CODE
{
     VdmEntryUndo,
     VdmEntryUpdateProcess,
     VdmEntryUpdateControlCHandler
} VDM_ENTRY_CODE;

typedef enum _COPYFILE2_COPY_PHASE { 
  COPYFILE2_PHASE_NONE               = 0,
  COPYFILE2_PHASE_PREPARE_SOURCE     = 1,
  COPYFILE2_PHASE_PREPARE_DEST       = 2,
  COPYFILE2_PHASE_READ_SOURCE        = 3,
  COPYFILE2_PHASE_WRITE_DESTINATION  = 4,
  COPYFILE2_PHASE_SERVER_COPY        = 5,
  COPYFILE2_PHASE_NAMEGRAFT_COPY     = 6,
  COPYFILE2_PHASE_MAX
} COPYFILE2_COPY_PHASE;

typedef struct _BASEP_APPCERT_ENTRY
{
     LIST_ENTRY Entry;
     UNICODE_STRING Name;
     PBASEP_APPCERT_PLUGIN_FUNC fPluginCertFunc;
} BASEP_APPCERT_ENTRY, *PBASEP_APPCERT_ENTRY;

typedef struct COPYFILE2_MESSAGE {
  COPYFILE2_MESSAGE_TYPE Type;
  DWORD                  dwPadding;
  union {
    struct {
      DWORD          dwStreamNumber;
      DWORD          dwReserved;
      HANDLE         hSourceFile;
      HANDLE         hDestinationFile;
      ULARGE_INTEGER uliChunkNumber;
      ULARGE_INTEGER uliChunkSize;
      ULARGE_INTEGER uliStreamSize;
      ULARGE_INTEGER uliTotalFileSize;
    } ChunkStarted;
    struct {
      DWORD          dwStreamNumber;
      DWORD          dwReserved;
      HANDLE         hSourceFile;
      HANDLE         hDestinationFile;
      ULARGE_INTEGER uliChunkNumber;
      ULARGE_INTEGER uliChunkSize;
      ULARGE_INTEGER uliStreamSize;
      ULARGE_INTEGER uliStreamBytesTransferred;
      ULARGE_INTEGER uliTotalFileSize;
      ULARGE_INTEGER uliTotalBytesTransferred;
    } ChunkFinished;
    struct {
      DWORD          dwStreamNumber;
      DWORD          dwReserved;
      HANDLE         hSourceFile;
      HANDLE         hDestinationFile;
      ULARGE_INTEGER uliStreamSize;
      ULARGE_INTEGER uliTotalFileSize;
    } StreamStarted;
    struct {
      DWORD          dwStreamNumber;
      DWORD          dwReserved;
      HANDLE         hSourceFile;
      HANDLE         hDestinationFile;
      ULARGE_INTEGER uliStreamSize;
      ULARGE_INTEGER uliStreamBytesTransferred;
      ULARGE_INTEGER uliTotalFileSize;
      ULARGE_INTEGER uliTotalBytesTransferred;
    } StreamFinished;
    struct {
      DWORD dwReserved;
    } PollContinue;
    struct {
      COPYFILE2_COPY_PHASE CopyPhase;
      DWORD                dwStreamNumber;
      HRESULT              hrFailure;
      DWORD                dwReserved;
      ULARGE_INTEGER       uliChunkNumber;
      ULARGE_INTEGER       uliStreamSize;
      ULARGE_INTEGER       uliStreamBytesTransferred;
      ULARGE_INTEGER       uliTotalFileSize;
      ULARGE_INTEGER       uliTotalBytesTransferred;
    } Error;
  } Info;
} COPYFILE2_MESSAGE;

typedef struct _nlsversioninfoex {
  DWORD dwNLSVersionInfoSize;
  DWORD dwNLSVersion;
  DWORD dwDefinedVersion;
  DWORD dwEffectiveId;
  GUID  guidCustomVersion;
} NLSVERSIONINFOEX, *LPNLSVERSIONINFOEX;

typedef struct _SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION {
   DWORD64 CycleTime;
} SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION,*PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION;

typedef struct _BASEP_ACTCTX_BLOCK
{
     ULONG Flags;
     PVOID ActivationContext;
     PVOID CompletionContext;
     LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
} BASEP_ACTCTX_BLOCK, *PBASEP_ACTCTX_BLOCK;

typedef enum _COPYFILE2_MESSAGE_ACTION { 
  COPYFILE2_PROGRESS_CONTINUE  = 0,
  COPYFILE2_PROGRESS_CANCEL    = 1,
  COPYFILE2_PROGRESS_STOP      = 2,
  COPYFILE2_PROGRESS_QUIET     = 3,
  COPYFILE2_PROGRESS_PAUSE     = 4
} COPYFILE2_MESSAGE_ACTION;

typedef enum _FIRMWARE_TYPE { 
  FirmwareTypeUnknown  = 0,
  FirmwareTypeBios     = 1,
  FirmwareTypeUefi     = 2,
  FirmwareTypeMax      = 3
} FIRMWARE_TYPE, *PFIRMWARE_TYPE;

typedef COPYFILE2_MESSAGE_ACTION (*PCOPYFILE2_PROGRESS_ROUTINE)(
    _In_      const COPYFILE2_MESSAGE *pMessage,
    _In_opt_  PVOID pvCallbackContext
);

typedef struct COPYFILE2_EXTENDED_PARAMETERS {
  DWORD                       dwSize;
  DWORD                       dwCopyFlags;
  BOOL                        *pfCancel;
  PCOPYFILE2_PROGRESS_ROUTINE pProgressRoutine;
  PVOID                       pvCallbackContext;
} COPYFILE2_EXTENDED_PARAMETERS;

typedef struct {
  UINT  cbSize;
  UINT  HistoryBufferSize;
  UINT  NumberOfHistoryBuffers;
  DWORD dwFlags;
} CONSOLE_HISTORY_INFO, *PCONSOLE_HISTORY_INFO;

typedef struct _REASON_CONTEXT {
  ULONG Version;
  DWORD Flags;
  union {
    struct {
      HMODULE LocalizedReasonModule;
      ULONG   LocalizedReasonId;
      ULONG   ReasonStringCount;
      LPWSTR  *ReasonStrings;
    } Detailed;
    LPWSTR SimpleReasonString;
  } Reason;
} REASON_CONTEXT, *PREASON_CONTEXT;

typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  _ANONYMOUS_UNION union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

typedef struct _CONSOLE_FONT_INFOEX {
  ULONG cbSize;
  DWORD nFont;
  COORD dwFontSize;
  UINT  FontFamily;
  UINT  FontWeight;
  WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;

typedef struct _SCOPETABLE_ENTRY {
    int EnclosingLevel;                 /* lexical level of enclosing scope */
    int (__cdecl *Filter)(void);        /* NULL for a termination handler */
    void (__cdecl *SpecificHandler)(void); /* xcpt or termination handler */
}SCOPETABLE_ENTRY,*PSCOPETABLE_ENTRY;

typedef struct _EH3_EXCEPTION_REGISTRATION {
    struct _EH3_EXCEPTION_REGISTRATION *Next;
    PVOID ExceptionHandler;
    PSCOPETABLE_ENTRY ScopeTable;
    DWORD TryLevel;
} EH3_EXCEPTION_REGISTRATION, *PEH3_EXCEPTION_REGISTRATION;

typedef struct _locale{
	LPCWSTR description;
	LPCWSTR code;
	LCID lcidHex;
	LCID lcidDec;	
}locale;

typedef struct _CPPEH_RECORD{
	struct _EH3_EXCEPTION_REGISTRATION registration;
}CPPEH_RECORD, *PCPPEH_RECORD;

typedef enum _FILE_INFO_BY_HANDLE_CLASS { 
  FileBasicInfo                   = 0,
  FileStandardInfo                = 1,
  FileNameInfo                    = 2,
  FileRenameInfo                  = 3,
  FileDispositionInfo             = 4,
  FileAllocationInfo              = 5,
  FileEndOfFileInfo               = 6,
  FileStreamInfo                  = 7,
  FileCompressionInfo             = 8,
  FileAttributeTagInfo            = 9,
  FileIdBothDirectoryInfo         = 10, // 0xA
  FileIdBothDirectoryRestartInfo  = 11, // 0xB
  FileIoPriorityHintInfo          = 12, // 0xC
  FileRemoteProtocolInfo          = 13, // 0xD
  FileFullDirectoryInfo           = 14, // 0xE
  FileFullDirectoryRestartInfo    = 15, // 0xF
  FileStorageInfo                 = 16, // 0x10
  FileAlignmentInfo               = 17, // 0x11
  FileIdInfo                      = 18, // 0x12
  FileIdExtdDirectoryInfo         = 19, // 0x13
  FileIdExtdDirectoryRestartInfo  = 20, // 0x14
  MaximumFileInfoByHandlesClass
} FILE_INFO_BY_HANDLE_CLASS, *PFILE_INFO_BY_HANDLE_CLASS;

typedef enum _POWER_REQUEST_TYPE { 
  PowerRequestDisplayRequired,
  PowerRequestSystemRequired,
  PowerRequestAwayModeRequired,
  PowerRequestExecutionRequired
} POWER_REQUEST_TYPE, *PPOWER_REQUEST_TYPE;

typedef enum _FILE_ID_TYPE { 
  FileIdType          = 0,
  ObjectIdType        = 1,
  ExtendedFileIdType  = 2,
  MaximumFileIdType
} FILE_ID_TYPE, *PFILE_ID_TYPE;

typedef enum _PIPE_ATTRIBUTE_TYPE {
  PipeAttribute = 0,
  PipeConnectionAttribute = 1,
  PipeHandleAttribute = 2
}PIPE_ATTRIBUTE_TYPE, *PPIPE_ATTRIBUTE_TYPE;

typedef struct {
  DWORD        dwSize;
  FILE_ID_TYPE Type;
  _ANONYMOUS_UNION union {
    LARGE_INTEGER FileId;
    GUID          ObjectId;
  } DUMMYUNIONNAME;
} FILE_ID_DESCRIPTOR;

typedef struct _CONSOLE_SCREEN_BUFFER_INFOEX {
  ULONG      cbSize;
  COORD      dwSize;
  COORD      dwCursorPosition;
  WORD       wAttributes;
  SMALL_RECT srWindow;
  COORD      dwMaximumWindowSize;
  WORD       wPopupAttributes;
  BOOL       bFullscreenSupported;
  COLORREF   ColorTable[16];
} CONSOLE_SCREEN_BUFFER_INFOEX, *PCONSOLE_SCREEN_BUFFER_INFOEX;

typedef struct _PROCESSOR_RELATIONSHIP {
  BYTE           Flags;
  BYTE           Reserved[21];
  WORD           GroupCount;
  GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
} PROCESSOR_RELATIONSHIP, *PPROCESSOR_RELATIONSHIP;

typedef struct _NUMA_NODE_RELATIONSHIP {
  DWORD          NodeNumber;
  BYTE           Reserved[20];
  GROUP_AFFINITY GroupMask;
} NUMA_NODE_RELATIONSHIP, *PNUMA_NODE_RELATIONSHIP;

typedef struct _PROCESSOR_GROUP_INFO {
  BYTE      MaximumProcessorCount;
  BYTE      ActiveProcessorCount;
  BYTE      Reserved[38];
  KAFFINITY ActiveProcessorMask;
} PROCESSOR_GROUP_INFO, *PPROCESSOR_GROUP_INFO;

typedef struct _GROUP_RELATIONSHIP {
  WORD                 MaximumGroupCount;
  WORD                 ActiveGroupCount;
  BYTE                 Reserved[20];
  PROCESSOR_GROUP_INFO GroupInfo[ANYSIZE_ARRAY];
} GROUP_RELATIONSHIP, *PGROUP_RELATIONSHIP;

typedef struct _CACHE_RELATIONSHIP {
  BYTE                 Level;
  BYTE                 Associativity;
  WORD                 LineSize;
  DWORD                CacheSize;
  PROCESSOR_CACHE_TYPE Type;
  BYTE                 Reserved[20];
  GROUP_AFFINITY       GroupMask;
} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;

typedef enum _FIND_DATA_TYPE
{
    FindFile   = 1,
    FindStream = 2
} FIND_DATA_TYPE;

typedef union _DIR_INFORMATION
{
    PVOID DirInfo;
    PFILE_FULL_DIR_INFORMATION FullDirInfo;
    PFILE_BOTH_DIR_INFORMATION BothDirInfo;
} DIR_INFORMATION;

typedef struct _FIND_FILE_DATA
{
    HANDLE Handle;
    FINDEX_INFO_LEVELS InfoLevel;
    FINDEX_SEARCH_OPS SearchOp;

    /*
     * For handling STATUS_BUFFER_OVERFLOW errors emitted by
     * NtQueryDirectoryFile in the FildNextFile function.
     */
    BOOLEAN HasMoreData;

    /*
     * "Pointer" to the next file info structure in the buffer.
     * The type is defined by the 'InfoLevel' parameter.
     */
    DIR_INFORMATION NextDirInfo;

    BYTE Buffer[FIND_DATA_SIZE];
} FIND_FILE_DATA, *PFIND_FILE_DATA;

typedef struct _FIND_STREAM_DATA
{
    STREAM_INFO_LEVELS InfoLevel;
    PFILE_STREAM_INFORMATION FileStreamInfo;
    PFILE_STREAM_INFORMATION CurrentInfo;
} FIND_STREAM_DATA, *PFIND_STREAM_DATA;

typedef struct _FIND_DATA_HANDLE
{
    FIND_DATA_TYPE Type;
    RTL_CRITICAL_SECTION Lock;

    /*
     * Pointer to the following finding data, located at
     * (this + 1). The type is defined by the 'Type' parameter.
     */
    union
    {
        PFIND_FILE_DATA FindFileData;
        PFIND_STREAM_DATA FindStreamData;
    } u;

} FIND_DATA_HANDLE, *PFIND_DATA_HANDLE;

typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  DWORD                          Size;
  union {
    PROCESSOR_RELATIONSHIP Processor;
    NUMA_NODE_RELATIONSHIP NumaNode;
    CACHE_RELATIONSHIP     Cache;
    GROUP_RELATIONSHIP     Group;
  };
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

/* FUNCTION PROTOTYPES *******************************************************/

/*-------------------------------------INTERNAL FUNCTIONS -----------------------------------------*/

HANDLE 
WINAPI 
BaseGetNamedObjectDirectory(VOID);

POBJECT_ATTRIBUTES 
WINAPI 
BaseFormatObjectAttributes(OUT POBJECT_ATTRIBUTES ObjectAttributes,
						   IN PSECURITY_ATTRIBUTES SecurityAttributes OPTIONAL, 
						   IN PUNICODE_STRING ObjectName);

VOID 
WINAPI 
BaseSetLastNTError(IN NTSTATUS Status);

PLARGE_INTEGER 
WINAPI 
BaseFormatTimeOut
(OUT PLARGE_INTEGER Timeout,
 IN DWORD dwMilliseconds);

DWORD
WINAPI 
FilenameU2A_FitOrFail(
   LPSTR  DestA,
   INT destLen, /* buffer size in TCHARS incl. nullchar */
   PUNICODE_STRING SourceU
); 

NTSTATUS
NTAPI
BasepConfigureAppCertDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

PUNICODE_STRING
WINAPI
Basep8BitStringToStaticUnicodeString(IN LPCSTR String);

BOOLEAN 
WINAPI 
Basep8BitStringToDynamicUnicodeString(OUT PUNICODE_STRING UnicodeString, IN LPCSTR String) ;	

HANDLE 
WINAPI 
BaseGetNamedObjectDirectory(VOID); 

BOOL WINAPI WerpReportExceptionInProcessContext(DWORD parameter);

DWORD WINAPI BasePrepareReasonContext(REASON_CONTEXT *contex, PVOID address);

void WINAPI WerpNotifyLoadStringResource(HMODULE module, LPCWSTR string, HANDLE handle, DWORD flags);

ULONG WINAPI KernelBaseGetGlobalData();

extern locale LocaleList[3];

void WINAPI initTable();

int wine_get_sortkey(int flags, const WCHAR *src, int srclen, char *dst, int dstlen);
int wine_compare_string(int flags, const WCHAR *str1, int len1, const WCHAR *str2, int len2);
/*-------------------------------------TRANSACTIONS FUNCTIONS -----------------------------------------*/

BOOL WINAPI SetCurrentTransaction(HANDLE new_transaction);
HANDLE WINAPI GetCurrentTransaction();
BOOLEAN WINAPI CreateSymbolicLinkW(IN LPCWSTR lpSymlinkFileName, IN LPCWSTR lpTargetFileName, IN DWORD dwFlags);
DWORD WINAPI RtlRunOnceBeginInitialize( RTL_RUN_ONCE *once, ULONG flags, void **context );
DWORD WINAPI RtlRunOnceComplete( RTL_RUN_ONCE *once, ULONG flags, void *context );
DWORD WINAPI RtlRunOnceExecuteOnce( RTL_RUN_ONCE *once, PRTL_RUN_ONCE_INIT_FN func, void *param, void **context);
BOOL WINAPI GetNamedPipeAttribute(HANDLE Pipe, PIPE_ATTRIBUTE_TYPE AttributeType, PSTR AttributeName, PVOID AttributeValue, PSIZE_T AttributeValueLength);