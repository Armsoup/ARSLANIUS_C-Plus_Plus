# ARSLANIUS 28 On C++

ARSLANIUS 28 On C++ is a portable console operating system ported from the original [ARSLANIUS 28](https://github.com/Armsoup/ARSLANIUS) to native C++17 (x64).

PLEASE SEND YOUR IDEAS on [Telegram](https://t.me/+8FQ20tOaKI5lNGMy) or [Discussions](https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/discussions)

> **What Windows NT was to Windows 9x — same interface, completely new kernel.**

Starting from the original batch version, the project evolves into a standalone `.exe` with its own console handling via WinAPI, user system, registry, background services, restore points, and application ecosystem.

The system runs directly from any folder and automatically initializes itself on first launch.

* * *

Features

• Boot menu with Safe Mode, Diagnostic Mode and manual

• Boot animation (17-frame loading bar)

• User accounts: SYSTEM, SYSTEM ADMINISTRATOR, GUEST, and custom users

• Access control with 5 privilege levels

• sudo command with admin password verification

• Background services (SysPulse, NetMonitor, TrustedInstaller)

• Recovery Environment with 6 options

• Startup Repair — recreates kernel, registry, and BCD

• Restore Points and System Restore

• System Image Backup and Recovery

• Memory Diagnostic tool

• ARS Store (application installer — Scanner, NotePad Lite, Calc)

• OOBE (Out-Of-Box Experience) — first-time setup with Windows XP setup music

• Full event logging (system.log)

• 14 BSOD (Blue Screen of Death) types with stop codes

• Integrity protection — hash checks, file size limits, config structure validation on every boot

• Lockdown mode — blocks all logins until OOBE is completed

• Portable architecture using executable path detection

• x64 native — no 32-bit limitations

* * *
Build numbering logic

Build 58.1.0

58 - VERSION (For example, ARSLANIUS 28 - 58.x.x)

1 - STAGE (0 -  alpha, 1 - beta, 2 - RC/Release, 3 - SP)

0 - PATCH OR VERSION BETA/ALPHA/RC
      
* * *

# ARSLANIUS Driver SDK

The ARSLANIUS Driver SDK allows anyone to extend the ARSLANIUS operating system with custom drivers written in C++.

## Quick Start

### 1. Prerequisites

- Microsoft Visual Studio 2019 or later (with C++ workload)
- ARSLANIUS 28 (Build 58.2.0 or later)

### 2. Get the SDK

Copy these files to your project folder:

- `arslanius.h` — The SDK header

- `template.cpp` — Sample driver to use as a starting point

### 3. Build Your Driver

Open **x64 Native Tools Command Prompt for VS 2019** and run:

```bat
cl /LD your_driver.cpp /Fe:your_driver.asd /EHsc /MT
```

This creates your_driver.asd — a 64-bit DLL with all dependencies statically linked.

1. Copy your_driver.asd to the Drivers folder inside Settings And System Files
2. Restart ARSLANIUS
3. The driver will load automatically at boot
4. Verify Installation

After boot, your driver's commands will be available in the ARSLANIUS shell.

Type help to see all available commands (including driver commands).

---

Sample Driver: template.asd

The included template.cpp demonstrates all 27 API functions and provides 4 commands:

```cpp
// template.asd - ARSLANIUS Sample Driver (SDK v2.0)
// Demonstrates ALL API functions for driver developers.
//
// Build (x64 Native Tools Command Prompt for VS):
//   cl /LD template.cpp /Fe:template.asd /EHsc /MT
//
// Copy template.asd to the Drivers folder and restart ARSLANIUS.

#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include "arslanius.h"

using namespace std;

ARSLANIUS_API* api = nullptr;

// =====================================================================
// COMMAND: sdk.demo — Demonstrates all API functions
// =====================================================================
void cmd_sdk_demo(const string& args) {
    api->clear_screen();
    api->set_color("0e");
    api->print("============================================================\n");
    api->print("         ARSLANIUS DRIVER SDK DEMO\n");
    api->print("============================================================\n\n");
    
    // === SYSTEM INFO ===
    api->print("[ SYSTEM ]\n");
    api->print("  OS: "); api->print(api->get_os_name()); api->print("\n");
    api->print("  Build: "); api->print(api->get_build()); api->print("\n");
    api->print("  User: "); api->print(api->get_current_user()); api->print("\n");
    api->print("  Safe Mode: "); api->print(api->get_safe_mode() ? "YES" : "NO"); api->print("\n");
    api->print("  Root: "); api->print(api->get_root_path()); api->print("\n");
    api->print("\n");
    
    // === REGISTRY ===
    api->print("[ REGISTRY ]\n");
    const char* osName = api->read_registry("OS_NAME");
    api->print("  OS_NAME = "); api->print(osName); api->print("\n");
    
    api->write_registry("SDK_DEMO", "Hello from driver!");
    api->print("  [ OK ] Wrote SDK_DEMO to registry\n");
    
    const char* demoVal = api->read_registry("SDK_DEMO");
    api->print("  SDK_DEMO = "); api->print(demoVal); api->print("\n");
    
    api->delete_registry("SDK_DEMO");
    api->print("  [ OK ] Deleted SDK_DEMO\n");
    api->print("\n");
    
    // === FILES ===
    api->print("[ FILES ]\n");
    
    if (api->file_exists("kernel.dll")) {
        api->print("  kernel.dll: EXISTS\n");
    }
    
    if (api->create_directory("SDK_Test")) {
        api->print("  [ OK ] Created folder: SDK_Test\n");
    }
    
    api->write_file("SDK_Test\\demo.txt", "Created by ARSLANIUS SDK Demo.\n");
    api->print("  [ OK ] Wrote: SDK_Test\\demo.txt\n");
    
    api->append_file("SDK_Test\\demo.txt", "This line was appended.\n");
    api->print("  [ OK ] Appended to demo.txt\n");
    
    const char* content = api->read_file("SDK_Test\\demo.txt");
    api->print("  demo.txt content:\n");
    api->print(content);
    
    api->delete_file("SDK_Test\\demo.txt");
    api->print("  [ OK ] Deleted demo.txt\n");
    
    api->shell_execute("rmdir /s /q SDK_Test");
    api->print("  [ OK ] Removed folder\n");
    api->print("\n");
    
    // === SECURITY ===
    api->print("[ SECURITY ]\n");
    
    const char* hash = api->calculate_hash("Acy98iolop_isArslanius-kop");
    api->print("  Hash of user 'SYSTEM': "); api->print(hash); api->print("\n");
    
    api->print("  User 'SYSTEM': ");
    api->print(api->user_exists("SYSTEM") ? "EXISTS" : "NOT FOUND");
    api->print("\n\n");
    
    // === OUTPUT ===
    api->print("[ OUTPUT ]\n");
    api->print("  print() - normal output\n");
    api->print_slow("  print_slow() - typewriter effect\n");
    api->print("\n");
    
    api->print("[ DONE ] SDK Demo complete!\n");
    api->write_log("SDK_DEMO_RUN");
    api->pause();
}

// =====================================================================
// COMMAND: sdk.calc — Interactive calculator
// =====================================================================
void cmd_sdk_calc(const string& args) {
    api->print("=== CALCULATOR ===\n");
    api->print("Enter expression (e.g., 5+7) or 'exit'\n\n");
    
    while (true) {
        api->print("calc> ");
        
        string input;
        getline(cin, input);
        
        if (input.empty()) continue;
        if (input == "exit" || input == "quit") break;
        
        int a, b;
        char op;
        stringstream ss(input);
        ss >> a >> op >> b;
        
        if (ss.fail()) {
            api->print("[ ERROR ] Use format: number+number\n");
            continue;
        }
        
        int result = 0;
        switch (op) {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': result = a * b; break;
            case '/': 
                if (b == 0) { api->print("Error: division by zero\n"); continue; }
                result = a / b; 
                break;
            default:
                api->print("Unknown operator. Use +, -, *, /\n");
                continue;
        }
        
        char buf[64];
        sprintf(buf, "%d %c %d = %d\n", a, op, b, result);
        api->print(buf);
    }
    
    api->print("Bye!\n");
}

// =====================================================================
// COMMAND: sdk.time — Current date and time
// =====================================================================
void cmd_sdk_time(const string& args) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    char buf[64];
    sprintf(buf, "Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
    api->print(buf);
    
    sprintf(buf, "Date: %02d.%02d.%d\n", st.wDay, st.wMonth, st.wYear);
    api->print(buf);
}

// =====================================================================
// COMMAND: sdk.info — Driver information
// =====================================================================
void cmd_sdk_info(const string& args) {
    api->print("=== SDK Sample Driver ===\n");
    api->print("Version: 1.0.0\n");
    api->print("Author: ARSLANIUS Community\n");
    api->print("\nCommands provided:\n");
    api->print("  sdk.demo  - Full API demonstration\n");
    api->print("  sdk.calc  - Interactive calculator\n");
    api->print("  sdk.time  - Current date and time\n");
    api->print("  sdk.info  - This information\n");
}

// =====================================================================
// DRIVER ENTRY POINT
// =====================================================================
extern "C" __declspec(dllexport) int asd_init(ARSLANIUS_API* a) {
    api = a;
    
    api->register_command("sdk.demo", cmd_sdk_demo);
    api->register_command("sdk.calc", cmd_sdk_calc);
    api->register_command("sdk.time", cmd_sdk_time);
    api->register_command("sdk.info", cmd_sdk_info);
    
    api->print("[ DRIVER ] SDK Sample loaded. Commands: sdk.demo, sdk.calc, sdk.time, sdk.info\n");
    api->write_log("DRIVER_SDK_SAMPLE_LOADED");
    
    return BAROS_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID lp) { return TRUE; }
```

Command Description

sdk.demo Full API demonstration (registry, files, security, output)

sdk.calc Interactive calculator (+, -, *, /)

sdk.time Current date and time

sdk.info Driver information and available commands

To try it:

```bat
cl /LD template.cpp /Fe:template.asd /EHsc /MT
copy template.asd "C:\Path\To\ARSLANIUS\Settings And System Files\Drivers\"
```

Then restart ARSLANIUS and type sdk.demo.

---

API Reference

Commands & Shell

```cpp
void register_command(const char* name, CommandHandler handler);
void unregister_command(const char* name);
void execute_command(const char* cmd);
```

Console Output

```cpp
void print(const char* text);
void print_slow(const char* text);     // Typewriter effect
void clear_screen();
void set_color(const char* color);     // VGA hex: "0e", "1f", "4f", etc.
void pause();
```

Registry

```cpp
const char* read_registry(const char* key);
void write_registry(const char* key, const char* value);
void delete_registry(const char* key);
```

File System

```cpp
bool file_exists(const char* path);
bool dir_exists(const char* path);
const char* read_file(const char* path);
void write_file(const char* path, const char* content);
void append_file(const char* path, const char* content);
bool delete_file(const char* path);
bool create_directory(const char* path);
```

System

```cpp
void write_log(const char* message);
const char* get_current_user();
const char* get_os_name();
const char* get_build();
int get_safe_mode();
int shell_execute(const char* command);   // Runs external programs
const char* get_root_path();
const char* get_config_path();
const char* get_users_path();
const char* get_drivers_path();
```

Security

```cpp
const char* calculate_hash(const char* input);
bool user_exists(const char* username);
```

---

Driver Structure

Every driver must export this function:

```cpp
extern "C" __declspec(dllexport) int asd_init(ARSLANIUS_API* api);
```

It receives the API table and returns 0 on success.

Minimal driver example:

```cpp
#include <windows.h>
#include "arslanius.h"

ARSLANIUS_API* api = nullptr;

void my_command(const std::string& args) {
    api->print("Hello from my driver!\n");
}

extern "C" __declspec(dllexport) int asd_init(ARSLANIUS_API* a) {
    api = a;
    api->register_command("mycommand", my_command);
    api->print("[ DRIVER ] My driver loaded!\n");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID lp) { return TRUE; }
```

---

File Extensions

Extension Purpose

.asd ARSLANIUS Driver (compiled DLL)

.cpp Driver source code (C++)

arslanius.h SDK header file

---

Troubleshooting

Driver not loading (0 loaded, 0 failed):

· Check that the .asd file is in the Drivers folder inside Settings And System Files

· Check that the file extension is .asd (not .dll or .asd.dll)

Driver fails to load (Error 193):

· The driver must be compiled as 64-bit (x64). Use x64 Native Tools Command Prompt.

· ARSLANIUS 28 RC and later are 64-bit applications.

Commands not working after loading:

· Ensure command names are lowercase and contain no spaces.

· Check that asd_init returned 0.

* * *

System Structure

ARSLANIUS/

├─ ARSLANIUS 28.exe

├─ Backup/

├─ Settings And System Files/

│⠀├─ kernel.dll

│⠀├─ REG.cfg

│⠀├─ BCD

│⠀├─ system.log

│⠀└─ systemprofile/

├─ Users/

├─ Programs/

└─ RestorePoints/

* * *

How to Run

1. Copy the ARSLANIUS folder to any location.

2. Launch ARSLANIUS 28.exe.

3. On first boot, complete OOBE (Out-Of-Box Experience) to create your user account.

4. Login as your user.

5. Type help for a list of commands.
