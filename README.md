# DiskBP: Breakpoint-like functionality for Disk I/O
DiskBP, a kernel driver help you to set breakpoints or get notification on disk read/write

DiskBP helps to:
- Trap to debugger when read/write on specified byte address range
- Notify usermode process when read/write on specified byte address range
- Deny read/write on specified byte address range
