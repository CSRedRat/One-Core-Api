/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"
#include <assert.h>
#include <rtl.h>
#include <ntsecapi.h>
#include <cmfuncs.h>
#include "localelcid.h"
#include <wine/debug.h>
#include <rtlavl.h>
#include <port.h>

/* Use intrinsics for x86 and x64 */
#if defined(_M_IX86) || defined(_M_AMD64)
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#endif
#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))

#define MAX_HW_COUNTERS 16

/* Definitions *****************************************/

typedef PVOID* PALPC_HANDLE;

typedef PVOID ALPC_HANDLE;

typedef DWORD WINAPI RTL_RUN_ONCE_INIT_FN(PRTL_RUN_ONCE, PVOID, PVOID*);

typedef RTL_RUN_ONCE_INIT_FN *PRTL_RUN_ONCE_INIT_FN;

/* SHA1 Helper Macros */

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
/* FIXME: This definition of DWORD2BE is little endian specific! */
#define DWORD2BE(x) (((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | (((x) << 24) & 0xff000000);
/* FIXME: This definition of blk0 is little endian specific! */
#define blk0(i) (Block[i] = (rol(Block[i],24)&0xFF00FF00)|(rol(Block[i],8)&0x00FF00FF))
#define blk1(i) (Block[i&15] = rol(Block[(i+13)&15]^Block[(i+8)&15]^Block[(i+2)&15]^Block[i&15],1))
#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)
/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

/* Enumarations ****************************************/

/* Structs *********************************************/

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

/* Internal header for table entries */
typedef struct _TABLE_ENTRY_HEADER
{
	RTL_SPLAY_LINKS SplayLinks;
    RTL_BALANCED_LINKS BalancedLinks;
	LIST_ENTRY ListEntry;
    LONGLONG UserData;
} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

typedef enum _RTL_AVL_BALANCE_FACTOR
{
    RtlUnbalancedAvlTree = -2,
    RtlLeftHeavyAvlTree,
    RtlBalancedAvlTree,
    RtlRightHeavyAvlTree,
} RTL_AVL_BALANCE_FACTOR;

typedef struct _RTLP_SRWLOCK_SHARED_WAKE
{
    LONG Wake;
    volatile struct _RTLP_SRWLOCK_SHARED_WAKE *Next;
} volatile RTLP_SRWLOCK_SHARED_WAKE, *PRTLP_SRWLOCK_SHARED_WAKE;

typedef ULONG LOGICAL;

typedef DWORD TP_VERSION, *PTP_VERSION;

typedef DWORD TP_WAIT_RESULT;

typedef PVOID* PALPC_HANDLE;

typedef PVOID ALPC_HANDLE;

typedef HANDLE 	DLL_DIRECTORY_COOKIE;

typedef struct _TP_POOL TP_POOL, *PTP_POOL;

typedef struct _TP_TIMER TP_TIMER, *PTP_TIMER;

typedef struct _TP_CLEANUP_GROUP TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;

typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;

typedef VOID (*PTP_TIMER_CALLBACK)(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);

typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;

typedef TP_CALLBACK_ENVIRON_V1 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;

typedef VOID (*PTP_WAIT_CALLBACK)(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT WaitResult);

typedef VOID (WINAPI *PBAD_MEMORY_CALLBACK_ROUTINE)(VOID);

typedef BOOLEAN (*PSECURE_MEMORY_CACHE_CALLBACK)(
    _In_  PVOID Addr,
    _In_  SIZE_T Range
);

typedef PVOID PDELAYLOAD_FAILURE_SYSTEM_ROUTINE; 

typedef struct _TP_IO{
	void * 	dummy;
}TP_IO,*PTP_IO;

typedef enum _NORM_FORM { 
  NormalizationOther  = 0,
  NormalizationC      = 0x1,
  NormalizationD      = 0x2,
  NormalizationKC     = 0x5,
  NormalizationKD     = 0x6
} NORM_FORM;

typedef struct _WIN32_MEMORY_RANGE_ENTRY {
  PVOID  VirtualAddress;
  SIZE_T NumberOfBytes;
} WIN32_MEMORY_RANGE_ENTRY, *PWIN32_MEMORY_RANGE_ENTRY;

typedef struct _CLAIM_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE {
  PVOID pValue;
  DWORD ValueLength;
} CLAIM_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE, *PCLAIM_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE;

typedef struct _CLAIM_SECURITY_ATTRIBUTE_FQBN_VALUE {
  DWORD64 Version;
  PWSTR   Name;
} CLAIM_SECURITY_ATTRIBUTE_FQBN_VALUE, *PCLAIM_SECURITY_ATTRIBUTE_FQBN_VALUE;

typedef struct _CLAIM_SECURITY_ATTRIBUTE_V1 {
  PWSTR Name;
  WORD  ValueType;
  WORD  Reserved;
  DWORD Flags;
  DWORD ValueCount;
  union {
    PLONG64                                      pInt64;
    PDWORD64                                     pUint64;
    PWSTR                                        *ppString;
    PCLAIM_SECURITY_ATTRIBUTE_FQBN_VALUE         pFqbn;
    PCLAIM_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE pOctetString;
  } Values;
} CLAIM_SECURITY_ATTRIBUTE_V1, *PCLAIM_SECURITY_ATTRIBUTE_V1;

typedef struct _CLAIM_SECURITY_ATTRIBUTES_INFORMATION {
  WORD  Version;
  WORD  Reserved;
  DWORD AttributeCount;
  union {
    PCLAIM_SECURITY_ATTRIBUTE_V1 pAttributeV1;
  } Attribute;
} CLAIM_SECURITY_ATTRIBUTES_INFORMATION, *PCLAIM_SECURITY_ATTRIBUTES_INFORMATION;

typedef struct _TP_POOL_STACK_INFORMATION
{
   SIZE_T StackReserve;
   SIZE_T StackCommit;
} TP_POOL_STACK_INFORMATION,*PTP_POOL_STACK_INFORMATION;

typedef enum  { 
  PMCCounter,
  MaxHardwareCounterType
} HARDWARE_COUNTER_TYPE;

typedef struct _HARDWARE_COUNTER_DATA {
  HARDWARE_COUNTER_TYPE Type;
  DWORD                 Reserved;
  DWORD64               Value;
} HARDWARE_COUNTER_DATA, *PHARDWARE_COUNTER_DATA;

typedef struct _PERFORMANCE_DATA {
  WORD                  Size;
  BYTE                  Version;
  BYTE                  HwCountersCount;
  DWORD                 ContextSwitchCount;
  DWORD64               WaitReasonBitMap;
  DWORD64               CycleTime;
  DWORD                 RetryCount;
  DWORD                 Reserved;
  HARDWARE_COUNTER_DATA HwCounters[MAX_HW_COUNTERS];
} PERFORMANCE_DATA, *PPERFORMANCE_DATA;

typedef struct _ALPC_PORT_ATTRIBUTES
{
  ULONG Flags;
  SECURITY_QUALITY_OF_SERVICE SecurityQos;
  SIZE_T MaxMessageLength;
  SIZE_T MemoryBandwidth;
  SIZE_T MaxPoolUsage;
  SIZE_T MaxSectionSize;
  SIZE_T MaxViewSize;
  SIZE_T MaxTotalSectionSize;
  ULONG DupObjectTypes;
 #ifdef _M_X64
  ULONG Reserved;
 #endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

typedef struct _ALPC_DATA_VIEW_ATTR
{
  ULONG Flags;
  ALPC_HANDLE SectionHandle;
  PVOID ViewBase; // must be zero on input
  SIZE_T ViewSize;
} ALPC_DATA_VIEW_ATTR, *PALPC_DATA_VIEW_ATTR;

typedef enum _ALPC_PORT_INFORMATION_CLASS
{
  AlpcBasicInformation, // q: out ALPC_BASIC_INFORMATION
  AlpcPortInformation, // s: in ALPC_PORT_ATTRIBUTES
  AlpcAssociateCompletionPortInformation, // s: in ALPC_PORT_ASSOCIATE_COMPLETION_PORT
  AlpcConnectedSIDInformation, // q: in SID
  AlpcServerInformation, // q: inout ALPC_SERVER_INFORMATION
  AlpcMessageZoneInformation, // s: in ALPC_PORT_MESSAGE_ZONE_INFORMATION
  AlpcRegisterCompletionListInformation, // s: in ALPC_PORT_COMPLETION_LIST_INFORMATION
  AlpcUnregisterCompletionListInformation, // s: VOID
  AlpcAdjustCompletionListConcurrencyCountInformation, // s: in ULONG
  AlpcRegisterCallbackInformation, // kernel-mode only
  AlpcCompletionListRundownInformation, // s: VOID
  MaxAlpcPortInfoClass
} ALPC_PORT_INFORMATION_CLASS;

typedef struct _ALPC_SECURITY_ATTR
{
  ULONG Flags;
  PSECURITY_QUALITY_OF_SERVICE SecurityQos;
  ALPC_HANDLE ContextHandle; // dbg
  ULONG Reserved1;
  ULONG Reserved2;
} ALPC_SECURITY_ATTR, *PALPC_SECURITY_ATTR;

typedef struct _ALPC_MESSAGE_ATTRIBUTES
{
  ULONG AllocatedAttributes;
  ULONG ValidAttributes;
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES;

typedef struct _ALPC_CONTEXT_ATTR
{
  PVOID PortContext;
  PVOID MessageContext;
  ULONG Sequence;
  ULONG MessageId;
  ULONG CallbackId;
} ALPC_CONTEXT_ATTR, *PALPC_CONTEXT_ATTR;

typedef struct _DELAYLOAD_PROC_DESCRIPTOR
{
     ULONG ImportDescribedByName;
     union {
         LPCSTR Name;
         ULONG Ordinal;
     } Description;
} DELAYLOAD_PROC_DESCRIPTOR, *PDELAYLOAD_PROC_DESCRIPTOR;

typedef struct _DELAYLOAD_INFO
{
     ULONG Size;
     PCIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor;
     PIMAGE_THUNK_DATA ThunkAddress;
     LPCSTR TargetDllName;
     DELAYLOAD_PROC_DESCRIPTOR TargetApiDescriptor;
     PVOID TargetModuleBase;
     PVOID Unused;
     ULONG LastError;
} DELAYLOAD_INFO, *PDELAYLOAD_INFO;
typedef PVOID (WINAPI *PDELAYLOAD_FAILURE_DLL_CALLBACK)(ULONG, PDELAYLOAD_INFO);

typedef struct _RTLP_SRWLOCK_WAITBLOCK
{
    /* SharedCount is the number of shared acquirers. */
    LONG SharedCount;

    /* Last points to the last wait block in the chain. The value
       is only valid when read from the first wait block. */
    volatile struct _RTLP_SRWLOCK_WAITBLOCK *Last;

    /* Next points to the next wait block in the chain. */
    volatile struct _RTLP_SRWLOCK_WAITBLOCK *Next;

    union
    {
        /* Wake is only valid for exclusive wait blocks */
        LONG Wake;
        /* The wake chain is only valid for shared wait blocks */
        struct
        {
            PRTLP_SRWLOCK_SHARED_WAKE SharedWakeChain;
            PRTLP_SRWLOCK_SHARED_WAKE LastSharedWake;
        };
    };

    BOOLEAN Exclusive;
} volatile RTLP_SRWLOCK_WAITBLOCK, *PRTLP_SRWLOCK_WAITBLOCK;

static void SHA1Transform(ULONG State[5], UCHAR Buffer[64])
{
   ULONG a, b, c, d, e;
   ULONG *Block;

   Block = (ULONG*)Buffer;

   /* Copy Context->State[] to working variables */
   a = State[0];
   b = State[1];
   c = State[2];
   d = State[3];
   e = State[4];

   /* 4 rounds of 20 operations each. Loop unrolled. */
   R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
   R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
   R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
   R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
   R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
   R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
   R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
   R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
   R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
   R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
   R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
   R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
   R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
   R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
   R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
   R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
   R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
   R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
   R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
   R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

   /* Add the working variables back into Context->State[] */
   State[0] += a;
   State[1] += b;
   State[2] += c;
   State[3] += d;
   State[4] += e;

   /* Wipe variables */
   a = b = c = d = e = 0;
}

/* Functions Prototypes for public*/

ULONG
NTAPI
RtlIsDosDeviceName_Ustr(
    _In_ PCUNICODE_STRING Name
);

PVOID 
WINAPI 
RtlEncodePointer(PVOID ptr);

PVOID 
WINAPI 
RtlDecodePointer(PVOID ptr);

PVOID 
WINAPI 
RtlEncodeSystemPointer(PVOID ptr);

PVOID 
WINAPI 
RtlDecodeSystemPointer(PVOID ptr);

void 
WINAPI 
A_SHAInit(PSHA_CTX Context);

VOID 
WINAPI 
A_SHAUpdate(PSHA_CTX Context,
	const unsigned char *  	Buffer,
	UINT  	BufferSize 
);  

VOID 
WINAPI 
A_SHAFinal(
	PSHA_CTX Context, 
	PULONG Result) ;