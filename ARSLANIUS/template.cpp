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
    
	string TEMP;
	string path = (TEMP = string(api->get_config_path())) + "\\SAM";
    if (api->file_exists(path.c_str())) {
        api->print("  SAM: EXISTS\n");
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
    api->print_slow("  print_slow() - typewriter effect\n", 50);
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