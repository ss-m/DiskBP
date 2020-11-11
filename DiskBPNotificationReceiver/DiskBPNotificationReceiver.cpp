#include <iostream>
#include <Windows.h>

using namespace std;

#define ACTION_READ		0x10000000
#define ACTION_WRITE	0x20000000
#define DEVICE_NAME L"\\\\.\\PhysicalDrive0"
#define DISKBPCTL_GET_NOTIFICATION		CTL_CODE(FILE_DEVICE_UNKNOWN,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct __UMNotificationMessage
{
	unsigned long Action;
	unsigned long Size;
	unsigned long long Location;
	int AttachedDeviceID;
} UMNotificationMessage, * PUMNotificationMessage;


HANDLE OpenDevice()
{
	return CreateFile
	(
		DEVICE_NAME,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

int main()
{
	UMNotificationMessage Notification;
	DWORD ReturnedBytes = 0;
	HANDLE DeviceHandle = OpenDevice();
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		cout << "Cannot open device";
		return 0;
	}
	while (1)
	{
		RtlZeroMemory(&Notification, sizeof(UMNotificationMessage));
		if (DeviceIoControl(DeviceHandle, DISKBPCTL_GET_NOTIFICATION, NULL, 0, &Notification, sizeof(UMNotificationMessage), &ReturnedBytes, NULL))
		{
			if (ReturnedBytes == sizeof(UMNotificationMessage))
			{
				if (Notification.Action == ACTION_READ)
					cout << "Read from: ";
				if (Notification.Action == ACTION_WRITE)
					cout << "Write to: ";
				cout << Notification.Location << " With size: " << Notification.Size << endl;
			}
			else
			{
				cout << "Invalid Message Size - LastError: " << GetLastError() << " - MessageSize Received: " << ReturnedBytes;
				CloseHandle(DeviceHandle);
				return 0;
			}
		}
		else
		{
			cout << "Error in DeviceIoControl - LastError: " << GetLastError();
			CloseHandle(DeviceHandle);
			return 0;
		}
	}
}