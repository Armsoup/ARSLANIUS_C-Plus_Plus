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
