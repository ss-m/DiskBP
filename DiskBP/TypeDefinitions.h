#pragma once

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif


#define POOLTAG 'PBsD'



// Actions
#define TRAP_TO_DBG		0x00000001
#define NOTIFY_UM		0x00000002
#define DENY_IO			0x00000004
#define BREAK_ON_READ	0x00010000
#define BREAK_ON_WRITE	0x00020000
#define ACTION_READ		0x10000000
#define ACTION_WRITE	0x20000000


typedef struct __BPInfo
{
	unsigned long Action;
	unsigned long long LocationBegin;
	unsigned long long LocationEnd;
}BPInfo , *PBPInfo;

typedef struct __BPListDataType
{
	PBPInfo BPList;
	unsigned long BPCount;
	unsigned long MaxBPCount;
}BPListDataType , *PBPListDataType;


typedef struct __UMNotificationMessage
{
	unsigned long Action;
	unsigned long Size;
	unsigned long long Location;
	int AttachedDeviceID;
	SINGLE_LIST_ENTRY SingleListEntry;
} UMNotificationMessage, * PUMNotificationMessage;


typedef struct __DeviceExtension
{
	PDEVICE_OBJECT TargetDevice;
	PDEVICE_OBJECT PhysicalDeviceObject;
	BPListDataType BPList;
	int DeviceID;
}DeviceExtension , *PDeviceExtension;



#define DISKBPCTL_ADD_BP				CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define DISKBPCTL_CLEAR_ALL_BP			CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_NEITHER,FILE_ANY_ACCESS)
#define DISKBPCTL_GET_NOTIFICATION		CTL_CODE(FILE_DEVICE_UNKNOWN,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)