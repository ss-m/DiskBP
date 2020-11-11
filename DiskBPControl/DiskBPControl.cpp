#include <iostream>
#include <Windows.h>

using namespace std;

#define DEVICE_NAME L"\\\\.\\PhysicalDrive0"
#define DISKBPCTL_ADD_BP				CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define DISKBPCTL_CLEAR_ALL_BP			CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_NEITHER,FILE_ANY_ACCESS)
#define BREAK_ON_READ	0x00010000
#define BREAK_ON_WRITE	0x00020000

typedef struct __BPInfo
{
	unsigned long Action;
	unsigned long long LocationBegin;
	unsigned long long LocationEnd;
}BPInfo, * PBPInfo;

HANDLE OpenDevice()
{
	return CreateFile
	(
		DEVICE_NAME,
		GENERIC_READ,
		FILE_SHARE_READ| FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

void DiskBPCtl_AddBP()
{
	int BreakOn = 0;
	BPInfo Breakpoint;
	DWORD ReturnedBytes = 0;
	HANDLE DeviceHandle = OpenDevice();
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		cout << "Cannot open device";
		return;
	}
	cout << "Breakpoint Begin At Byte: ";
	cin >> Breakpoint.LocationBegin;
	cout << "Breakpoint End At Byte: ";
	cin >> Breakpoint.LocationEnd;
	cout << "Breakpoint Action (TRAP_TO_DBG:1, NOTIFY_UM:2, DENY_IO:4): ";
	cin >> Breakpoint.Action;

	cout << "Break on(READ:1, WRITE:2): ";
	cin >> BreakOn;
	if (BreakOn & 1)
		Breakpoint.Action |= BREAK_ON_READ;
	if (BreakOn & 2)
		Breakpoint.Action |= BREAK_ON_WRITE;
	
	if (DeviceIoControl(DeviceHandle, DISKBPCTL_ADD_BP, &Breakpoint, sizeof(BPInfo), NULL, 0, &ReturnedBytes, NULL))
	{
		cout << "Breakpoint added!";
	}
	else
	{
		cout << "Error in DeviceIoControl - LastError: " << GetLastError();
	}
	CloseHandle(DeviceHandle);
	return;
}

void DiskBPCtl_ClearBPs()
{
	DWORD ReturnedBytes = 0;
	HANDLE DeviceHandle = OpenDevice();
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		cout << "Cannot open device";
		return;
	}
	if (DeviceIoControl(DeviceHandle, DISKBPCTL_CLEAR_ALL_BP, NULL, 0, NULL, 0, &ReturnedBytes, NULL))
	{
		cout << "All breakpoints cleared! ";
	}
	else
	{
		cout << "Error in DeviceIoControl - LastError: " << GetLastError();
	}
	CloseHandle(DeviceHandle);
	return;
}


int main()
{
	int Option;
	while (1)
	{
		cout << "Select option (1- Add Breakpoint , 2- Clear all breakpoints): ";
		cin >> Option;
		switch (Option)
		{
		case 1:
			DiskBPCtl_AddBP();
			break;
		case 2:
			DiskBPCtl_ClearBPs();
			break;
		}
		cout << endl;
		system("pause");
		system("cls");
	}
}
