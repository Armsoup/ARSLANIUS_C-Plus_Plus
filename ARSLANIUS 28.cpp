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
#include <mmsystem.h>
#include <algorithm>
#include <cctype>
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <filesystem>
#include "resource.h"

#pragma comment(lib, "winmm.lib")

using namespace std;

// =====================================================================
// CONSTANTS
// =====================================================================
const string CURRENT_BUILD = "58.1.1";
const string REG_VERSION = "28";
const string EXPECTED_SYSTEM_HASH = "350703396";
const string EXPECTED_ADMIN_HASH = "734380451";
const string OS_NAME_DEFAULT = "ARSLANIUS 28";
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

DWORD64 bootTimeout = BOOT_TIMEOUT_DEFAULT;
DWORD64 logoutTimeout = 60;
int defaultMode = DEFAULT_MODE_DEFAULT;
int bootChoice = 0;
int safeMode = 0;
int rec = 0;
int diagnostic = 0;
int lastSuccessfulMode = 0;
int acpiRequest = 0;
int enableLua = 1;
int REG_VERSION_FOUND = 0;
int setup = 0;
int system_hash_found = 0;
int admin_hash_found = 0;
int sudo_command = 0;
int lockdown = 0;
int loginAttempts = 0;
DWORD64* ptr = nullptr;
string systemColor = "0e";
string adminColor = "4f";
string userColor = "1f";
string recoveryRequest = "0";
string p_in;
string failFile;
string u_in;

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
void check_AUTHORITY();
void check_kernel();
void check_registry();
void check_BCD();
void installapp(const string& app_id);
void core(const string& cmd);
bool checkHash(const string& input, const string& expectedHash);
string calculateHash(const string& input);
void bsod(const string& code);
void arslogon(const string& authority);
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
void SetConsoleWidthOnly(int newWidth);
namespace fs = std::filesystem;

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
	return fs::exists(path) && fs::is_regular_file(path);
}

bool dirExists(const string& path) {
	return fs::exists(path) && fs::is_directory(path);
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
		log << "[" << getCurrentDateTime() << "]" << " " << message << endl;
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
		f.close();
	}
	else {
		bsod("UNKNOWN");
	}
}

void appendFile(const string& path, const string& content) {
	ofstream f(path, ios::app);
	if (f.is_open()) {
		f << content;
	}
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

void pause() {
	cout << "Press any key to continue...";
	_getch();
	cout << endl;
}

void clearScreen() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordScreen = { 0, 0 };
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);

	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);

	SetConsoleCursorPosition(hConsole, coordScreen);
}

void setColor(const string& colorCode) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	try {
		int color = stoi(colorCode, nullptr, 16);
		SetConsoleTextAttribute(hConsole, (WORD)color);
		clearScreen();
	}
	catch (...) {
		SetConsoleTextAttribute(hConsole, 7);
	}
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

void installapp(const string& app_id) {
	if (app_id == "1") {
		string appPath = programsRoot + "\\Scanner.bat";
		stringstream app_install;
		app_install << "@echo off" << endl;
		app_install << "echo Scanning %cd%..." << endl;
		app_install << "dir /s /b \"%cd%\" > \"%cd%\\scan_%date%.txt\"" << endl;
		app_install << "echo Scan complete. Saved to scan_%date%.txt" << endl;
		app_install << "pause" << endl;
		app_install << "exit /b" << endl;
		writeFile(appPath, app_install.str());
	}
	if (app_id == "2") {
		string appPath = programsRoot + "\\Notes.bat";
		stringstream app_install;
		app_install << "@echo off" << endl;
		app_install << "set /p txt=Enter text: " << endl;
		app_install << "echo %txt% >> \"%cd%\\notes.txt\"" << endl;
		app_install << "echo Note saved." << endl;
		app_install << "pause" << endl;
		app_install << "exit /b" << endl;
		writeFile(appPath, app_install.str());
	}
	if (app_id == "3") {
		string appPath = programsRoot + "\\Calc.bat";
		stringstream app_install;
		app_install << "@echo off" << endl;
		app_install << "title Calculator" << endl;
		app_install << "color 2f" << endl;
		app_install << ":start" << endl;
		app_install << "set /p sum=Please enter the question:" << endl;
		app_install << "set /a ans=%sum%" << endl;
		app_install << "echo The Answer=%ans%" << endl;
		app_install << "pause" << endl;
		app_install << "cls" << endl;
		app_install << "goto start" << endl;
		writeFile(appPath, app_install.str());
	}
}

void check_kernel() {
	vector<string> linesToFind = {
		"SYSTEM =",
		"SYSTEM ADMINISTRATOR ="
	};
	string content = readFile(kernelPath);
	string lowerContent = content;
	transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
	for (const string& searchFor : linesToFind) {
		string lowerSearch = searchFor;
		transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
		if (lowerContent.find(lowerSearch) != string::npos) {
		}
		else {
			bsod("8");
		}
	}
}

void SetConsoleWidthOnly(int newWidth) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

	short currentHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	COORD newBufferSize = { (short)newWidth, csbi.dwSize.Y };
	SetConsoleScreenBufferSize(hOut, newBufferSize);

	SMALL_RECT rect;
	rect.Left = csbi.srWindow.Left;
	rect.Top = csbi.srWindow.Top;
	rect.Right = csbi.srWindow.Left + (short)newWidth - 1;
	rect.Bottom = csbi.srWindow.Top + currentHeight - 1;

	SetConsoleWindowInfo(hOut, TRUE, &rect);
}

void check_registry() {
	vector<string> linesToFind = {
		"OS_NAME=",
		"SYSTEM_COLOR=",
		"ADMIN_COLOR=",
		"USER_COLOR=",
		"ENABLE_LUA=",
		"LOCKDOWN=",
		"SETUP=",
		"REG_VERSION="
	};
	string content = readFile(regPath);
	string lowerContent = content;
	transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
	for (const string& searchFor : linesToFind) {
		string lowerSearch = searchFor;
		transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
		if (lowerContent.find(lowerSearch) != string::npos) {
		}
		else {
			bsod("7");
		}
	}
}
void check_BCD() {
	vector<string> linesToFind = {
		"BOOT_TIMEOUT=",
		"DEFAULT_MODE=",
	};
	string content = readFile(configRoot + "\\BCD");
	string lowerContent = content;
	transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
	for (const string& searchFor : linesToFind) {
		string lowerSearch = searchFor;
		transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
		if (lowerContent.find(lowerSearch) != string::npos) {
		}
		else {
			bsod("13");
		}
	}
}

void check_AUTHORITY() {
	string kernel = readFile(kernelPath);
	string lowerKernel = kernel;
	transform(lowerKernel.begin(), lowerKernel.end(), lowerKernel.begin(), ::tolower);

	if (lowerKernel.find("baros authority") != string::npos) {
		bsod("5");
	}
}

void loadRegistry() {
	if (!fileExists(regPath)) return;

	ifstream reg(regPath);
	string line;
	while (getline(reg, line)) {
		size_t sep = line.find('=');
		if (sep == string::npos) continue;
		string key = line.substr(0, sep);
		string value = line.substr(sep + 1);

		if (value.empty()) continue;

		try {
			if (key == "OS_NAME") osName = value;
			else if (key == "ENABLE_LUA") enableLua = stoi(value);
			else if (key == "LOCKDOWN") lockdown = stoi(value);
			else if (key == "SYSTEM_COLOR") systemColor = stoi(value);
			else if (key == "ADMIN_COLOR") adminColor = stoi(value);
			else if (key == "USER_COLOR") userColor = stoi(value);
			else if (key == "REG_VERSION") REG_VERSION_FOUND = stoi(value);
			else if (key == "SETUP") setup = stoi(value);
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
	clearScreen();
	cout << "======================================================================================================================" << endl;
	cout << "                                         " << osName << " - USER MANUAL" << endl;
	cout << "======================================================================================================================" << endl;
	cout << endl;
	cout << "[ QUICK START ]" << endl;
	cout << "  1. First run? Press R at BSoD then press 1 [Startup Repair]" << endl;
	cout << "  2. complete OOBE (Out-Of-Box Experience) to create your user account." << endl;
	cout << "  3. Login as your user." << endl;
	cout << "  4. Type help for a list of commands." << endl;
	cout << endl;
	cout << "[ ARSLANIUS BOOT MANAGER ]" << endl;
	cout << "  - Press 1-7 to select mode" << endl;
	cout << "  - Auto-boot in " << bootTimeout << " seconds" << endl;
	cout << endl;
	cout << "[ COMMANDS ]" << endl;
	cout << "  System: help, lock, rebootemer or arslogon -emergency reboot, cls, ver, whoami, reboot, shutdown, lockmenu [ctrl alt del]" << endl;
	cout << "  Files: ls, cd, cat, ren, mkdir, touch, edit, cp, mv, rm" << endl;
	cout << "  Admin: adduser, deluser, passwd, regedit, bcdedit, bcdboot, reset" << endl;
	cout << "  Network: ping, netstat, ipconfig, tracert, nslookup, arp, route" << endl;
	cout << "  Recovery: backup, backup-restore, restore-point, restore, sfc, events" << endl;
	cout << "  Fun: bsod, Notepad, wait_mode, echo" << endl;
	cout << endl;
	cout << "[ RECOVERY ENVIRONMENT ]" << endl;
	cout << "  1 [Startup Repair] - recreates all files" << endl;
	cout << "  4 [Mini cmd] - command line with recovery tools" << endl;
	cout << "  2 or 3 - restore from restore points or backup" << endl;
	cout << "======================================================================================================================" << endl;
	pause();
	bootMenu();
}

void Update() {
	cout << "If you have files from an older version, place them in the root folder along with the .cmd file and press Y." << endl;
	cout << "Y/N: ";
	char c = _getch();
	cout << c << endl;

	if (toupper(c) == 'Y') {
		if (!fileExists(kernelPath)) {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] kernel.dll not found." << endl;
			pause();
			bsod("1");
		}
		saveBCD();

		stringstream reg_update;
		reg_update << "OS_NAME=ARSLANIUS 28" << endl;
		reg_update << "SYSTEM_COLOR=0e" << endl;
		reg_update << "ADMIN_COLOR=4f" << endl;
		reg_update << "USER_COLOR=1f" << endl;
		reg_update << "ENABLE_LUA=1" << endl;
		reg_update << "LOCKDOWN=0" << endl;
		reg_update << "SETUP=0" << endl;
		reg_update << "REG_VERSION=" << REG_VERSION << endl;
		writeFile(regPath, reg_update.str());
		bootMenu();
	}
	bootMenu();
}

void ensureDirectories() {
	fs::create_directories(configRoot);
	fs::create_directories(sysServices);
	fs::create_directories(usersRoot);
	fs::create_directories(programsRoot);
}

// =====================================================================
// BSOD SCREENS
// =====================================================================

void bsod(const string& code) {
	clearScreen();
	std::thread soundThread([]() { Beep(750, 1710); });
	soundThread.detach();

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
		pause();
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
		pause();
		recoveryEnv();
	}
	else if (code == "2") {
		setColor("17");
		cout << "*** STOP: 0x00000002 [0xc00000002, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
		cout << endl;
		cout << "SYSTEM_ACCOUNT_HASH_MISMATCH - Someone's been playing with kernel.dll in Notepad, huh?" << endl;
		cout << "Please reinstall or run Startup Repair." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Expected system hash: " << EXPECTED_SYSTEM_HASH << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "3") {
		setColor("17");
		cout << "*** STOP: 0x00000003 [0xc00000003, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\REG.cfg" << endl;
		cout << endl;
		cout << "REGISTRY_VERSION_MISMATCH - The version specified in the REG.cfg file is incorrect." << endl;
		cout << "Please update it to continue working." << endl;
		cout << endl;
		cout << "Expected version: " << REG_VERSION << endl;
		cout << "Found version: " << REG_VERSION_FOUND << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
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
		pause();
		recoveryEnv();
	}
	else if (code == "5") {
		setColor("04");
		cout << "*** STOP: 0x00000005 [0xc00000005, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
		cout << endl;
		cout << "RESERVED_USERNAME_DETECTED - Security violation!" << endl;
		cout << "Someone tried to create 'BarOS AUTHORITY' in kernel.dll." << endl;
		cout << "That's like printing your own \"100% REAL OFFICIAL\" dollar bill." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Reserved username found in kernel space." << endl;
		cout << "*** System halted to prevent unauthorized access." << endl;
		cout << endl;
		cout << "If you call a support specialist, tell him this information so that he can laugh." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "6") {
		setColor("17");
		cout << "*** STOP: 0x00000006 [0xc00000006, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
		cout << endl;
		cout << "ADMIN_ACCOUNT_HASH_MISMATCH - Someone tried to give themselves admin privileges the hard way." << endl;
		cout << "The ADMINISTRATOR hash doesn't match. Spoiler: it didn't work." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Expected admin hash: " << EXPECTED_ADMIN_HASH << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "7") {
		setColor("17");
		cout << "*** STOP: 0x00000007 [0xc00000007, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\REG.cfg" << endl;
		cout << endl;
		cout << "BAD_SYSTEM_CONFIG_INFO - The registry is missing required entries." << endl;
		cout << "OS_NAME, SYSTEM_COLOR, ADMIN_COLOR, USER_COLOR, ENABLE_LUA, or REG_VERSION" << endl;
		cout << "is missing. Run Startup Repair to restore the registry." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Registry file found but incomplete." << endl;
		cout << "*** Expected entries: OS_NAME, SYSTEM_COLOR, ADMIN_COLOR, USER_COLOR," << endl;
		cout << "*** ENABLE_LUA, REG_VERSION" << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "8") {
		setColor("17");
		cout << "*** STOP: 0x00000008 [0xc00000008, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
		cout << endl;
		cout << "KERNEL_INCOMPLETE - The kernel is missing required SYSTEM or ADMINISTRATOR entries." << endl;
		cout << "Someone deleted important lines from kernel.dll. Probably with Notepad." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Missing: SYSTEM or SYSTEM ADMINISTRATOR account" << endl;
		cout << "*** Kernel file found but incomplete." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "9") {
		setColor("17");
		cout << "*** STOP: 0x00000009 [0xc00000009, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\ARSLANIUS 28.exe" << endl;
		cout << endl;
		cout << "LOGON_ATTACK_DETECTED - Too many failed login attempts." << endl;
		cout << "A brute force attack may be in progress." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Failed attempts: 10" << endl;
		cout << "*** System halted to prevent unauthorized access." << endl;
		cout << "*** Run Recovery Environment to investigate." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "10") {
		setColor("17");
		cout << "*** STOP: 0x00000010 [0xc00000010, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\kernel.dll" << endl;
		cout << endl;
		cout << "The kernel is full, run startup repair to reset it." << endl;
		cout << "Looks like someone needed TOO many accounts." << endl;
		cout << "Run Startup Repair to restore kernel." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		std::cout << "*** Size: " << *ptr << " bytes" << std::endl;
		delete ptr;
		cout << "*** Kernel file found but locked." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "11") {
		setColor("17");
		cout << "*** STOP: 0x00000011 [0xc00000011, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\REG.cfg" << endl;
		cout << endl;
		cout << "The registry is full, run startup repair to reset it." << endl;
		cout << "Looks like someone filled the registry with all sorts of junk, huh?" << endl;
		cout << "Run Startup Repair to restore registry." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		std::cout << "*** Size: " << *ptr << " bytes" << std::endl;
		delete ptr;
		cout << "*** REG file found but locked." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "12") {
		setColor("17");
		cout << "*** STOP: 0x00000012 [0xc00000012, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\system.log" << endl;
		cout << endl;
		cout << "LOG_OVERFLOW - The system log is full of shit." << endl;
		cout << "Someone's been writing a novel in system.log." << endl;
		cout << "150 KB is the limit. Run Startup Repair to clear the log." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		std::cout << "*** Size: " << *ptr << " bytes" << std::endl;
		delete ptr;
		cout << "*** Log file found but locked due to size limit." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "13") {
		setColor("17");
		cout << "*** STOP: 0x00000013 [0xc00000013, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\BCD" << endl;
		cout << endl;
		cout << "BCD_CORRUPTED - The BCD is missing required entries." << endl;
		cout << "DEFAULT_MODE or BOOT_TIMEOUT is missing." << endl;
		cout << "Run Startup Repair to restore the BCD." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** BCD file found but incomplete." << endl;
		cout << "*** Expected entries: BOOT_TIMEOUT, DEFAULT_MODE" << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "14") {
		setColor("17");
		cout << "*** STOP: 0x00000014 [0xc00000014, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\Settings And System Files\\BCD" << endl;
		cout << endl;
		cout << "BCD_NOT_FOUND - The BCD is missing." << endl;
		cout << "Please reinstall or run Startup Repair." << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		recoveryEnv();
	}
	else if (code == "DIED") {
		setColor("17");
		cout << "*** STOP: CRITICAL_PROCESS_DIED [0xc00000000, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "*** File: \\ARSLANIUS 28.exe" << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** ArsLogon died" << endl;
		cout << endl;
		cout << "If this is the first time you've seen this error, restart the system." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		bootMenu();
	}
	else if (code == "666") {
		setColor("17");
		cout << "*** STOP: 0xDEADBEEF [0x00000666, 0x00000000, 0x00000000, 0x00000000]" << endl;
		cout << endl;
		cout << "MANUAL_CRASH - You typed 'bsod' and now you're here. Surprised? You shouldn't be." << endl;
		cout << "This error was intentionally triggered by the 'bsod' command." << endl;
		cout << "No real damage was done. Just reboot and continue." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** Crash initiated by user: " << currentUser << endl;
		cout << "*** Stop code: 0xTeam_by_" << currentUser << endl;
		cout << "*** Next time try 'help' instead. Or don't. I'm not your mom." << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		bootMenu();
	}
	else {
		setColor("08");
		cout << "*** STOP: 0x00001225a [0x00000220, 0x00000002, 0x00000000a, 0x00000000]" << endl;
		cout << endl;
		cout << "UNKNOWN_ERROR - Something went wrong and ARSLANIUS crashed," << endl;
		cout << "perhaps the bsod environment variable was not defined due to a serious problem," << endl;
		cout << "replace the main ARSLANIUS file with the original one." << endl;
		cout << endl;
		cout << "Technical information:" << endl;
		cout << "*** UNKNOWN_ERROR" << endl;
		cout << "*** Stop code: 0x00001225a" << endl;
		cout << endl;
		cout << "For support, visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/issues" << endl;
		pause();
		exit(1);
	}
}

// =====================================================================
// BOOT MENU
// =====================================================================

void bootMenu() {
	if (!dirExists(configRoot)) {
		bsod("1a");
	};
	loadBCD();
	loadRegistry();
	HANDLE hKernel = CreateFileA(kernelPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hKernel != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(hKernel, &fileSize)) {
			if (fileSize.QuadPart > MAX_KERNEL_SIZE) {
				DWORD64 size = (DWORD64)fileSize.QuadPart;
				CloseHandle(hKernel);
				ptr = new DWORD64(size);
				bsod("10");
			}
		}
		CloseHandle(hKernel);
	}
	HANDLE hReg = CreateFileA(regPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReg != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(hReg, &fileSize)) {
			if (fileSize.QuadPart > MAX_REGISTRY_SIZE) {
				DWORD64 size = (DWORD64)fileSize.QuadPart;
				CloseHandle(hReg);
				ptr = new DWORD64(size);
				bsod("11");
			}
		}
		CloseHandle(hReg);
	}
	HANDLE hLog = CreateFileA(logPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLog != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(hLog, &fileSize)) {
			if (fileSize.QuadPart > MAX_LOG_SIZE) {
				DWORD64 size = (DWORD64)fileSize.QuadPart;
				CloseHandle(hLog);
				ptr = new DWORD64(size);
				bsod("12");
			}
		}
		CloseHandle(hLog);
	}
	if (setup == 1) {
		setColor("1f");
		cout << "Welcome to OOBE " << osName << "! Do you want to start?" << endl;
		cout << "Y/N: ";
		char choice = _getch();
		choice = tolower(choice);
		cout << choice << endl;

		switch (choice) {
		case 'y': {
			cout << "Preparing ARSLANIUS..." << endl;
			Sleep(2000);
			PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
			clearScreen();
			cout << "Type your Username: ";
			string nu, np;
			getline(cin, nu);
			nu = trim(nu);
			if (nu == "SYSTEM" || nu == "BarOS AUTHORITY\\SYSTEM" || nu == "SYSTEM ADMINISTRATOR") {
				cout << "[ ERROR ] Reserved username." << endl;
				return;
			}
			string user_exists = readFile(kernelPath);
			if (user_exists.find(nu + " =") != string::npos) {
				cout << "[ ERROR ] User already exists." << endl;
				return;
			}
			cout << "Password: ";
			char ch;
			while ((ch = _getch()) != '\r') {
				if (ch == '\b') {
					if (!np.empty()) {
						np.pop_back();
						cout << "\b \b";
					}
				}
				else {
					np += ch;
					cout << '*';
				}
			}
			cout << endl;

			string hash = calculateHash(np);
			appendFile(kernelPath, nu + " = " + hash + "\n");
			fs::create_directories(usersRoot + "\\" + nu);
			cout << "[ OK ] User created." << endl;
			pause();
			stringstream reg;
			reg << "OS_NAME=ARSLANIUS 28" << endl;
			reg << "SYSTEM_COLOR=0e" << endl;
			reg << "ADMIN_COLOR=4f" << endl;
			reg << "USER_COLOR=1f" << endl;
			reg << "ENABLE_LUA=1" << endl;
			reg << "LOCKDOWN=0" << endl;
			reg << "SETUP=0" << endl;
			reg << "REG_VERSION=" << REG_VERSION << endl;
			writeFile(regPath, reg.str());
			clearScreen();
			cout << "[ OK ] Everything is ready to go." << endl;
			pause();
			PlaySound(NULL, NULL, 0);
			bootMenu(); break;
		}
		case 'n': {
			exit(1);
		}
		default: bootMenu();
		}
		bootMenu();
	}

	if (recoveryRequest != "1") {
		clearScreen();
		setColor("0f");
		diagnostic = 0;
		safeMode = 0;
		rec = 0;
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
		DWORD64 startTime = GetTickCount64();
		bool keyPressed = false;
		char choice = defaultMode; // default

		while (GetTickCount64() - startTime < (DWORD64)(bootTimeout * 1000)) {
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

		if (!fileExists(kernelPath)) {
			bsod("1");
		};
		if (!fileExists(regPath)) {
			bsod("4");
		};
		check_registry();
		if (REG_VERSION_FOUND != std::stoi(REG_VERSION)) {
			bsod("3");
		}
		check_AUTHORITY();
		check_kernel();
		string kernel_hash_check = readFile(kernelPath);
		if (kernel_hash_check.find("SYSTEM = " + EXPECTED_SYSTEM_HASH) == string::npos) bsod("2");
		if (kernel_hash_check.find("SYSTEM ADMINISTRATOR = " + EXPECTED_ADMIN_HASH) == string::npos) bsod("6");
		if (!fileExists(configRoot + "\\BCD")) {
			bsod("14");
		};
		check_BCD();
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
		clearScreen();
		setColor("0f");
		cout << "======================================================================================================================" << endl;
		cout << "                                                 ARSLANIUS BOOT MANAGER" << endl;
		cout << "======================================================================================================================" << endl;
		cout << "                                                  Loading " << osName << endl;
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
	clearScreen();
	setColor("0f");
	safeMode = 1;
	cout << "======================================================================================================================" << endl;
	cout << "                                                 ARSLANIUS BOOT MANAGER" << endl;
	cout << "======================================================================================================================" << endl;
	Sleep(2000);

	if (!fileExists(configRoot + "\\BCD")) {
		PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
	fs::remove(sysServices + "\\SysPulse.active");
	fs::remove(sysServices + "\\TrustedInstaller.active");
	fs::remove(sysServices + "\\NetMonitor.active");

	logonScreen();
}

void diagnosticMode(int mode) {
	diagnostic = mode;
	safeMode = 0;
	rec = 0;

	// Disable some services
	if (mode == 1) {
		fs::remove(sysServices + "\\SysPulse.active");
		fs::remove(sysServices + "\\TrustedInstaller.active");
		fs::remove(sysServices + "\\NetMonitor.active");
		currentUser = "BarOS SERVICE\\SysPulse";
	}
	else if (mode == 2) {
		fs::remove(sysServices + "\\SysPulse.active");
		fs::remove(sysServices + "\\TrustedInstaller.active");
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

	clearScreen();
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
	reg << "OS_NAME=ARSLANIUS 28" << endl;
	reg << "SYSTEM_COLOR=0e" << endl;
	reg << "ADMIN_COLOR=4f" << endl;
	reg << "USER_COLOR=1f" << endl;
	reg << "ENABLE_LUA=1" << endl;
	reg << "LOCKDOWN=1" << endl;
	reg << "SETUP=1" << endl;
	reg << "REG_VERSION=" << REG_VERSION << endl;
	writeFile(regPath, reg.str());

	writeLog("KERNEL_RESTORED_BY_RECOVERY");

	cout << "[ OK ] Startup Repair completed." << endl;
	pause();
}

void restoreMenu() {
	if (!dirExists(restoreRoot)) {
		PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
		cout << "[ ERROR ] No restore points directory." << endl;
		pause();
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
		PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
		cout << "[ ERROR ] Invalid restore point." << endl;
		pause();
		return;
	}

	fs::copy_file(rpPath + "\\kernel.dll", kernelPath, fs::copy_options::overwrite_existing);
	fs::copy_file(rpPath + "\\REG.cfg", regPath, fs::copy_options::overwrite_existing);
	fs::copy_file(rpPath + "\\system.log", logPath, fs::copy_options::overwrite_existing);

	writeLog("RESTORE_APPLIED_FROM_RECOVERY: " + rpSel);

	cout << "[ OK ] System restored from " << rpSel << "." << endl;
	pause();
}

void imageRecovery() {
	cout << endl << "===== SYSTEM IMAGE RECOVERY =====" << endl;
	cout << endl;

	string backupPath = rootPath + "\\Backup";
	if (dirExists(backupPath)) {
		cout << "[ FOUND ] Backup directory detected." << endl;
	}
	else {
		PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
		cout << "[ ERROR ] No backup image found." << endl;
		pause();
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
	pause();
}

void memoryDiag() {
	clearScreen();
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
	pause();
}

// =====================================================================
// LOGON SCREEN
// =====================================================================

void logonScreen() {
	currentUser = "SYSTEM";
	userHome = sysProf;
	acpiRequest = 0;

	clearScreen();
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

	getline(cin, u_in);

	if (u_in == "Shutdown" || u_in == "shutdown") {
		writeLog("SHUTDOWN_FROM_LOGON");
		acpiRequest = 1;
		arslogon("logoutRequest");
		return;
	}
	if (u_in == "Reboot" || u_in == "reboot") {
		writeLog("REBOOT_FROM_LOGON");
		acpiRequest = 2;
		arslogon("logoutRequest");
		return;
	}
	if (u_in == "arslogon -emergency reboot" || u_in == "rebootemer" || u_in == "Rebootemer") {
		writeLog("EMERGENCY_REBOOT_FROM_LOGON");
		arslogon("emergency_reboot");
		return;
	}

	if (u_in.empty()) {
		logonScreen();
		return;
	}

	// Check login attempts
	failFile = sysServices + "\\fail_" + u_in + ".cnt";
	if (fileExists(failFile)) {
		ifstream f(failFile);
		f >> loginAttempts;
	}

	if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
		bsod("9");
		return;
	}

	cout << "Password: ";
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
	arslogon("authorization");
}

void arslogon(const string& authority) {
	if (authority == "authorization") {
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
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] User not found." << endl;
			pause();
			logonScreen();
			return;
		}

		string inputHash = calculateHash(p_in);

		if (inputHash == storedHash) {
			// Success
			fs::remove(failFile);

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

			fs::create_directories(userHome);
			SetCurrentDirectoryA(userHome.c_str());

			if (currentUser == "Armsoup") {
				cout << "[ ERROR ] Login denied by the system" << endl;
				pause();
				logonScreen();
			}
			if (lockdown == 1) {
				cout << "[ ERROR ] Login denied by registry policy" << endl;
				pause();
				logonScreen();
			}
			if (currentUser.find("BarOS SERVICE") == 0) {
				cout << "[ ERROR ] Seriously? You decided to join the " << osName << " service? Hahahaha" << endl;
				pause();
				logonScreen();
			}

			applyColor();
		}
		else {
			loginAttempts++;
			ofstream f(failFile);
			f << loginAttempts;
			f.close();

			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Password incorrect. (Attempt " << loginAttempts << "/" << MAX_LOGIN_ATTEMPTS << ")" << endl;
			if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
				bsod("9");
				return;
			}
			pause();
			logonScreen();
		}
	}
	else if (authority == "SecureAS_lockmenu") {
		clearScreen();
		cout << "Secure Settings" << endl;
		cout << endl;
		cout << "                                                 1. Logout" << endl;
		cout << "                                                 2. Passwd" << endl;
		cout << "                                                 3. Shutdown" << endl;
		cout << "                                                 4. Reboot" << endl;
		cout << "                                                 5. Help" << endl;
		cout << "                                                 6. Return" << endl;
		cout << endl;
		cout << endl;
		cout << "Automatically logout after 60 seconds..." << endl;
		cout << "Select option (1-6): ";
		DWORD64 startTime = GetTickCount64();
		bool keyPressed = false;
		char ch = '1'; // default

		while (GetTickCount64() - startTime < (DWORD64)(logoutTimeout * 1000)) {
			if (_kbhit()) {
				ch = _getch();
				keyPressed = true;
				break;
			}
			Sleep(100);
		}
		cout << ch << endl;
		switch (ch) {
		case '1': acpiRequest = 0; arslogon("logoutRequest"); break;
		case '2': core("passwd"); break;
		case '3': acpiRequest = 1; arslogon("logoutRequest"); break;
		case '4': acpiRequest = 2; arslogon("logoutRequest"); break;
		case '5': core("help"); break;
		case '6': interfaceScreen();
		}
	}
	else if (authority == "waitMode") {
		clearScreen();
		setColor("0f");
		cout << "\n\n                                                  " << osName << "\n\n";
		pause();
		applyColor();
	}
	else if (authority == "logoutRequest") {
		clearScreen();
		setColor("5b");
		cout << "\n\n                                                      Logout...\n\n";
		Sleep(2000);

		PlaySoundA("C:\\Windows\\Media\\Windows Shutdown.wav", NULL, SND_FILENAME | SND_ASYNC);
		if (acpiRequest == 1) shutdownScreen();
		else if (acpiRequest == 2) rebootScreen();
		else {
			currentUser = "SYSTEM";
			userHome = sysProf;
			logonScreen();
			return;
		}
	}
	else if (authority == "emergency_reboot") {
		clearScreen();
		setColor("5b");
		cout << endl;
		cout << endl;
		cout << "                                                 Please Wait..." << endl;
		Sleep(500);
		cout << "                                             terminating ArsLogon..." << endl;
		Sleep(1000);
		bsod("DIED");
	}
	else {
		clearScreen();
		setColor("5b");
		cout << endl;
		cout << endl;
		cout << "                                                 Please Wait..." << endl;
		Sleep(2000);
		cout << "                                 ERROR: Session active, terminating ArsLogon..." << endl;
		Sleep(1500);
		bsod("DIED");
	}
}



void applyColor() {
	PlaySoundA("C:\\Windows\\Media\\Windows Logon.wav", NULL, SND_FILENAME | SND_ASYNC);
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
	interfaceScreen();
}

// =====================================================================
// INTERFACE AND COMMAND LOOP
// =====================================================================

void interfaceScreen() {
	saveBCD();

	// Start services
	if (safeMode == 0 && rec == 0 && diagnostic == 0) {
		if (!fileExists(sysServices + "\\SysPulse.active")) {
			cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\SysPulse..." << endl;
			writeFile(sysServices + "\\SysPulse.active", "RUNNING");
			Sleep(1000);
		}
		if (!fileExists(sysServices + "\\NetMonitor.active")) {
			cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\NetMonitor..." << endl;
			writeFile(sysServices + "\\NetMonitor.active", "RUNNING");
			Sleep(1000);
		}
		if (!fileExists(sysServices + "\\TrustedInstaller.active")) {
			cout << "[ KERNEL ] Booting background service: BarOS SERVICE\\TrustedInstaller..." << endl;
			writeFile(sysServices + "\\TrustedInstaller.active", "RUNNING");
			Sleep(1000);
		}
	}

	if (rec == 1) {
		currentUser = "BarOS SERVICE\\TrustedInstaller";
		fs::remove(sysServices + "\\SysPulse.active");
		fs::remove(sysServices + "\\NetMonitor.active");
		userHome = sysServices + "\\TrustedInstaller";
		fs::create_directories(userHome);
	}
	if (diagnostic == 1) {
		currentUser = "BarOS SERVICE\\SysPulse";
		fs::remove(sysServices + "\\TrustedInstaller.active");
		fs::remove(sysServices + "\\NetMonitor.active");
		userHome = sysServices + "\\SysPulse";
		fs::create_directories(userHome);
	}
	if (diagnostic == 2) {
		currentUser = "BarOS SERVICE\\NetMonitor";
		fs::remove(sysServices + "\\SysPulse.active");
		fs::remove(sysServices + "\\TrustedInstaller.active");
		userHome = sysServices + "\\NetMonitor";
		fs::create_directories(userHome);
	}

	SetCurrentDirectoryA(userHome.c_str());

	clearScreen();
	if (diagnostic == 2 || diagnostic == 1 || rec == 1) setColor("0e");
	if (safeMode == 1) setColor("07");

	cout << osName << " [Build " << currentBuild << "] - Session: " << currentUser;
	cout << " (SAFE MODE: " << safeMode << ")" << endl;
	cout << "Profile: " << userHome << endl;
	cout << "Have ideas or found a bug? Visit: https://github.com/Armsoup/ARSLANIUS_C-Plus_Plus/discussions" << endl;
	cout << "Or: https://t.me/+8FQ20tOaKI5lNGMy" << endl;
	cout << "----------------------------------------------------------------------------------------------------------------------" << endl;

	// Check autorun
	if (safeMode == 0 && rec == 0 && diagnostic == 0) {
		string alertFile = userHome + "\\alert.sys";
		if (fileExists(alertFile)) {
			setColor("4f");
			cout << "======================================================================================================================" << endl;
			cout << "                                                    CRITICAL SYSTEM ALERT" << endl;
			cout << "======================================================================================================================" << endl;
			cout << endl;
			cout << "                                                   " << readFile(alertFile) << endl;
			cout << endl;
			cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
			cout << endl;
			pause();
			fs::remove(alertFile);
			writeLog("ALERT_VIEVED: " + currentUser);
			applyColor();
			interfaceScreen();
		}
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
		// services
		if (safeMode == 0 && rec == 0 && diagnostic == 0) {
			if (!dirExists(sysServices)) fs::create_directories(sysServices);
			if (!fileExists(sysServices + "\\TrustedInstaller.active")) {
				writeFile(sysServices + "\\TrustedInstaller.active", "RUNNING");
			}
			if (fileExists(sysServices + "\\TrustedInstaller.active")) {
				int ins_err = 0;
				if (!fileExists(kernelPath)) ins_err = 1;
				if (!fileExists(regPath)) ins_err = 1;
				if (!fileExists(configRoot + "\\BCD")) ins_err = 1;

				if (ins_err == 1) {
					cout << "[BarOS SERVICE\\TrustedInstaller] Integrity violation detected!" << endl;
					cout << "[BarOS SERVICE\\TrustedInstaller] Executing background repair..." << endl;
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
						reg_ins << "OS_NAME=ARSLANIUS 28" << endl;
						reg_ins << "SYSTEM_COLOR=0e" << endl;
						reg_ins << "ADMIN_COLOR=4f" << endl;
						reg_ins << "USER_COLOR=1f" << endl;
						reg_ins << "ENABLE_LUA=1" << endl;
						reg_ins << "LOCKDOWN=1" << endl;
						reg_ins << "SETUP=1" << endl;
						reg_ins << "REG_VERSION=" << REG_VERSION << endl;
						writeFile(regPath, reg_ins.str());
						cout << "[BarOS SERVICE\\TrustedInstaller] System restored." << endl;
						writeLog("BarOS SERVICE\\TrustedInstaller: AUTO_REPAIR_SUCCESS");
					}
				}
			}
			if (fileExists(sysServices + "\\SysPulse.active")) {
				int PulseCheck = rand() % 10;
				if (PulseCheck == 5) {
					writeLog("BarOS SERVICE\\SYSPULSE: System Health OK");
				}
				const size_t MAX_LOG_SIZE_SYS = 10240;
				HANDLE hLog = CreateFileA(logPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hLog != INVALID_HANDLE_VALUE) {
					LARGE_INTEGER fileSize;
					if (GetFileSizeEx(hLog, &fileSize)) {
						if (fileSize.QuadPart > MAX_LOG_SIZE_SYS) {
							CloseHandle(hLog);
							fs::remove(logPath);
							writeLog("BarOS SERVICE\\SYSPULSE: LOG_CLEARED");
						}
					}
					CloseHandle(hLog);
				}
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
		// Prompt
		sudo_command = 0;
		cout << currentUser << "@ARSLANIUS> ";
		string cmd;
		getline(cin, cmd);
		cmd = trim(cmd);

		if (cmd.empty()) continue;

		core(cmd);
	}
}

void core(const string& cmd) {
	vector<string> parts = split(cmd, ' ');
	if (parts.empty()) return;

	if (cmd == "arslogon -emergency reboot" || cmd == "rebootemer") {
		arslogon("emergency_reboot");
	}
	if (cmd == "arslogon") {
		if (currentUser != "BarOS SERVICE\\TrustedInstaller" &&
			currentUser != "BarOS SERVICE\\SysPulse" &&
			currentUser != "BarOS SERVICE\\NetMonitor"
			) {
			arslogon("");
		}
	}
	if (cmd == "lockmenu") {
		if (currentUser != "BarOS SERVICE\\TrustedInstaller" &&
			currentUser != "BarOS SERVICE\\SysPulse" &&
			currentUser != "BarOS SERVICE\\NetMonitor"
			) {
			arslogon("SecureAS_lockmenu");
		}
	}
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
		sudo_command = 0;
		char ch;
		while ((ch = _getch()) != '\r') {
			if (ch == '\b') {
				if (!a_p.empty()) {
					a_p.pop_back();
					cout << "\b \b";
				}
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
			sudo_command = 1;
			core(ex_c);
		}
		else {
			sudo_command = 0;
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Access denied." << endl;
		}
		return;
	}

	// Command routing
	transform(ex_c.begin(), ex_c.end(), ex_c.begin(), ::tolower);

	if (currentUser == "GUEST") {
		bool allowed = false;
		vector<string> guestCmds = { "help", "lock", "wait_mode", "echo", "ls", "cd",
									 "lockmenu", "report", "cls", "ver", "whoami",
									 "calc", "notepad", "reboot", "shutdown" };
		for (const string& c : guestCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Guest cannot use this command." << endl;
			return;
		}
	}

	if (currentUser == "BarOS SERVICE\\TrustedInstaller") {
		bool allowed = false;
		vector<string> tiCmds = { "help", "mv", "bcdboot", "cp", "rm", "touch", "edit",
								  "echo", "bcdedit", "mkdir", "ls", "cd", "cat", "ren",
								  "reset", "reboot_to_recovery", "cls", "ver", "whoami",
								  "events", "sfc", "adduser", "deluser", "regedit",
								  "reboot", "shutdown" };
		for (const string& c : tiCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Restricted." << endl;
			return;
		}
	}

	if (currentUser == "BarOS SERVICE\\SysPulse") {
		bool allowed = false;
		vector<string> spCmds = { "help", "ls", "sysinfo", "events", "report", "echo",
								  "taskmgr", "cd", "cat", "reboot", "shutdown",
								  "reboot_to_recovery", "cls", "ver", "whoami" };
		for (const string& c : spCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Restricted." << endl;
			return;
		}
	}

	if (currentUser == "BarOS SERVICE\\NetMonitor") {
		bool allowed = false;
		vector<string> nmCmds = { "help", "reboot", "ping", "netstat", "ipconfig",
								  "echo", "tracert", "nslookup", "arp", "route",
								  "shutdown", "cls", "whoami" };
		for (const string& c : nmCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Restricted." << endl;
			return;
		}
	}

	if (safeMode == 1) {
		bool allowed = false;
		vector<string> safeCmds = { "help", "lock", "wait_mode", "lockmenu", "mv", "cp",
									"echo", "bcdboot", "bcdedit", "rm", "touch", "mkdir",
									"ls", "cd", "cat", "ren", "backup", "backup-restore",
									"sysinfo", "reset", "reboot_to_recovery", "report",
									"cls", "ver", "whoami", "events", "sfc", "restore-point",
									"restore", "adduser", "deluser", "regedit", "reboot",
									"shutdown" };
		for (const string& c : safeCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Safe mode restriction." << endl;
			return;
		}
	}

	if (currentUser == "SYSTEM ADMINISTRATOR") {
		bool allowed = false;
		vector<string> adminCmds = { "help", "calc", "ping", "wait_mode", "lockmenu",
									 "echo", "autorun", "bcdedit", "bcdboot", "netstat",
									 "ipconfig", "tracert", "nslookup", "arp", "route",
									 "taskmgr", "sysinfo", "cp", "mv", "rm", "reset",
									 "bsod", "touch", "mkdir", "ls", "cd", "cat", "ren",
									 "backup", "backup-restore", "lock", "events",
									 "reboot_to_recovery", "report", "cls", "ver", "whoami",
									 "shutdown", "reboot", "adduser", "start", "sfc",
									 "clean", "mail-read", "mail-send", "edit", "guest",
									 "regedit", "deluser", "msg-all", "broadcast", "alert", "restore-point",
									 "restore" };
		for (const string& c : adminCmds) {
			if (ex_c == c) { allowed = true; break; }
		}
		if (!allowed) {
			cout << "[ SECURITY ] Restricted context." << endl;
			return;
		}
	}

	if (enableLua == 1 &&
		currentUser != "BarOS AUTHORITY\\SYSTEM" &&
		currentUser != "SYSTEM ADMINISTRATOR" &&
		currentUser != "BarOS SERVICE\\TrustedInstaller" &&
		currentUser != "BarOS SERVICE\\SysPulse" &&
		currentUser != "BarOS SERVICE\\NetMonitor" &&
		sudo_command == 0) {
		bool allowed = false;
		vector<string> userCmds = { "help", "arsstore", "mkdir", "wait_mode", "echo", "lockmenu",
									"autorun", "ping", "cp", "mv", "touch", "backup",
									"ls", "cd", "cat", "ren", "backup-restore", "passwd",
									"reboot_to_recovery", "lock", "calc", "sysinfo",
									"restore", "restore-point", "mail-send", "mail-read",
									"clean", "report", "cls", "ver", "whoami", "calc",
									"notepad", "reboot", "shutdown" };
		for (const string& c : userCmds) {
			if (ex_c == c) { allowed = true; sudo_command = 0; break; }
		}
		if (!allowed) {
			cout << "Error: Access Denied. Use \"sudo " << ex_c << "\"." << endl;
			sudo_command = 0;
			return;
		}
	}

	if (ex_c == "help" || ex_c == "?") {
		cout << "Apps: Notepad, Calc, taskmgr, edit, install, regedit, ArsStore, sysinfo" << endl;
		cout << "System: Help, Lock, lockmenu, sudo, cls, Shutdown, ver, whoami, reboot, clean, events, restore-point, restore, echo, passwd, backup, backup-restore, ls, wait_mode, cd, cat, ren, mkdir, touch, cp, rebootemer or arslogon -emergency reboot, mv, autorun" << endl;
		cout << "Admin: adduser, deluser, alert, Guest, report, reset, reboot_to_recovery, bsod, rm, netstat, ipconfig, tracert, nslookup, arp, route, bcdboot, bcdedit" << endl;
	}
	else if (ex_c == "cls") interfaceScreen();
	else if (ex_c == "ver") cout << osName << " [Build " << currentBuild << "]" << endl;
	else if (ex_c == "whoami") {
		cout << "Current User: " << currentUser << endl;
		cout << "Path: " << userHome << endl;
	}
	else if (ex_c == "mail-read") {
		string mailFile = userHome + "\\mail.txt";
		if (fileExists(mailFile)) {
			clearScreen();
			cout << "======================================================================================================================" << endl;
			cout << "                                                        MAILBOX" << endl;
			cout << "======================================================================================================================" << endl;
			cout << endl;
			cout << readFile(mailFile) << endl;
			cout << endl;
			cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
			cout << "Clear mailbox? (Y/N): ";
			char choice = _getch();
			choice = tolower(choice);
			cout << choice << endl;

			switch (choice) {
			case 'y': {
				fs::remove(mailFile);
				cout << "[ OK ] Mailbox cleared." << endl;
			}
			}
		}
		else {
			cout << "[ INFO ] No mail." << endl;
		}
	}
	else if (ex_c == "mail-send") {
		string m_to, m_text;
		cout << "To user: ";
		getline(cin, m_to);
		m_to = trim(m_to);
		if (m_to.empty()) {
			cout << "[ ERROR ] Username cannot be empty." << endl;
			return;
		}
		string hkernel = readFile(kernelPath);
		if (hkernel.find(m_to + " =") == string::npos && hkernel.find(m_to + "=") == string::npos && m_to != "SYSTEM" && m_to != "SYSTEM ADMINISTRATOR") {
			cout << "[ ERROR ] User not found." << endl;
			return;
		}
		cout << "Message: ";
		getline(cin, m_text);
		if (m_text.empty()) {
			cout << "[ ERROR ] Message cannot be empty." << endl;
			return;
		}
		string destPath;
		if (m_to == "SYSTEM" || m_to == "BarOS AUTHORITY\\SYSTEM") {
			destPath = sysProf;
		}
		else if (m_to == "SYSTEM ADMINISTRATOR") {
			destPath = usersRoot + "\\SYSTEM ADMINISTRATOR";
		}
		else {
			destPath = usersRoot + "\\" + m_to;
		}
		fs::create_directories(destPath);
		string mailFile = destPath + "\\mail.txt";
		string mailEntry = "[" + getCurrentDateTime() + "] " + "From " + currentUser + ": " + m_text + "\n\n";
		appendFile(mailFile, mailEntry);
		writeLog("MAIL_SENT: " + currentUser);
		cout << "[ OK ] Message sent to " << m_to << "." << endl;
	}
	else if (ex_c == "msg-all" || ex_c == "broadcast") {
		string msgText;
		cout << "Global Message: ";
		getline(cin, msgText);
		if (msgText.empty()) {
			cout << "[ ERROR ] Message cannot be empty." << endl;
			return;
		}
		string mailEntry = "[" + getCurrentDateTime() + "] " + "[GLOBAL] From " + currentUser + ": " + msgText + "\n\n";
		for (const auto& entry : fs::directory_iterator(usersRoot)) {
			if (entry.is_directory()) {
				string mailFile = entry.path().string() + "\\mail.txt";
				appendFile(mailFile, mailEntry);
			}
		}
		string sysMail = sysProf + "\\mail.txt";
		appendFile(sysMail, mailEntry);
		writeLog("BROADCAST: " + currentUser);
		cout << "[ OK ] Broadcast send to all users." << endl;
	}
	else if (ex_c == "alert") {
		string alertText;
		cout << "Alert Message: ";
		getline(cin, alertText);
		if (alertText.empty()) {
			cout << "[ ERROR ] Alert Message cannot be empty." << endl;
			return;
		}
		for (const auto& entry : fs::directory_iterator(usersRoot)) {
			if (entry.is_directory()) {
				string alertFile = entry.path().string() + "\\alert.sys";
				writeFile(alertFile, alertText);
			}
		}
		string sysAlert = sysProf + "\\alert.sys";
		writeFile(sysAlert, alertText);
		writeLog("ALERT_SENT: " + alertText);
		cout << "[ OK ] Alert deployed to all users." << endl;
	}
	else if (ex_c == "arsstore") {
		clearScreen();
		cout << "======================================================================================================================" << endl;
		cout << "                                                    ARSLANIUS STORE[v2.0]" << endl;
		cout << "======================================================================================================================" << endl;
		cout << "Available Apps :" << endl;
		cout << " [1] System Scanner(Utility)" << endl;
		cout << " [2] NotePad Lite(Office)" << endl;
		cout << " [3] Calc(Utility)" << endl;
		cout << " [4] Exit Store" << endl;
		cout << "----------------------------------------------------------------------------------------------------------------------" << endl;
		cout << "Select number: ";
		char choice = _getch();
		cout << choice << endl;
		if (choice == '1') installapp("1");
		else if (choice == '2') installapp("2");
		else if (choice == '3') installapp("3");
		else if (choice == '4') interfaceScreen();
		core("arsstore");
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
				PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] File not found." << endl;
		}
	}
	else if (ex_c == "autorun") {
		string autorunCmd;
		cout << "Command to autorun: ";
		getline(cin, autorunCmd);
		autorunCmd = trim(autorunCmd);
		if (autorunCmd.empty()) {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Command cannot be empty." << endl;
			return;
		}
		writeFile(userHome + "\\autorun.txt", autorunCmd);
		writeLog("AUTORUN_SET: " + autorunCmd + " BY " + currentUser);
		cout << "[ OK ] Autorun set: " << autorunCmd << endl;
	}
	else if (ex_c == "mkdir") {
		string folderName;
		cout << "Enter folder name: ";
		getline(cin, folderName);
		folderName = trim(folderName);
		if (fs::create_directory(folderName)) {
			cout << "[ OK ] Folder created." << endl;
		}
		else {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
		if (fs::copy_file(src, dst)) {
			cout << "[ OK ] Copied." << endl;
		}
		else {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Move failed." << endl;
		}
	}
	else if (ex_c == "rm") {
		string targetFile;
		cout << "File to delete: ";
		getline(cin, targetFile);
		targetFile = trim(targetFile);
		if (fs::remove(targetFile)) {
			cout << "[ OK ] Deleted." << endl;
		}
		else {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Rename failed." << endl;
		}
	}
	else if (ex_c == "echo") {
		string text;
		cout << "Enter text: ";
		getline(cin, text);
		cout << text << endl;
	}
	else if (ex_c == "reset") {
		cout << "WARNING: This will delete ALL users and reset system to defaults. " << endl;
		cout << "Type YES to continue: ";
		string confirm;
		getline(cin, confirm);
		confirm = trim(confirm);
		if (confirm == "YES") {
			startupRepair();
			userHome = rootPath;
			SetCurrentDirectoryA(userHome.c_str());
			try {
				fs::path pathToRemove = rootPath + "\\Users";
				if (fs::exists(pathToRemove)) {
					fs::remove_all(pathToRemove);
				}
				cout << "[ OK ] System reset completed. Shutting Down..." << endl;
			}
			catch (const fs::filesystem_error& e) {
				bsod("");
			}
			pause();
			acpiRequest = 1;
			arslogon("logoutRequest");
		}
		else {
			cout << "Canceled." << endl;
		}
	}
	else if (ex_c == "shutdown") {
		acpiRequest = 1;
		arslogon("logoutRequest");
	}
	else if (ex_c == "reboot") {
		acpiRequest = 2;
		arslogon("logoutRequest");
	}
	else if (ex_c == "lock") {
		acpiRequest = 0;
		arslogon("logoutRequest");
	}
	else if (ex_c == "bsod") bsod("666");
	else if (ex_c == "wait_mode") arslogon("waitMode");
	else if (ex_c == "adduser") {
		string nu, np;
		cout << "Username: ";
		getline(cin, nu);
		nu = trim(nu);
		if (nu == "SYSTEM" || nu == "BarOS AUTHORITY\\SYSTEM" || nu == "SYSTEM ADMINISTRATOR") {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Reserved username." << endl;
			return;
		}
		string user_exists = readFile(kernelPath);
		if (user_exists.find(nu + " =") != string::npos) {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] User already exists." << endl;
			return;
		}
		cout << "Password: ";
		char ch;
		while ((ch = _getch()) != '\r') {
			if (ch == '\b') {
				if (!np.empty()) {
					np.pop_back();
					cout << "\b \b";
				}
			}
			else {
				np += ch;
				cout << '*';
			}
		}
		cout << endl;

		string hash = calculateHash(np);
		appendFile(kernelPath, nu + " = " + hash + "\n");
		fs::create_directories(usersRoot + "\\" + nu);
		writeLog("NEW_USER_CREATED: " + nu);
		cout << "[ OK ] User created." << endl;
	}
	else if (ex_c == "deluser") {
		string du;
		cout << "Enter username: ";
		getline(cin, du);
		du = trim(du);
		if (du == "SYSTEM" || du == "SYSTEM ADMINISTRATOR") {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
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
			if (ch == '\b') {
				if (!old.empty()) {
					old.pop_back();
					cout << "\b \b";
				}
			}
			else { old += ch; cout << '*'; }
		}
		cout << endl;
		string inputHash = calculateHash(old);

		string kernel_passwd = readFile(kernelPath);
		istringstream isspasswd(kernel_passwd);
		string line_passwd;
		bool found = false;
		string storedHash;

		while (getline(isspasswd, line_passwd)) {
			line_passwd = trim(line_passwd);
			if (line_passwd.find(currentUser + " =") == 0 || line_passwd.find(u_in + "=") == 0) {
				size_t eqPos = line_passwd.find('=');
				storedHash = trim(line_passwd.substr(eqPos + 1));
				found = true;
				break;
			}
		}
		if (inputHash == storedHash) {
		}
		else {
			PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
			cout << "[ ERROR ] Password incorrect." << endl;
			pause();
			cmdLoop();
		}

		cout << "New password: ";
		while ((ch = _getch()) != '\r') {
			if (ch == '\b') {
				if (!new1.empty()) {
					new1.pop_back();
					cout << "\b \b";
				}
			}
			else { new1 += ch; cout << '*'; }
		}
		cout << endl;

		cout << "Confirm: ";
		while ((ch = _getch()) != '\r') {
			if (ch == '\b') {
				if (!new2.empty()) {
					new2.pop_back();
					cout << "\b \b";
				}
			}
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
		if (!dirExists(restoreRoot)) fs::create_directories(restoreRoot);

		string rpName = "RP_" + getCurrentDateTime();
		for (auto& c : rpName) if (c == ' ' || c == ':') c = '_';

		string rpDir = restoreRoot + "\\" + rpName;
		fs::create_directories(rpDir);
		fs::copy_file(kernelPath, rpDir + "\\kernel.dll", fs::copy_options::overwrite_existing);
		fs::copy_file(regPath, rpDir + "\\REG.cfg", fs::copy_options::overwrite_existing);
		fs::copy_file(logPath, rpDir + "\\system.log", fs::copy_options::overwrite_existing);

		writeLog("RESTORE_POINT_CREATED: " + rpName);
		cout << "[ OK ] Restore point created: " << rpName << endl;
	}
	else if (ex_c == "restore") {
		restoreMenu();
	}
	else if (ex_c == "backup") {
		string backupDir = rootPath + "\\Backup";
		if (fs::exists(backupDir)) {
			fs::remove_all(backupDir);
		}
		fs::create_directories(backupDir);
		for (const auto& entry : fs::directory_iterator(rootPath)) {
			string name = entry.path().filename().string();
			if (name != "Backup") {
				if (entry.is_directory()) {
					fs::copy(entry.path(), backupDir + "\\" + name, fs::copy_options::recursive);
				}
				else {
					fs::copy_file(entry.path(), backupDir + "\\" + name, fs::copy_options::overwrite_existing);
				}
			}
		}
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
		clearScreen();
		cout << "======================================================================================================================" << endl;
		cout << "                                                ARSLANIUS SYSTEM INFORMATION" << endl;
		cout << "======================================================================================================================" << endl;
		cout << endl;
		cout << "[USER]" << endl;
		cout << "  Current User  : " << currentUser << endl;
		cout << "  Home          : " << userHome << endl;
		cout << endl;
		cout << "[SYSTEM]" << endl;
		cout << "  OS Name       : " << osName << endl;
		cout << "  Build         : " << currentBuild << endl;
		cout << "  Kernel        : BarOS 23.1" << endl;
		cout << "  Safe Mode     : " << safeMode << endl;
		cout << endl;
		cout << "[SERVICES]" << endl;
		if (fileExists(sysServices + "\\SysPulse.active")) {
			cout << "  BarOS SERVICE\\SysPulse          : ONLINE" << endl;
		}
		else {
			cout << "  BarOS SERVICE\\SysPulse          : OFFLINE" << endl;
		}
		if (fileExists(sysServices + "\\TrustedInstaller.active")) {
			cout << "  BarOS SERVICE\\TrustedInstaller  : ONLINE" << endl;
		}
		else {
			cout << "  BarOS SERVICE\\TrustedInstaller  : OFFLINE" << endl;
		}
		if (fileExists(sysServices + "\\NetMonitor.active")) {
			cout << "  BarOS SERVICE\\NetMonitor        : ONLINE" << endl;
		}
		else {
			cout << "  BarOS SERVICE\\NetMonitor        : OFFLINE" << endl;
		}
		pause();
	}
	else if (ex_c == "reboot_to_recovery") {
		recoveryRequest = "1";
		acpiRequest = 2;
		arslogon("logoutRequest");
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
		auto sanitize = [](string& s, string def) {
			if (s.empty()) { s = def; return; }
			for (char c : s) {
				if ((unsigned char)c < 32 && c != '\n' && c != '\r') {
					s = def;
					return;
				}
			}
		};

		sanitize(osName, "ARSLANIUS 28");
		sanitize(systemColor, "0e");
		sanitize(adminColor, "4f");
		sanitize(userColor, "1f");
		cout << "[ SECURITY ] Press enter." << endl;
		if (cin.peek() == EOF || cin.peek() == '\n') cin.ignore();
		cin.ignore(cin.rdbuf()->in_avail(), '\n');
		if (cin.peek() == '\n') cin.ignore();
		string input;
		cout << "Enter new OS Name (or Enter to skip): ";
		input = "";
		if (getline(cin, input)) {
			string t = trim(input);
			if (!t.empty()) osName = t;
		}
		cout << "Enter new system color (or Enter to skip): ";
		input = "";
		if (getline(cin, input)) {
			string t = trim(input);
			if (!t.empty()) systemColor = t;
		}
		cout << "Enter new admin color (or Enter to skip): ";
		input = "";
		if (getline(cin, input)) {
			string t = trim(input);
			if (!t.empty()) adminColor = t;
		}
		cout << "Enter new user color (or Enter to skip): ";
		input = "";
		if (getline(cin, input)) {
			string t = trim(input);
			if (!t.empty()) userColor = t;
		}
		if (osName.empty()) osName = "ARSLANIUS 28";
		if (systemColor.empty()) systemColor = "0e";
		if (adminColor.empty()) adminColor = "4f";
		if (userColor.empty()) userColor = "1f";
		if (osName.length() > 50) osName = "ARSLANIUS 28";
		if (systemColor.length() > 10) systemColor = "0e";
		if (adminColor.length() > 10) adminColor = "4f";
		if (userColor.length() > 10) userColor = "1f";

		stringstream reg;
		reg << "OS_NAME=" << osName << endl;
		reg << "SYSTEM_COLOR=" << systemColor << endl;
		reg << "ADMIN_COLOR=" << adminColor << endl;
		reg << "USER_COLOR=" << userColor << endl;
		reg << "ENABLE_LUA=" << enableLua << endl;
		reg << "LOCKDOWN=0" << endl;
		reg << "SETUP=0" << endl;
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
		std::string calcPath = programsRoot + "\\Calc.bat";
		std::string command = "start \"\" \"" + calcPath + "\"";
		system(command.c_str());
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
	else if (ex_c == "arp") {
		system("arp -a");
	}
	else if (ex_c == "route") {
		system("route print");
	}
	else if (ex_c == "netstat") {
		system("netstat -an");
	}
	else if (ex_c == "nslookup") {
		system("nslookup");
	}
	else {
		cout << "\"" << ex_c << "\" is not recognized." << endl;
	}
}

void shutdownScreen() {
	clearScreen();
	setColor("9f");
	cout << "\n\n\n\n\n\n\n\n\n                                               SHUTTING DOWN\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
	cout << "                                               " << osName << endl;
	Sleep(3000);
	fs::remove(sysServices + "\\SysPulse.active");
	fs::remove(sysServices + "\\TrustedInstaller.active");
	fs::remove(sysServices + "\\NetMonitor.active");
	exit(0);
}

void rebootScreen() {
	clearScreen();
	setColor("9f");
	cout << "\n\n\n\n\n\n\n\n\n                                               SHUTTING DOWN\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
	cout << "                                               " << osName << endl;
	Sleep(3000);
	fs::remove(sysServices + "\\SysPulse.active");
	fs::remove(sysServices + "\\TrustedInstaller.active");
	fs::remove(sysServices + "\\NetMonitor.active");
	bootMenu();
}

// =====================================================================
// MAIN
// =====================================================================

int main() {
	SetConsoleTitleA("ARSLANIUS 28 Beta 2");

	SetConsoleWidthOnly(120);

	initPaths();
	ensureDirectories();

	bootMenu();

	return 0;
};