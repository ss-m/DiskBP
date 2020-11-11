#include <Wdm.h>
#include <Ntddk.h>

// Global Variables
NPAGED_LOOKASIDE_LIST gUMNotificationMemory;
KSEMAPHORE gUMNotificationSemaphore;
SINGLE_LIST_ENTRY gUMNotificationList;
KSPIN_LOCK gUMNotificationListSpinLock;
int gAttachedDeviceCounter = 0;

#include "TypeDefinitions.h"
#include "Actions.h"

// Function Definitions
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DiskBP_Unload;
DRIVER_ADD_DEVICE DiskBP_AddDevice;
DRIVER_DISPATCH DiskBP_SkipIRP;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DiskBP_Control;
__drv_dispatchType(IRP_MJ_READ) DRIVER_DISPATCH DiskBP_Read;
__drv_dispatchType(IRP_MJ_WRITE) DRIVER_DISPATCH DiskBP_Write;
__drv_dispatchType(IRP_MJ_PNP) DRIVER_DISPATCH DiskBP_Pnp;


#include "IOFunctions.h"


NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	unsigned long Index;
	PDRIVER_DISPATCH* dispatch;
	(RegistryPath);
	for (Index = 0, dispatch = DriverObject->MajorFunction;
		Index <= IRP_MJ_MAXIMUM_FUNCTION;
		Index++, dispatch++)
	{
		*dispatch = DiskBP_SkipIRP;
	}
	DriverObject->DriverExtension->AddDevice = DiskBP_AddDevice;
	DriverObject->DriverUnload = DiskBP_Unload;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DiskBP_Control;
	DriverObject->MajorFunction[IRP_MJ_READ] = DiskBP_Read;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DiskBP_Write;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DiskBP_Pnp;


	// Initialize user-mode notification semaphore, lock, storage lookaside and list
	ExInitializeNPagedLookasideList(&gUMNotificationMemory, NULL, NULL, 0, sizeof(UMNotificationMessage), POOLTAG, 0);
	KeInitializeSpinLock(&gUMNotificationListSpinLock);
	KeInitializeSemaphore(&gUMNotificationSemaphore, 0, 1024);
	gUMNotificationList.Next = NULL;

	return STATUS_SUCCESS;
}


VOID DiskBP_Unload(IN PDRIVER_OBJECT DriverObject)
{
	(DriverObject);
	ExDeleteNPagedLookasideList(&gUMNotificationMemory);
	return;
}