#pragma once

NTSTATUS DiskBP_AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	NTSTATUS Status;
	PDEVICE_OBJECT FilterDeviceObject = NULL;
	PDeviceExtension  DeviceExtension = NULL;
	Status = IoCreateDevice(DriverObject,
		sizeof(DeviceExtension),
		NULL, PhysicalDeviceObject->DeviceType
		/*FILE_DEVICE_DISK*/,
		PhysicalDeviceObject->Characteristics,
		FALSE,
		&FilterDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		// Cannot create filter device
		return STATUS_SUCCESS;
	}

	DeviceExtension = (PDeviceExtension)FilterDeviceObject->DeviceExtension;

	if (DeviceExtension != NULL)
	{
		
		DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
		DeviceExtension->TargetDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, PhysicalDeviceObject);
		if (DeviceExtension->TargetDevice == NULL)
		{
			IoDeleteDevice(FilterDeviceObject);
			return STATUS_SUCCESS;
		}
		DeviceExtension->BPList.BPCount = 0;
		DeviceExtension->BPList.BPList = ExAllocatePoolWithTag(NonPagedPool, 1024 * sizeof(BPInfo), POOLTAG);
		if (DeviceExtension->BPList.BPList)
		{
			DeviceExtension->BPList.MaxBPCount = 1024;
		}
		DeviceExtension->DeviceID = gAttachedDeviceCounter++;
	}
	FilterDeviceObject->Flags |= DO_DIRECT_IO;
	//filterDeviceObject->Flags |= PhysicalDeviceObject->Flags;
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}


NTSTATUS DiskBP_SkipIRP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDeviceExtension)DeviceObject->DeviceExtension)->TargetDevice, Irp);
}


NTSTATUS DiskBP_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION IRPStack = IoGetCurrentIrpStackLocation(Irp);
	unsigned long long ReadBegin;
	unsigned long long ReadEnd;
	unsigned long Index = 0;

	ReadBegin = IRPStack->Parameters.Read.ByteOffset.QuadPart;
	ReadEnd = IRPStack->Parameters.Read.ByteOffset.QuadPart + IRPStack->Parameters.Read.Length;

	for (Index = 0; Index < ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPCount; Index++)
	{
		if ((ReadBegin <= ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].LocationBegin) &&
			(ReadEnd >= ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].LocationEnd) &&
			(FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, BREAK_ON_READ)))
		{
			// BP Location Matched 
			ReadFromDiskBasedOnBPList(ACTION_READ, ReadBegin, IRPStack->Parameters.Read.Length);
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, NOTIFY_UM))
			{
				SendMessageToUser(ACTION_READ, ReadBegin, IRPStack->Parameters.Read.Length);
			}
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, DENY_IO))
			{
				// Complete Irp With Status: Access Denied
				Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, TRAP_TO_DBG))
				{
					// Breakpoint
					DbgBreakPoint();
				}
				return STATUS_ACCESS_DENIED;
			}
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, TRAP_TO_DBG))
			{
				// Breakpoint
				DbgBreakPoint();
			}
		}
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDeviceExtension)DeviceObject->DeviceExtension)->TargetDevice, Irp);
}

NTSTATUS DiskBP_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION IRPStack = IoGetCurrentIrpStackLocation(Irp);
	unsigned long long WriteBegin;
	unsigned long long WriteEnd;
	unsigned long Index = 0;

	WriteBegin = IRPStack->Parameters.Write.ByteOffset.QuadPart;
	WriteEnd = IRPStack->Parameters.Write.ByteOffset.QuadPart + IRPStack->Parameters.Write.Length;
	
	for (Index = 0; Index < ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPCount; Index++)
	{
		if ((WriteBegin <= ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].LocationBegin) &&
			(WriteEnd >= ((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].LocationEnd) &&
			(FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, BREAK_ON_WRITE)))
		{
			// BP Location Matched 
			WriteToDiskBasedOnBPList(ACTION_READ, WriteBegin, IRPStack->Parameters.Write.Length);
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, NOTIFY_UM))
			{
				SendMessageToUser (ACTION_WRITE, WriteBegin, IRPStack->Parameters.Write.Length);
			}
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, DENY_IO))
			{
				// Complete Irp With Status: Access Denied
				Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, TRAP_TO_DBG))
				{
					// Breakpoint
					DbgBreakPoint();
				}
				return STATUS_ACCESS_DENIED;

			}
			if (FlagOn(((PDeviceExtension)DeviceObject->DeviceExtension)->BPList.BPList[Index].Action, TRAP_TO_DBG))
			{
				// Breakpoint
				DbgBreakPoint();
			}
		}
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDeviceExtension)DeviceObject->DeviceExtension)->TargetDevice, Irp);
}

// Handle our device related control codes and pass down others
NTSTATUS DiskBP_Control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	unsigned long OutputWritedSize = 0;
	NTSTATUS Status;
	PIO_STACK_LOCATION IRPStack = IoGetCurrentIrpStackLocation(Irp);
	
	Status  = HandleControlCode(
		IRPStack->Parameters.DeviceIoControl.IoControlCode,
		DeviceObject->DeviceExtension,
		Irp->AssociatedIrp.SystemBuffer,
		IRPStack->Parameters.DeviceIoControl.InputBufferLength,
		IRPStack->Parameters.DeviceIoControl.OutputBufferLength,
		&OutputWritedSize);
	if (Status == STATUS_SUCCESS)
	{
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = OutputWritedSize;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Status;
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDeviceExtension)DeviceObject->DeviceExtension)->TargetDevice, Irp);
}

// Handling PNP: when device is removed, detach and free allocated pool
NTSTATUS DiskBP_Pnp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION  IRPStack = IoGetCurrentIrpStackLocation(Irp);
	PDeviceExtension   DeviceExt;
	DeviceExt = (PDeviceExtension)DeviceObject->DeviceExtension;
	if (IRPStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
	{
		IoDetachDevice(DeviceExt->TargetDevice);
		IoDeleteDevice(DeviceObject);
		if (DeviceExt->BPList.BPList) ExFreePool(DeviceExt->BPList.BPList);
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDeviceExtension)DeviceObject->DeviceExtension)->TargetDevice, Irp);
}