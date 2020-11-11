#pragma once

// To extend functionality , called when write to disk happen and match a BP
void WriteToDiskBasedOnBPList(unsigned long Action, unsigned long long Location, unsigned long Size)
{
	(Action);
	(Location);
	(Size);

	// Do something when write happened 
	return;
}

// To extend functionality , called when read from disk happen and match a BP
void ReadFromDiskBasedOnBPList(unsigned long Action, unsigned long long Location, unsigned long Size)
{
	(Action);
	(Location);
	(Size);

	// Do something when read happened 
	return;
}


// Store notification in a list and send to user 
void SendMessageToUser(unsigned long Action , unsigned long long Location , unsigned long Size)
{
	PUMNotificationMessage Message = (PUMNotificationMessage)ExAllocateFromNPagedLookasideList(&gUMNotificationMemory);
	if (Message == NULL)
	{
		return;
	}
	Message->Action = Action;
	Message->Location = Location;
	Message->Size = Size;
	//PushEntryList(&gAsyncNotifyListHead, &(Msg->SingleListEntry));
	ExInterlockedPushEntryList(&gUMNotificationList, &(Message->SingleListEntry), &gUMNotificationListSpinLock);
	//
	__try
	{
		KeReleaseSemaphore(&gUMNotificationSemaphore, 0, 1, FALSE);
	}
	__except (GetExceptionCode() == STATUS_SEMAPHORE_LIMIT_EXCEEDED ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		PSINGLE_LIST_ENTRY List;
		PUMNotificationMessage Message2;

		List = ExInterlockedPopEntryList(&gUMNotificationList, &gUMNotificationListSpinLock);
		if (List)
		{
			Message2 = CONTAINING_RECORD(List, UMNotificationMessage, SingleListEntry);
			if (Message2)
			{
				ExFreeToNPagedLookasideList(&gUMNotificationMemory, Message2);
			}
		}
	}
	return;
}

NTSTATUS HandleControlCode(ULONG CtlCode,PDeviceExtension DeviceExt , PVOID Buffer, unsigned int InputSize, unsigned int OutputSize, unsigned long* WritedSize)
{
	*WritedSize = 0;
	switch (CtlCode)
	{
	case DISKBPCTL_ADD_BP:
		if (InputSize == sizeof(BPInfo))
		{
			if (DeviceExt->BPList.BPCount < DeviceExt->BPList.MaxBPCount)
			{
				RtlCopyMemory(&DeviceExt->BPList.BPList[DeviceExt->BPList.BPCount], Buffer, sizeof(BPInfo));
				DeviceExt->BPList.BPCount++;
				return STATUS_SUCCESS;
			}
		}
		break;
	case DISKBPCTL_CLEAR_ALL_BP:
		if ((InputSize == 0) & (OutputSize == 0))
		{
			DeviceExt->BPList.BPCount = 0;
			return STATUS_SUCCESS;
		}
		break;
	case DISKBPCTL_GET_NOTIFICATION:
		if ((InputSize == 0) & (OutputSize == (sizeof(UMNotificationMessage) - sizeof(SINGLE_LIST_ENTRY))))
		{
			PUMNotificationMessage Message;
			if (NT_SUCCESS(KeWaitForSingleObject(&gUMNotificationSemaphore, Executive, KernelMode, FALSE, NULL)))
			{
				PSINGLE_LIST_ENTRY ListItem;
				ListItem = ExInterlockedPopEntryList(&gUMNotificationList, &gUMNotificationListSpinLock);
				Message = CONTAINING_RECORD(ListItem, UMNotificationMessage, SingleListEntry);
				RtlCopyMemory(Buffer, Message, sizeof(UMNotificationMessage) - sizeof(SINGLE_LIST_ENTRY));
				*WritedSize = sizeof(UMNotificationMessage) - sizeof(SINGLE_LIST_ENTRY);
				ExFreeToNPagedLookasideList(&gUMNotificationMemory, Message);
				return STATUS_SUCCESS;
			}
		}
		break;
	}

	return STATUS_NOT_IMPLEMENTED;

}