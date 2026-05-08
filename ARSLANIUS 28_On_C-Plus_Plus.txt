#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
#include <conio.h>
#include <direct.h>
#include <io.h>

#pragma comment(lib, "winmm.lib")

using namespace std;

// =====================================================================
// CONSTANTS
// =====================================================================
const string CURRENT_BUILD = "58.2";
const string REG_VERSION = "28";
const string EXPECTED_SYSTEM_HASH = "350703396";
const string EXPECTED_ADMIN_HASH = "734380451";
const string OS_NAME_DEFAULT = "ARSLANIUS 28";
const string ADMIN_PASSWORD_DEFAULT = "Jiupolaqmn_isArslanius-lo";
const string SYSTEM_PASSWORD_HASH = "350703396"; // hash of SYSTEM default pass
const int BOOT_TIMEOUT_DEFAULT = 30;
const int DEFAULT_MODE_DEFAULT = 1;
const int MAX_LOGIN_ATTEMPTS = 10;
const size_t MAX_KERNEL_SIZE = 12288;
const size_t MAX_REGISTRY_SIZE = 2048;
const size_t MAX_LOG_SIZE = 153600;

// =====================================================================
// GLOBAL VARIABLES
// =====================================================================
string rootPath;
string configRoot;
string kernelPath;
string usersRoot;
string programsRoot;
string sysProf;
string sysServices;
string regPath;
string logPath;
string restoreRoot;

string currentUser = "SYSTEM";
string userHome;
string osName = OS_NAME_DEFAULT;
string currentBuild = CURRENT_BUILD;

int bootTimeout = BOOT_TIMEOUT_DEFAULT;
int defaultMode = DEFAULT_MODE_DEFAULT;
int bootChoice = 0;
int safeMode = 0;
int rec = 0;
int diagnostic = 0;
int logonSuccessOk = 0;
int lastSuccessfulMode = 0;
int initctrl = 0;
int waitMode = 0;
int logoutRequest = 0;
int acpiRequest = 0;
int enableLua = 1;
string recoveryRequest = "0";

string systemColor = "0e";
string adminColor = "4f";
string userColor = "1f";

string regKey = "SYSTEM_COLOR";

// =====================================================================
// FORWARD DECLARATIONS
// =====================================================================
void initPaths();
void loadBCD();
void saveBCD();
void bootMenu();
void normalBoot();
void safeModeBoot();
void recoveryEnv();
void diagnosticMode(int mode);
void startupRepair();
void restoreMenu();
void imageRecovery();
void memoryDiag();
void logonScreen();
void applyColor();
void cmdLoop();
void core(const string& cmd);
bool checkHash(const string& input, const string& expectedHash);
string calculateHash(const string& input);
void bsod(const string& code);
void shutdownScreen();
void rebootScreen();
void interfaceScreen();
bool fileExists(const string& path);
bool dirExists(const string& path);
vector<string> split(const string& s, char delimiter);
string trim(const string& s);
string getCurrentDateTime();
void writeLog(const string& message);
string readFile(const string& path);
void writeFile(const string& path, const string& content);
void appendFile(const string& path, const string& content);
void setColor(const string& colorCode);

// =====================================================================
// IMPLEMENTATION: UTILITY FUNCTIONS
// =====================================================================

void initPaths() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string exePath = buffer;
    size_t lastSlash = exePath.find_last_of("\\/");
    rootPath = exePath.substr(0, lastSlash);

    configRoot = rootPath + "\\Settings And System Files";
    kernelPath = configRoot + "\\kernel.dll";
    usersRoot = rootPath + "\\Users";
    programsRoot = rootPath + "\\Programs";
    sysProf = configRoot + "\\systemprofile";
    sysServices = sysProf;
    regPath = configRoot + "\\REG.cfg";
    logPath = configRoot + "\\system.log";
    restoreRoot = rootPath + "\\RestorePoints";

    userHome = sysProf;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        token = trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

bool fileExists(const string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool dirExists(const string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

string getCurrentDateTime() {
    auto now = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(now);
    struct tm tm_info;
    localtime_s(&tm_info, &tt);
    char buf[64];
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &tm_info);
    return string(buf);
}

void writeLog(const string& message) {
    ofstream log(logPath, ios::app);
    if (log.is_open()) {
        log << "[" << getCurrentDateTime() << " " << message << "]" << endl;
    }
}

string readFile(const string& path) {
    ifstream f(path);
    if (!f.is_open()) return "";
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void writeFile(const string& path, const string& content) {
    ofstream f(path);
    if (f.is_open()) {
        f << content;
    }
}

void appendFile(const string& path, const string& content) {
    ofstream f(path, ios::app);
    if (f.is_open()) {
        f << content;
    }
}

void setColor(const string& colorCode) {
    if (colorCode == "0f") system("color 0f");
    else if (colorCode == "0e") system("color 0e");
    else if (colorCode == "4f") system("color 4f");
    else if (colorCode == "1f") system("color 1f");
    else if (colorCode == "5b") system("color 5b");
    else if (colorCode == "07") system("color 07");
    else if (colorCode == "9f") system("color 9f");
    else if (colorCode == "17") system("color 17");
    else if (colorCode == "04") system("color 04");
    else system("color 07");
}

string calculateHash(const string& input) {
    long long hashVal = 0;
    const int salt = 79;

    for (char c : input) {
        int code = 0;
        if (c >= 'a' && c <= 'z') code = c - 'a' + 1;
        else if (c >= 'A' && c <= 'Z') code = c - 'A' + 27;
        else if (c >= '0' && c <= '9') code = c - '0' + 53;
        else if (c == '_') code = 63;
        else if (c == '-') code = 64;
        else if (c == '?') code = 65;
        else code = 66;

        hashVal = hashVal * 31 + code + salt;
        hashVal = hashVal % 1000000000;
    }
    int truncated = (int)(hashVal % 256);
    if (truncated >= 128) {
        truncated = truncated - 256;
    }

    return to_string(hashVal);
}

bool checkHash(const string& input, const string& expectedHash) {
    return calculateHash(input) == expectedHash;
}

void loadBCD() {
    string bcdPath = configRoot + "\\BCD";
    if (!fileExists(bcdPath)) return;

    ifstream bcd(bcdPath);
    string line;
    while (getline(bcd, line)) {
        size_t sep = line.find('=');
        if (sep == string::npos) continue; 
        string key = line.substr(0, sep);
        string value = line.substr(sep + 1);

        if (value.empty()) continue; 

        try {
            if (key == "BOOT_TIMEOUT") bootTimeout = stoi(value);
            else if (key == "DEFAULT_MODE") defaultMode = stoi(value);
            else if (key == "LAST_SUCCESSFUL_MODE") lastSuccessfulMode = stoi(value);
        }
        catch (...) {
            continue;
        }
    }
}


void saveBCD() {
    string bcdPath = configRoot + "\\BCD";
    stringstream ss;
    ss << "BOOT_TIMEOUT=" << bootTimeout << endl;
    ss << "DEFAULT_MODE=" << defaultMode << endl;
    ss << "LAST_SUCCESSFUL_MODE=" << lastSuccessfulMode << endl;
    ss << "BOOT_COUNT=0" << endl;
    ss << "LAST_BOOT_SUCCESS=" << getCurrentDateTime() << endl;
    writeFile(bcdPath, ss.str());
}

void Manual() {
    system("cls");
    cout << "======================================================================================================================" << endl;
    cout << "                                         ARSLANIUS 28 - USER MANUAL" << endl;
    cout << "======================================================================================================================" << endl;
    cout << "" << endl;
    cout << "[ QUICK START ]" << endl;
    cout << "1. First run? Press R at BSoD then press 1 [Startup Repair]" << endl;
    cout << "2. Login as SYSTEM ADMINISTRATOR [password: Jiupolaqmn_isArslanius-lo]" << endl;
    cout << "3. Type 'adduser', then enter your name and your password" << endl;
    cout << "4. Type 'lock', then login as your user" << endl;
    cout << "" << endl;
    cout << "[ ARSLANIUS BOOT MANAGER ]" << endl;
    cout << "  - Press 1-7 to select mode" << endl;
    cout << "  - Auto-boot in %boot_timeout% seconds" << endl;
    cout << "" << endl;
    cout << "[ COMMANDS ]" << endl;
    cout << "System: help, lock, cls, ver, whoami, reboot, shutdown, lockmenu [ctrl alt del]" << endl;
    cout << "Files: ls, cd, cat, ren, mkdir, touch, edit, cp, mv, rm" << endl;
    cout << "Admin: adduser, deluser, passwd, regedit, bcdedit, bcdboot, reset" << endl;
    cout << "Network: ping, netstat, ipconfig, tracert, nslookup, arp, route" << endl;
    cout << "Recovery: backup, backup-restore, restore-point, restore, sfc, events" << endl;
    cout << "Fun: bsod, Notepad, wait_mode, echo" << endl;
    cout << "" << endl;
    cout << "[ RECOVERY ENVIRONMENT ]" << endl;
    cout << "1 [Startup Repair] - recreates all files" << endl;
    cout << "4 [Mini cmd] - command line with recovery tools" << endl;
    cout << "2 or 3 - restore from restore points or backup" << endl;
    cout << "======================================================================================================================" << endl;
    system ("pause");
    bootMenu();
}

void Update() {
    cout << "If you have files from an older version, place them in the root folder along with the .cmd file and press Y." << endl;
    cout << "Y/N: ";
    char c = _getch();
    cout << c << endl;

    if (toupper(c) == 'Y') {
        if (!fileExists(kernelPath)) {
            cout << "[ ERROR ] kernel.dll not found.";
            bsod("1");
        }
        saveBCD();

        stringstream reg_update;
        reg_update << "OS_NAME = ARSLANIUS 28" << endl;
        reg_update << "SYSTEM_COLOR=0e" << endl;
        reg_update << "ADMIN_COLOR=4f" << endl;
        reg_update << "USER_COLOR=1f" << endl;
        reg_update << "ENABLE_LUA=1" << endl;
        reg_update << "REG_VERSION=" << REG_VERSION << endl;
        writeFile(regPath, reg_update.str());
        bootMenu();
    }
    bootMenu();
}

void ensureDirectories() {
    if (!dirExists(configRoot)) CreateDirectoryA(configRoot.c_str(), NULL);
    if (!dirExists(sysServices)) CreateDirectoryA(sysServices.c_str(), NULL);
    if (!dirExists(usersRoot)) CreateDirectoryA(usersRoot.c_str(), NULL);
    if (!dirExists(programsRoot)) CreateDirectoryA(programsRoot.c_str(), NULL);
}

// =====================================================================
// BSOD SCREENS
// =====================================================================

void bsod(const string& code) {
    system("cls");

    if (code == "1a") {
        setColor("17");
        cout << "*** STOP: 0x00000001a [0xc00000001a, 0x00000000, 0x00000000, 0x00000000]" << endl;
        cout << endl;
        cout << "*** File: \\Settings And System Files" << endl;
        cout << endl;
        cout << "CONFIG_ROOT_NOT_FOUND - The config root is missing." << endl;
        cout << "Please reinstall or run Startup Repair." << endl;
        cout << endl;
        cout << "If this is the first time you've seen this error, restart the system." << endl;
        cout << endl;
        cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
        system("pause");
        recoveryEnv();
    }
    else if (code == "1") {
        setColor("17");
        cout << "*** STOP: 0x00000001 [0xc00000001, 0x00000000, 0x00000000, 0x00000000]" << endl;
        cout << endl;
        cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
        cout << endl;
        cout << "KERNEL_NOT_FOUND - The kernel.dll is missing." << endl;
        cout << "Please reinstall or run Startup Repair." << endl;
        cout << endl;
        cout << "If this is the first time you've seen this error, restart the system." << endl;
        cout << endl;
        cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
        system("pause");
        recoveryEnv();
    }
    else if (code == "4") {
        setColor("17");
        cout << "*** STOP: 0x00000004 [0xc00000004, 0x00000000, 0x00000000, 0x00000000]" << endl;
        cout << endl;
        cout << "*** File: \\Settings And System Files\\REG.cfg" << endl;
        cout << endl;
        cout << "REGISTRY_NOT_FOUND - The REG.cfg is missing." << endl;
        cout << "Please reinstall or run Startup Repair." << endl;
        cout << endl;
        cout << "If this is the first time you've seen this error, restart the system." << endl;
        cout << endl;
        cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
        system("pause");
        recoveryEnv();
    }
    else if (code == "14") {
        setColor("17");
        cout << "*** STOP: 0x00000014 [0xc00000014, 0x00000000, 0x00000000, 0x00000000]" << endl;
        cout << endl;
        cout << "BCD_NOT_FOUND - The BCD is missing." << endl;
        cout << "Please reinstall or run Startup Repair." << endl;
        cout << endl;
        cout << "If this is the first time you've seen this error, restart the system." << endl;
        cout << endl;
        cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
        system("pause");
        recoveryEnv();
    }
    else if (code == "666") {
        setColor("17");
        cout << "*** STOP: 0xDEADBEEF [0x00000666, 0x00000000, 0x00000000, 0x00000000]" << endl;
        cout << endl;
        cout << "MANUAL_CRASH - You typed 'bsod' and now you're here." << endl;
        system("pause");
        bootMenu();
    }
    else {
        setColor("17");
        cout << "*** STOP: 0x00001225a [UNKNOWN_ERROR]" << endl;
        system("pause");
        exit(1);
    }
}

// =====================================================================
// BOOT MENU
// =====================================================================

void bootMenu() {
    loadBCD();

    if (recoveryRequest != "1") {
        system("cls");
        setColor("0f");
        cout << "======================================================================================================================" << endl;
        cout << "                                                 ARSLANIUS BOOT MANAGER" << endl;
        cout << "======================================================================================================================" << endl;
        if (lastSuccessfulMode != 0) {
            cout << "  Last Known Good Configuration [Mode " << lastSuccessfulMode << "]" << endl;
        }
        cout << "  1. Start ARSLANIUS Normally" << endl;
        cout << "  2. Safe Mode [No Services / No Autorun]" << endl;
        cout << "  3. Recovery Environment" << endl;
        cout << "  4. Diagnostic Mode LOCAL" << endl;
        cout << "  5. Diagnostic Mode NETWORK" << endl;
        cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
        cout << "  Auto-boot in " << bootTimeout << " seconds..." << endl;
        cout << "  Press 6 for Manual" << endl;
        cout << "  Press 7 for update from old version" << endl;
        cout << "======================================================================================================================" << endl;

        // Simple timed input
        cout << "Select option (1-7): ";

        // Simulate auto-boot with sleep
        DWORD startTime = GetTickCount();
        bool keyPressed = false;
        char choice = '1'; // default

        while (GetTickCount() - startTime < (DWORD)(bootTimeout * 1000)) {
            if (_kbhit()) {
                choice = _getch();
                keyPressed = true;
                break;
            }
            Sleep(100);
        }

        if (!keyPressed) {
            choice = '0' + defaultMode;
        }
        cout << choice << endl;

        if (choice == '1') bootChoice = 1;
        else if (choice == '2') bootChoice = 2;
        else if (choice == '3') bootChoice = 3;
        else if (choice == '4') bootChoice = 4;
        else if (choice == '5') bootChoice = 5;
        else if (choice == '6') Manual();
        else if (choice == '7') Update();
        else bootChoice = defaultMode;
    }

    if (recoveryRequest == "1") {
        bootChoice = 1;
        recoveryRequest = "0";
        recoveryEnv();
        return;
    }

    // Process choice
    if (bootChoice == 1) normalBoot();
    else if (bootChoice == 2) safeModeBoot();
    else if (bootChoice == 3) recoveryEnv();
    else if (bootChoice == 4) diagnosticMode(1);
    else if (bootChoice == 5) diagnosticMode(2);
}

void normalBoot() {
    // Check config
    if (!dirExists(configRoot)) {
        bsod("1a");
        return;
    }
    if (!fileExists(configRoot + "\\BCD")) {
        bsod("14");  // BCD not found
        return;
    }

    // Boot animation
    for (int i = 1; i <= 17; i++) {
        system("cls");
        setColor("0f");
        cout << "======================================================================================================================" << endl;
        cout << "                                                 ARSLANIUS BOOT MANAGER" << endl;
        cout << "======================================================================================================================" << endl;
        cout << "                                                  Loading ARSLANIUS 28" << endl;
        cout << endl;
        cout << "         Build: " << currentBuild << endl;
        cout << "         Kernel: BarOS 23.1" << endl;
        cout << endl;
        cout << "                                                     _____________" << endl;

        switch (i) {
        case 1: cout << "                                                    |.            |" << endl; break;
        case 2: cout << "                                                    |..           |" << endl; break;
        case 3: cout << "                                                    |...          |" << endl; break;
        case 4: cout << "                                                    | ...         |" << endl; break;
        case 5: cout << "                                                    |  ...        |" << endl; break;
        case 6: cout << "                                                    |   ...       |" << endl; break;
        case 7: cout << "                                                    |    ...      |" << endl; break;
        case 8: cout << "                                                    |     ...     |" << endl; break;
        case 9: cout << "                                                    |      ...    |" << endl; break;
        case 10: cout << "                                                    |       ...   |" << endl; break;
        case 11: cout << "                                                    |        ...  |" << endl; break;
        case 12: cout << "                                                    |         ... |" << endl; break;
        case 13: cout << "                                                    |          ...|" << endl; break;
        case 14: cout << "                                                    |           ..|" << endl; break;
        case 15: cout << "                                                    |            .|" << endl; break;
        case 16: cout << "                                                    |             |" << endl; break;
        case 17: cout << "                                                    |.            |" << endl; break;
        }
        cout << "                                                     -------------" << endl;
        Sleep(230);
    }

    logonScreen();
}

void safeModeBoot() {
    system("cls");
    setColor("0f");
    safeMode = 1;
    cout << "======================================================================================================================" << endl;
    cout << "                                                 ARSLANIUS BOOT MANAGER" << endl;
    cout << "======================================================================================================================" << endl;
    Sleep(2000);

    if (!fileExists(configRoot + "\\BCD")) {
        cout << "[ ERROR ] BCD not found" << endl;
        Sleep(3000);
        return;
    }

    cout << "Loaded: \\Settings And System Files\\BCD" << endl;
    if (fileExists(kernelPath)) cout << "Loaded: \\Settings And System Files\\kernel.dll" << endl;
    Sleep(1000);
    if (fileExists(regPath)) cout << "Loaded: \\Settings And System Files\\REG.cfg" << endl;
    Sleep(1000);
    cout << "Loaded: \\Settings And System Files\\systemprofile" << endl;
    Sleep(1000);
    cout << endl << "[ INFO ] Safe Mode enabled." << endl;
    Sleep(1000);

    // Disable services
    DeleteFileA((sysServices + "\\SysPulse.active").c_str());
    DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
    DeleteFileA((sysServices + "\\NetMonitor.active").c_str());

    logonScreen();
}

void diagnosticMode(int mode) {
    diagnostic = mode;
    safeMode = 0;
    rec = 0;

    // Disable some services
    if (mode == 1) {
        DeleteFileA((sysServices + "\\SysPulse.active").c_str());
        DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
        DeleteFileA((sysServices + "\\NetMonitor.active").c_str());
        currentUser = "BarOS SERVICE\\SysPulse";
    }
    else if (mode == 2) {
        DeleteFileA((sysServices + "\\SysPulse.active").c_str());
        DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
        currentUser = "BarOS SERVICE\\NetMonitor";
    }

    interfaceScreen();
}

// =====================================================================
// RECOVERY ENVIRONMENT
// =====================================================================

void recoveryEnv() {
    rec = 1;
    diagnostic = 0;
    safeMode = 0;

    system("cls");
    setColor("1f");
    cout << "======================================================================================================================" << endl;
    cout << "                                             ARSLANIUS RECOVERY ENVIRONMENT" << endl;
    cout << "======================================================================================================================" << endl;
    cout << endl;
    cout << "  [1] Startup Repair        - Fix kernel/registry" << endl;
    cout << "  [2] System Restore        - Go to restore points" << endl;
    cout << "  [3] System Image Recovery - Restore from backup" << endl;
    cout << "  [4] Command Line          - Mini cmd" << endl;
    cout << "  [5] Memory Diagnostic     - Check system memory" << endl;
    cout << "  [6] Return to boot menu" << endl;
    cout << endl;
    cout << "======================================================================================================================" << endl;
    cout << "Select option (1-6): ";

    char choice = _getch();
    cout << choice << endl;

    switch (choice) {
    case '1': startupRepair(); break;
    case '2': restoreMenu(); break;
    case '3': imageRecovery(); break;
    case '4':
        rec = 1;
        interfaceScreen();
        return;
    case '5': memoryDiag(); break;
    case '6': bootMenu(); return;
    default: recoveryEnv(); return;
    }

    recoveryEnv();
}

void startupRepair() {
    cout << endl << "[ WAIT ] Running Startup Repair..." << endl;

    ensureDirectories();

    // Create kernel.dll
    stringstream kernel;
    kernel << "SYSTEM = 350703396" << endl;
    kernel << "SYSTEM ADMINISTRATOR = 734380451" << endl;
    writeFile(kernelPath, kernel.str());

    // Create BCD
    saveBCD();

    // Create REG.cfg
    stringstream reg;
    reg << "OS_NAME = ARSLANIUS 28" << endl;
    reg << "SYSTEM_COLOR=0e" << endl;
    reg << "ADMIN_COLOR=4f" << endl;
    reg << "USER_COLOR=1f" << endl;
    reg << "ENABLE_LUA=1" << endl;
    reg << "REG_VERSION=" << REG_VERSION << endl;
    writeFile(regPath, reg.str());

    writeLog("KERNEL_RESTORED_BY_RECOVERY");

    cout << "[ OK ] Startup Repair completed." << endl;
    system("pause");
}

void restoreMenu() {
    if (!dirExists(restoreRoot)) {
        cout << "[ ERROR ] No restore points directory." << endl;
        system("pause");
        return;
    }

    cout << "Available restore points:" << endl;
    // List directories
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((restoreRoot + "\\*").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                string name = fd.cFileName;
                if (name != "." && name != "..") {
                    cout << "  " << name << endl;
                }
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    cout << endl << "Enter restore point name (or 0 for exit): ";
    string rpSel;
    cin >> rpSel;
    cin.ignore();

    if (rpSel == "0") return;

    string rpPath = restoreRoot + "\\" + rpSel;
    if (!dirExists(rpPath) || !fileExists(rpPath + "\\kernel.dll")) {
        cout << "[ ERROR ] Invalid restore point." << endl;
        system("pause");
        return;
    }

    CopyFileA((rpPath + "\\kernel.dll").c_str(), kernelPath.c_str(), FALSE);
    CopyFileA((rpPath + "\\REG.cfg").c_str(), regPath.c_str(), FALSE);
    CopyFileA((rpPath + "\\system.log").c_str(), logPath.c_str(), FALSE);

    writeLog("RESTORE_APPLIED_FROM_RECOVERY: " + rpSel);

    cout << "[ OK ] System restored from " << rpSel << "." << endl;
    system("pause");
}

void imageRecovery() {
    cout << endl << "===== SYSTEM IMAGE RECOVERY =====" << endl;
    cout << endl;

    string backupPath = rootPath + "\\Backup";
    if (dirExists(backupPath)) {
        cout << "[ FOUND ] Backup directory detected." << endl;
    }
    else {
        cout << "[ ERROR ] No backup image found." << endl;
        system("pause");
        return;
    }

    cout << "Restore from backup? (Y/N): ";
    char confirm;
    cin >> confirm;
    cin.ignore();

    if (toupper(confirm) != 'Y') return;

    cout << "[ WAIT ] Restoring system image..." << endl;

    // Simple copy (recursive copy would need more code)
    string cmd = "xcopy /e /y \"" + backupPath + "\\*\" \"" + rootPath + "\\\"";
    system(cmd.c_str());

    writeLog("IMAGE_RESTORE_EXECUTED");
    cout << "[ OK ] System image restored." << endl;
    system("pause");
}

void memoryDiag() {
    system("cls");
    setColor("1f");
    cout << "======================================================================================================================" << endl;
    cout << "                                             ARSLANIUS MEMORY DIAGNOSTIC" << endl;
    cout << "======================================================================================================================" << endl;
    cout << endl << "Checking for memory problems..." << endl << endl;

    int total = 0;
    for (int i = 1; i <= 10; i++) {
        total += 64;
        cout << "Progress: " << (i * 10) << "% complete..." << endl;
        Sleep(1000);
    }

    cout << endl;
    cout << "[ PASS ] Memory test complete. No errors detected." << endl;
    cout << "Total memory simulated: " << total << " MB" << endl;
    cout << "Status: Healthy" << endl;
    system("pause");
}

// =====================================================================
// LOGON SCREEN
// =====================================================================

void logonScreen() {
    currentUser = "SYSTEM";
    userHome = sysProf;
    logonSuccessOk = 0;

    system("cls");
    setColor("5b");
    cout << "======================================================================================================================" << endl;
    cout << "                                              " << osName << " LOCK SCREEN" << endl;
    cout << "======================================================================================================================" << endl;
    cout << "Status: Protected / Context: " << currentUser << endl;
    cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "COMMANDS: Shutdown, Reboot" << endl;
    cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Users:" << endl;

    if (fileExists(kernelPath)) {
        string kernel = readFile(kernelPath);
        cout << kernel << endl;
    }

    cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
    cout << endl;
    cout << "Username: ";

    string u_in;
    getline(cin, u_in);

    if (u_in == "Shutdown" || u_in == "shutdown") {
        writeLog("SHUTDOWN_FROM_LOGON");
        shutdownScreen();
        return;
    }
    if (u_in == "Reboot" || u_in == "reboot") {
        writeLog("REBOOT_FROM_LOGON");
        rebootScreen();
        return;
    }

    if (u_in.empty()) {
        logonScreen();
        return;
    }

    // Check login attempts
    string failFile = sysServices + "\\fail_" + u_in + ".cnt";
    int loginAttempts = 0;
    if (fileExists(failFile)) {
        ifstream f(failFile);
        f >> loginAttempts;
    }

    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        bsod("9");
        return;
    }

    cout << "Password: ";
    string p_in;
    // Hide password input
    char ch;
    p_in = "";
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (!p_in.empty()) {
                p_in.pop_back();
                cout << "\b \b";
            }
        }
        else {
            p_in += ch;
            cout << '*';
        }
    }
    cout << endl;

    // Verify credentials
    string kernel = readFile(kernelPath);
    istringstream iss(kernel);
    string line;
    bool found = false;
    string storedHash;

    while (getline(iss, line)) {
        line = trim(line);
        if (line.find(u_in + " =") == 0 || line.find(u_in + "=") == 0) {
            size_t eqPos = line.find('=');
            storedHash = trim(line.substr(eqPos + 1));
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "[ ERROR ] User not found." << endl;
        system("pause");
        logonScreen();
        return;
    }

    string inputHash = calculateHash(p_in);

    if (inputHash == storedHash) {
        // Success
        DeleteFileA(failFile.c_str());

        currentUser = u_in;
        if (currentUser == "SYSTEM") {
            userHome = sysProf;
            currentUser = "BarOS AUTHORITY\\SYSTEM";
            regKey = "SYSTEM_COLOR";
        }
        else if (currentUser == "SYSTEM ADMINISTRATOR") {
            userHome = usersRoot + "\\SYSTEM ADMINISTRATOR";
            regKey = "ADMIN_COLOR";
        }
        else {
            userHome = usersRoot + "\\" + currentUser;
            regKey = "USER_COLOR";
        }

        CreateDirectoryA(userHome.c_str(), NULL);
        SetCurrentDirectoryA(userHome.c_str());

        applyColor();
        interfaceScreen();
    }
    else {
        loginAttempts++;
        ofstream f(failFile);
        f << loginAttempts;
        f.close();

        cout << "[ ERROR ] Password incorrect. (Attempt " << loginAttempts << "/" << MAX_LOGIN_ATTEMPTS << ")" << endl;
        if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
            bsod("9");
            return;
        }
        system("pause");
        logonScreen();
    }
}

void applyColor() {
    if (fileExists(regPath)) {
        string reg = readFile(regPath);
        istringstream iss(reg);
        string line;
        while (getline(iss, line)) {
            if (line.find(regKey + "=") != string::npos) {
                size_t eqPos = line.find('=');
                string color = trim(line.substr(eqPos + 1));
                setColor(color);
                break;
            }
        }
    }
}

// =====================================================================
// INTERFACE AND COMMAND LOOP
// =====================================================================

void interfaceScreen() {
    saveBCD();

    // Start services
    if (safeMode == 0 && rec == 0 && diagnostic == 0) {
        cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\SysPulse..." << endl;
        writeFile(sysServices + "\\SysPulse.active", "RUNNING");
        Sleep(1000);
        cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\NetMonitor..." << endl;
        writeFile(sysServices + "\\NetMonitor.active", "RUNNING");
        Sleep(1000);
        cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\TrustedInstaller..." << endl;
        writeFile(sysServices + "\\TrustedInstaller.active", "RUNNING");
        Sleep(1000);
    }

    if (rec == 1) {
        currentUser = "BarOS SERVICE\\TrustedInstaller";
        DeleteFileA((sysServices + "\\SysPulse.active").c_str());
        DeleteFileA((sysServices + "\\NetMonitor.active").c_str());
        userHome = sysServices + "\\TrustedInstaller";
        CreateDirectoryA(userHome.c_str(), NULL);
    }
    if (diagnostic == 1) {
        currentUser = "BarOS SERVICE\\SysPulse";
        DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
        DeleteFileA((sysServices + "\\NetMonitor.active").c_str());
        userHome = sysServices + "\\SysPulse";
        CreateDirectoryA(userHome.c_str(), NULL);
    }
    if (diagnostic == 2) {
        currentUser = "BarOS SERVICE\\NetMonitor";
        DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
        DeleteFileA((sysServices + "\\SysPulse.active").c_str());
        userHome = sysServices + "\\NetMonitor";
        CreateDirectoryA(userHome.c_str(), NULL);
    }

    SetCurrentDirectoryA(userHome.c_str());

    system("cls");
    if (diagnostic == 2 || diagnostic == 1 || rec == 1) setColor("0e");
    if (safeMode == 1) setColor("07");

    cout << osName << " [Build " << currentBuild << "] - Session: " << currentUser;
    cout << " (SAFE MODE: " << safeMode << ")" << endl;
    cout << "Profile: " << userHome << endl;
    cout << "Have ideas or found a bug ? Visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/discussions" << endl;
    cout << "Or: https://t.me/+8FQ20tOaKI5lNGMy" << endl;
    cout << "----------------------------------------------------------------------------------------------------------------------" << endl;

    // Check autorun
    if (safeMode == 0 && rec == 0 && diagnostic == 0) {
        string mailFile = userHome + "\\mail.txt";
        if (fileExists(mailFile)) {
            cout << "[ MAIL ] You have unread messages! Type \"mail-read\"." << endl;
        }
        string autorunFile = userHome + "\\autorun.txt";
        if (fileExists(autorunFile)) {
            string cmdLine = trim(readFile(autorunFile));
            if (!cmdLine.empty()) {
                cout << "[ AUTO ] Starting: " << cmdLine << "..." << endl;
                core(cmdLine);
            }
        }
    }

    cmdLoop();
}

void cmdLoop() {
    while (true) {
        // Check logout
        if (logoutRequest) {
            system("cls");
            setColor("5b");
            cout << "\n\n                                                      Logout...\n\n";
            Sleep(2000);

            if (acpiRequest == 1) shutdownScreen();
            else if (acpiRequest == 2) rebootScreen();
            else {
                currentUser = "SYSTEM";
                userHome = sysProf;
                logonSuccessOk = 0;
                logoutRequest = 0;
                waitMode = 0;
                initctrl = 0;
                logonScreen();
                return;
            }
        }

        if (waitMode) {
            system("cls");
            setColor("0f");
            cout << "\n\n                                                  " << osName << "\n\n";
            system("pause");
            waitMode = 0;
        }
        // services
        if (safeMode == 0 && rec == 0 && diagnostic == 0) {
            if (fileExists(sysServices + "\\TrustedInstaller.active")) {
                string ins_err = "0";
                if (!fileExists(kernelPath)) ins_err = "1";
                if (!fileExists(regPath)) ins_err = "1";
                if (!fileExists(configRoot + "\\BCD")) ins_err = "1";

                if (ins_err == "1") {
                    if (!fileExists(kernelPath)) {
                        stringstream kernel_ins;
                        kernel_ins << "SYSTEM = 350703396" << endl;
                        kernel_ins << "SYSTEM ADMINISTRATOR = 734380451" << endl;
                        writeFile(kernelPath, kernel_ins.str());
                    }
                    if (!fileExists(configRoot + "\\BCD")) {
                        saveBCD();
                    }
                    if (!fileExists(regPath)) {
                        stringstream reg_ins;
                        reg_ins << "OS_NAME = ARSLANIUS 28" << endl;
                        reg_ins << "SYSTEM_COLOR=0e" << endl;
                        reg_ins << "ADMIN_COLOR=4f" << endl;
                        reg_ins << "USER_COLOR=1f" << endl;
                        reg_ins << "ENABLE_LUA=1" << endl;
                        reg_ins << "REG_VERSION=" << REG_VERSION << endl;

                    }
                }
            }
            if (fileExists(sysServices + "\\SysPulse.active")) {
                int PulseCheck = rand() % 10;
                if (PulseCheck == 5) {
                    writeLog("BarOS SERVICE\\SYSPULSE: System Health OK");
                }
                if (fileExists(sysServices + "\\NetMonitor.active")) {
                    int NetCheck = rand() % 10;
                    if (NetCheck == 5) {
                        string host = "github.com";
                        string NetCheckS = "ping -n 1 " + host + " >nul 2>&1";
                        int NetCheckResult = system(NetCheckS.c_str());
                        if (NetCheckResult != 0) {
                            cout << "BarOS SERVICE\\NETMONITOR: OFFLINE" << endl;
                            writeLog("BarOS SERVICE\\NETMONITOR: OFFLINE");
                        }
                        else {
                            cout << "BarOS SERVICE\\NETMONITOR: ONLINE" << endl;
                            writeLog("BarOS SERVICE\\NETMONITOR: ONLINE");
                        }
                    }
                }
            }
        }
        // Prompt
        cout << currentUser << "@ARSLANIUS> ";
        string cmd;
        getline(cin, cmd);
        cmd = trim(cmd);

        if (cmd.empty()) continue;

        core(cmd);
    }
}

    void core(const string & cmd) {
    vector<string> parts = split(cmd, ' ');
    if (parts.empty()) return;

    string firstWord = parts[0];
    string rest;
    if (parts.size() > 1) {
        rest = parts[1];
        for (size_t i = 2; i < parts.size(); i++) {
            rest += " " + parts[i];
        }
    }

    string ex_c = cmd;
    string f_w = firstWord;
    string t_c = rest;

    // Handle sudo
    if (f_w == "sudo" || f_w == "Sudo") {
        if (t_c.empty()) {
            cout << "Usage: sudo [command]" << endl;
            return;
        }
        if (currentUser == "BarOS AUTHORITY\\SYSTEM" ||
            currentUser == "SYSTEM ADMINISTRATOR" ||
            currentUser == "BarOS SERVICE\\TrustedInstaller" ||
            currentUser == "BarOS SERVICE\\SysPulse" ||
            currentUser == "BarOS SERVICE\\NetMonitor") {
            ex_c = t_c;
            core(ex_c);
            return;
        }
        cout << "Enter ADMIN password: ";
        string a_p;
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
                if (!a_p.empty()) a_p.pop_back();
            }
            else {
                a_p += ch;
                cout << '*';
            }
        }
        cout << endl;

        if (checkHash(a_p, EXPECTED_ADMIN_HASH)) {
            writeLog("SUDO_EXEC: " + t_c + " BY " + currentUser);
            ex_c = t_c;
            core(ex_c);
        }
        else {
            cout << "[ ERROR ] Access denied." << endl;
        }
        return;
    }

    // Command routing
    transform(ex_c.begin(), ex_c.end(), ex_c.begin(), ::tolower);

    if (ex_c == "help" || ex_c == "?") {
        cout << "Apps: Notepad, Calc, taskmgr, edit, install, regedit, ArsStore, sysinfo" << endl;
        cout << "System: Help, Lock, lockmenu, sudo, cls, Shutdown, ver, whoami, reboot, clean, events, restore-point, restore, echo, passwd, backup, backup-restore, ls, wait_mode, cd, cat, ren, mkdir, touch, cp, mv, autorun" << endl;
        cout << "Admin: adduser, deluser, alert, Guest, report, reset, reboot_to_recovery, set, bsod, rm, netstat, ipconfig, tracert, nslookup, arp, route, loader_error, bcdboot, bcdedit" << endl;
    }
    else if (ex_c == "cls") interfaceScreen();
    else if (ex_c == "ver") cout << osName << " [Build " << currentBuild << "]" << endl;
    else if (ex_c == "whoami") {
        cout << "Current User: " << currentUser << endl;
        cout << "Path: " << userHome << endl;
    }
    else if (ex_c == "ls") system("dir /w");
    else if (ex_c == "cd") {
        string newPath;
        cout << "Enter path: ";
        getline(cin, newPath);
        newPath = trim(newPath);
        if (!newPath.empty()) {
            if (SetCurrentDirectoryA(newPath.c_str())) {
                userHome = newPath;
                cout << "[ OK ] Directory changed to " << userHome << endl;
            }
            else {
                cout << "[ ERROR ] Invalid path." << endl;
            }
        }
    }
    else if (ex_c == "cat") {
        string targetFile;
        cout << "Enter filename: ";
        getline(cin, targetFile);
        targetFile = trim(targetFile);
        if (fileExists(targetFile)) {
            cout << readFile(targetFile) << endl;
        }
        else {
            cout << "[ ERROR ] File not found." << endl;
        }
    }
    else if (ex_c == "mkdir") {
        string folderName;
        cout << "Enter folder name: ";
        getline(cin, folderName);
        folderName = trim(folderName);
        if (CreateDirectoryA(folderName.c_str(), NULL)) {
            cout << "[ OK ] Folder created." << endl;
        }
        else {
            cout << "[ ERROR ] Failed to create folder." << endl;
        }
    }
    else if (ex_c == "touch") {
        string touchFile;
        cout << "Enter filename: ";
        getline(cin, touchFile);
        touchFile = trim(touchFile);
        if (fileExists(touchFile)) {
            cout << "[ ERROR ] File exists. Overwrite? (Y/N): ";
            char c = _getch();
            cout << c << endl;
            if (toupper(c) == 'Y') writeFile(touchFile, "");
        }
        else {
            writeFile(touchFile, "");
            cout << "[ OK ] File created." << endl;
        }
    }
    else if (ex_c == "cp") {
        string src, dst;
        cout << "Source file: ";
        getline(cin, src);
        src = trim(src);
        cout << "Destination file: ";
        getline(cin, dst);
        dst = trim(dst);
        if (CopyFileA(src.c_str(), dst.c_str(), FALSE)) {
            cout << "[ OK ] Copied." << endl;
        }
        else {
            cout << "[ ERROR ] Copy failed." << endl;
        }
    }
    else if (ex_c == "mv") {
        string src, dst;
        cout << "Source file: ";
        getline(cin, src);
        src = trim(src);
        cout << "Destination file: ";
        getline(cin, dst);
        dst = trim(dst);
        if (MoveFileA(src.c_str(), dst.c_str())) {
            cout << "[ OK ] Moved." << endl;
        }
        else {
            cout << "[ ERROR ] Move failed." << endl;
        }
    }
    else if (ex_c == "rm") {
        string targetFile;
        cout << "File to delete: ";
        getline(cin, targetFile);
        targetFile = trim(targetFile);
        if (DeleteFileA(targetFile.c_str())) {
            cout << "[ OK ] Deleted." << endl;
        }
        else {
            cout << "[ ERROR ] Delete failed." << endl;
        }
    }
    else if (ex_c == "ren") {
        string oldName, newName;
        cout << "Old name: ";
        getline(cin, oldName);
        oldName = trim(oldName);
        cout << "New name: ";
        getline(cin, newName);
        newName = trim(newName);
        if (rename(oldName.c_str(), newName.c_str()) == 0) {
            cout << "[ OK ] Renamed." << endl;
        }
        else {
            cout << "[ ERROR ] Rename failed." << endl;
        }
    }
    else if (ex_c == "echo") {
        string text;
        cout << "Enter text: ";
        getline(cin, text);
        cout << text << endl;
    }
    else if (ex_c == "shutdown") {
        logoutRequest = 1;
        acpiRequest = 1;
    }
    else if (ex_c == "reboot") {
        logoutRequest = 1;
        acpiRequest = 2;
    }
    else if (ex_c == "lock") {
        logoutRequest = 1;
        acpiRequest = 0;
    }
    else if (ex_c == "bsod") bsod("666");
    else if (ex_c == "wait_mode") waitMode = 1;
    else if (ex_c == "lockmenu") {
        // Simplified lock menu
        cout << "1. Logout" << endl;
        cout << "2. Passwd" << endl;
        cout << "3. Shutdown" << endl;
        cout << "4. Reboot" << endl;
        cout << "5. Help" << endl;
        cout << "6. Return" << endl;
        cout << "Select: ";
        char ch = _getch();
        cout << ch << endl;
        switch (ch) {
        case '1': logoutRequest = 1; acpiRequest = 0; break;
        case '2': core("passwd"); break;
        case '3': logoutRequest = 1; acpiRequest = 1; break;
        case '4': logoutRequest = 1; acpiRequest = 2; break;
        case '5': core("help"); break;
        case '6': return; 
        }
    }
    else if (ex_c == "adduser") {
        string nu, np;
        cout << "Username: ";
        getline(cin, nu);
        nu = trim(nu);
        if (nu == "SYSTEM" || nu == "BarOS AUTHORITY\\SYSTEM" || nu == "SYSTEM ADMINISTRATOR") {
            cout << "[ ERROR ] Reserved username." << endl;
            return;
        }
        string user_exists = readFile( kernelPath );
        if (user_exists.find(nu + " =") != string::npos) {
            cout << "[ERROR] User arleady exists." << endl;
            return;
        }
        cout << "Password: ";
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
                if (!np.empty()) np.pop_back();
            }
            else {
                np += ch;
                cout << '*';
            }
        }
        cout << endl;

        string hash = calculateHash(np);
        appendFile(kernelPath, nu + " = " + hash + "\n");
        CreateDirectoryA((usersRoot + "\\" + nu).c_str(), NULL);
        writeLog("NEW_USER_CREATED: " + nu);
        cout << "[ OK ] User created." << endl;
    }
    else if (ex_c == "deluser") {
        string du;
        cout << "Enter username: ";
        getline(cin, du);
        du = trim(du);
        if (du == "SYSTEM" || du == "SYSTEM ADMINISTRATOR") {
            cout << "[ ERROR ] Restricted." << endl;
            return;
        }

        string kernel = readFile(kernelPath);
        istringstream iss(kernel);
        string line, newKernel;
        while (getline(iss, line)) {
            if (line.find(du + " =") != 0 && line.find(du + "=") != 0) {
                newKernel += line + "\n";
            }
        }
        writeFile(kernelPath, newKernel);
        RemoveDirectoryA((usersRoot + "\\" + du).c_str());
        cout << "[ OK ] User removed." << endl;
    }
    else if (ex_c == "passwd") {
        if (currentUser == "BarOS AUTHORITY\\SYSTEM" ||
            currentUser == "SYSTEM ADMINISTRATOR" ||
            currentUser.find("BarOS SERVICE") == 0) {
            cout << "Cannot change this account's password." << endl;
            return;
        }
        string old, new1, new2;
        cout << "Current password: ";
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') { if (!old.empty()) old.pop_back(); }
            else { old += ch; cout << '*'; }
        }
        cout << endl;

        cout << "New password: ";
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') { if (!new1.empty()) new1.pop_back(); }
            else { new1 += ch; cout << '*'; }
        }
        cout << endl;

        cout << "Confirm: ";
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') { if (!new2.empty()) new2.pop_back(); }
            else { new2 += ch; cout << '*'; }
        }
        cout << endl;

        if (new1 != new2) {
            cout << "Passwords do not match." << endl;
            return;
        }

        // Update kernel
        string kernel = readFile(kernelPath);
        string displayName = currentUser;
        if (displayName == "BarOS AUTHORITY\\SYSTEM") displayName = "SYSTEM";

        istringstream iss(kernel);
        string line, newKernel;
        while (getline(iss, line)) {
            if (line.find(displayName + " =") == 0 || line.find(displayName + "=") == 0) {
                newKernel += displayName + " = " + calculateHash(new1) + "\n";
            }
            else {
                newKernel += line + "\n";
            }
        }
        writeFile(kernelPath, newKernel);
        cout << "[ OK ] Password changed." << endl;
    }
    else if (ex_c == "restore-point") {
        ensureDirectories();
        if (!dirExists(restoreRoot)) CreateDirectoryA(restoreRoot.c_str(), NULL);

        string rpName = "RP_" + getCurrentDateTime();
        for (auto& c : rpName) if (c == ' ' || c == ':') c = '_';

        string rpDir = restoreRoot + "\\" + rpName;
        CreateDirectoryA(rpDir.c_str(), NULL);
        CopyFileA(kernelPath.c_str(), (rpDir + "\\kernel.dll").c_str(), FALSE);
        CopyFileA(regPath.c_str(), (rpDir + "\\REG.cfg").c_str(), FALSE);
        CopyFileA(logPath.c_str(), (rpDir + "\\system.log").c_str(), FALSE);

        writeLog("RESTORE_POINT_CREATED: " + rpName);
        cout << "[ OK ] Restore point created: " << rpName << endl;
    }
    else if (ex_c == "restore") {
        restoreMenu();
    }
    else if (ex_c == "backup") {
        string backupDir = rootPath + "\\Backup";
        if (dirExists(backupDir)) {
            string cmd = "rd /s /q \"" + backupDir + "\"";
            system(cmd.c_str());
        }
        string cmd = "xcopy /e /y \"" + rootPath + "\\*\" \"" + backupDir + "\\\" /EXCLUDE:Backup";
        system(cmd.c_str());
        writeLog("SYSTEM_BACKUP_EXECUTED");
        cout << "[ OK ] Backup created." << endl;
    }
    else if (ex_c == "backup-restore") {
        imageRecovery();
    }
    else if (ex_c == "sfc") {
        cout << "[ SYSTEM ] Beginning system file check..." << endl;
        bool errors = false;
        if (!fileExists(configRoot + "\\BCD")) { cout << "[ FAIL ] BCD MISSING" << endl; errors = true; }
        else cout << "[ OK ] BCD" << endl;
        if (!fileExists(kernelPath)) { cout << "[ FAIL ] kernel.dll MISSING" << endl; errors = true; }
        else cout << "[ OK ] kernel.dll" << endl;
        if (!fileExists(regPath)) { cout << "[ FAIL ] REG.cfg MISSING" << endl; errors = true; }
        else cout << "[ OK ] REG.cfg" << endl;

        if (errors) {
            cout << "[ WARN ] Integrity violations found. Repair? (Y/N): ";
            char c = _getch();
            cout << c << endl;
            if (toupper(c) == 'Y') startupRepair();
        }
        else {
            cout << "[ DONE ] All system files healthy." << endl;
        }
    }
    else if (ex_c == "sysinfo") {
        cout << "Current User: " << currentUser << endl;
        cout << "Home: " << userHome << endl;
        cout << "OS: " << osName << " Build " << currentBuild << endl;
        cout << "Safe Mode: " << safeMode << endl;
    }
    else if (ex_c == "reboot_to_recovery") {
        recoveryRequest = "1";
        logoutRequest = 1;
        acpiRequest = 3;
    }
    else if (ex_c == "edit") {
        string ef, et;
        cout << "File: ";
        getline(cin, ef);
        ef = trim(ef);
        cout << "Text: ";
        getline(cin, et);
        appendFile(ef, et + "\n");
    }
    else if (ex_c == "bcdedit") {
        cout << "Enter new timeout (or Enter to skip): ";
        string input;
        getline(cin, input);
        input = trim(input);
        if (!input.empty()) bootTimeout = stoi(input);

        cout << "Enter Default Mode (ex: 1): ";
        getline(cin, input);
        input = trim(input);
        if (!input.empty()) defaultMode = stoi(input);

        saveBCD();
        cout << "[ DONE ] Configuration saved." << endl;
    }
    else if (ex_c == "bcdboot") {
        saveBCD();
        cout << "[ OK ] BCD created." << endl;
    }
    else if (ex_c == "regedit") {
        cout << "Enter new OS Name (or Enter to skip): ";
        string input;
        getline(cin, input);
        input = trim(input);
        if (!input.empty()) osName = input;

        stringstream reg;
        reg << "OS_NAME = " << osName << endl;
        reg << "SYSTEM_COLOR=0e" << endl;
        reg << "ADMIN_COLOR=4f" << endl;
        reg << "USER_COLOR=1f" << endl;
        reg << "ENABLE_LUA=1" << endl;
        reg << "REG_VERSION=" << REG_VERSION << endl;
        writeFile(regPath, reg.str());
        cout << "[ DONE ] Registry updated." << endl;
    }
    else if (ex_c == "taskmgr") {
        int cpu = rand() % 15 + 1;
        cout << "[ CORE ] BarOS 23.1 (CPU: " << cpu << "%)" << endl;
    }
    else if (ex_c == "events") {
        cout << readFile(logPath) << endl;
    }
    else if (ex_c == "clean") {
        system("for /d %i in (NewFolder*) do rd /s /q \"%i\" 2>nul");
        cout << "[ OK ] Cleaned." << endl;
    }
    else if (ex_c == "alt+f4") {
        cout << "ARSLANIUS is not Windows :)" << endl;
    }
    else if (ex_c == "notepad") {
        system("notepad.exe");
    }
    else if (ex_c == "calc") {
        system("calc.exe");
    }
    else if (ex_c == "ping") {
        cout << "Enter host: ";
        string host;
        getline(cin, host);
        host = trim(host);
        string cmd = "ping -n 4 " + host;
        system(cmd.c_str());
    }
    else if (ex_c == "ipconfig") {
        system("ipconfig /all");
    }
    else if (ex_c == "netstat") {
        system("netstat -an");
    }
    else {
        cout << "\"" << ex_c << "\" is not recognized." << endl;
    }
}

void shutdownScreen() {
    system("cls");
    setColor("9f");
    cout << "\n\n\n\n\n\n\n\n\n                                               SHUTTING DOWN\n\n\n\n\n\n\n\n\n";
    cout << "                                               " << osName << endl;
    Sleep(3000);
    DeleteFileA((sysServices + "\\SysPulse.active").c_str());
    DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
    DeleteFileA((sysServices + "\\NetMonitor.active").c_str());
    exit(0);
}

void rebootScreen() {
    system("cls");
    setColor("9f");
    cout << "\n\n\n\n\n\n\n\n\n                                               REBOOTING\n\n\n\n\n\n\n\n\n";
    cout << "                                               " << osName << endl;
    Sleep(3000);
    DeleteFileA((sysServices + "\\SysPulse.active").c_str());
    DeleteFileA((sysServices + "\\TrustedInstaller.active").c_str());
    DeleteFileA((sysServices + "\\NetMonitor.active").c_str());
    bootMenu();
}

// =====================================================================
// MAIN
// =====================================================================

int main() {
    SetConsoleTitleA("ARSLANIUS 28");

    initPaths();
    ensureDirectories();


    if (!fileExists(kernelPath)) {
        bsod("1");
    };
    if (!fileExists(regPath)) {
        bsod("4");
    };
    if (!fileExists(configRoot + "\\BCD")) {
        bsod("14");
    };
    if (!dirExists(configRoot)) {
        bsod("1a");
    };

    bootMenu();

    return 0;
};