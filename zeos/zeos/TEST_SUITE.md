# ZEOS Milestone 3 & 4 Test Suite for Professor

## Overview
All tests are **automated** and run when the kernel boots. Tests are located in `user.c`.

## Automated Test Suites

### 1. **Write Validation Tests**
- Test invalid file descriptors (`write(fd=0)` should return -1)
- Test negative sizes (`write(size<0)` should return -1)  
- Test NULL pointers (`write(NULL)` should return -1)
- errno validation for each error case
- **Expected Output**: 3 PASS tests

### 2. **Read Validation Tests**
- Test NULL buffer pointer (`read(NULL)` should return -1)
- Test negative max size (`read(size<0)` should return -1)
- Test zero max size (`read(size=0)` should return 0)
- errno validation for each error case
- **Expected Output**: 3 PASS tests

### 3. **Gotoxy/Set_Color Tests** (Milestone 4)
- `set_color(valid)` returns 0
- `get_color()` retrieves the set color correctly
- `set_color(invalid)` returns -22 (EINVAL)
- `gotoxy(valid)` returns 0
- `gotoxy(invalid)` returns -22 (EINVAL)
- **Expected Output**: 5 PASS tests

### 4. **Fork/Block/Unblock Tests**
- Parent/child PID validation
- Memory copy isolation
- Block/unblock synchronization
- **Expected Output**: 15+ PASS tests

### 5. **Screen Color Demo** (Manual)
After automated tests, the kernel displays:
- RED text at position (0,22)
- GREEN text at position (0,23)
- Uses `gotoxy()` and `set_color()` syscalls

### 6. **Interactive Features**
- **Keyboard buffer demo**: Press F1 to dump buffer contents
- **Keyboard read tests**: 3 multitasking tasks reading from keyboard simultaneously
- **Frame 2000 test**: Press 'q' to write into high memory frame (Milestone 3 validation)

## How to Run Tests

### Option 1: Build Only (Recommended if Bochs has issues)
```bash
cd /home/alumne/Documents/SOA/SOA/zeos/zeos
make -j4
# zeos.bin is created, ready to run on any x86 emulator
```

### Option 2: Run with Bochs (if ROMs available)
```bash
cd /home/alumne/Documents/SOA/SOA/zeos/zeos
bochs -f bochsrc
# Follow on-screen prompts
```

### Option 3: Run with QEMU
```bash
cd /home/alumne/Documents/SOA/SOA/zeos/zeos
qemu-system-i386 -fda zeos.bin -no-shutdown -no-reboot
```

## Expected Test Output Summary

```
======= ZEOS USER FULL TEST =======
[PASS] getpid() in parent > 0
[PASS] gettime() initial value >= 0
[PASS] initial memory probe value
[PASS] write(valid args) returns byte count

-- write() validation tests --
[PASS] write(fd=0) returns -1
[PASS] errno=EBADF (9)
[PASS] write(size<0) returns -1
[PASS] errno=EINVAL (22)
[PASS] write(NULL) returns -1
[PASS] errno=EFAULT (14)

-- read() validation tests --
[PASS] read(NULL) returns -1
[PASS] errno=EFAULT (14)
[PASS] read(size<0) returns -1
[PASS] errno=EINVAL (22)
[PASS] read(size=0) returns 0

-- gotoxy/set_color tests --
[PASS] set_color(valid) returns 0
[PASS] get_color returns set value
[PASS] set_color(fg<0) returns -22
[PASS] set_color(bg>15) returns -22
[PASS] gotoxy(valid) returns 0
[PASS] gotoxy(x>=80) returns -22

-- fork/block/unblock/exit tests --
[PASS] fork() succeeded
[PASS] parent got child pid > 0 from fork
[...more fork/block tests...]

[SUMMARY] tests passed: 25+
[SUMMARY] tests total : 25+
ALL TESTS PASS

[DEMO] gotoxy/set_color demo:
This is RED text at (0,22)
This is GREEN text at (0,23)
```

## Milestone Validation Checklist

### Milestone 3 (Extended Physical Memory to 2048 frames)
✅ Static system page tables replaced with `alloc_frame()` allocation
✅ Two-window mapping system (low 0-1023 identity, high 1024-2047 via second PT)
✅ All syscalls using `map_system_frame()`/`unmap_system_frame()` helpers
✅ Frame 2000 test accessible via 'q' key (writes high memory)

### Milestone 4 (Screen Control Syscalls)
✅ `int gotoxy(int x, int y)` - set cursor position
✅ `int set_color(int fg, int bg)` - set text color (0-15 for fg/bg)
✅ `int get_color(void)` - retrieve current color
✅ Bounds checking and error codes (returns -22 for invalid args)
✅ Automated validation tests with 5+ assertions

## Files Modified
- `sys.c`: Added `sys_gotoxy()`, `sys_set_color()`, `sys_get_color()`
- `sys_call_table.S`: Registered syscalls at slots 11, 12, 13
- `wrappers.S`: Added user-space syscall stubs
- `libc.h`: Exposed prototypes for new syscalls
- `io.c`: Added screen position/color management
- `user.c`: Added 5+ automated test functions

## Build Status
✅ Compiles without errors
✅ All 3+ files successfully edited
✅ Disk image created: `zeos.bin` (115 KB)
