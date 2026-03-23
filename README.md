# PrivEsc

> [!WARNING]
> This project is intended strictly for educational purposes, security research, and authorized penetration testing activities. Unauthorized use against systems without explicit permission may violate applicable laws and regulations.

## Overview

**PrivEsc** is a C++ privilege escalation utility for Windows x64 environments. It implements a staged execution model to transition process context across Windows Integrity Levels:

* Execute a process as **Administrator** from a **standard user**
* Execute a process as **NT AUTHORITY\SYSTEM** from an **Administrator**

The implementation directly leverages Windows internals, including token inspection, privilege adjustment, registry manipulation, and primary token duplication.

This project is an adapted implementation of `S12cybersecurity/PrivilegeEscalationClass`, maintained by **@undefinedable**.

---

## Architecture

The core logic is implemented in `PrivEscalationClass`, responsible for:

* Integrity Level evaluation using `TokenElevation`
* Automatic privilege adjustment (`SeDebugPrivilege`)
* Registry-based execution redirection
* Process enumeration and SYSTEM token acquisition
* Primary token duplication and process creation

---

## Privilege Escalation Flow

### 1. Initial Context Evaluation

* Queries current process token via `OpenProcessToken`
* Uses `GetTokenInformation(TokenElevation)` to determine elevation state
* Resolves current identity via token SID lookup

---

### 2. User → Administrator (High Integrity)

* Writes to:

  ```
  HKCU\Software\Classes\ms-settings\Shell\Open\command
  ```

* Sets:

  * Default value → target executable path
  * `DelegateExecute` → empty string

* Launches:

  ```
  C:\Windows\System32\fodhelper.exe
  ```

* Waits briefly for execution and removes registry keys via `RegDeleteKeyW`

---

### 3. Administrator → NT AUTHORITY\SYSTEM

* Ensures `SeDebugPrivilege` is enabled via `AdjustTokenPrivileges`
* Enumerates processes using `CreateToolhelp32Snapshot`
* Identifies processes owned by `NT AUTHORITY\SYSTEM`
* Retrieves access tokens using `OpenProcessToken`

---

### 4. Primary Token Duplication

* Duplicates SYSTEM token:

  ```
  DuplicateTokenEx(..., TokenPrimary)
  ```

* Spawns target process:

  ```
  CreateProcessWithTokenW(...)
  ```

---

## Key Components

### `EnableDebugPrivilege()`

* Enables `SeDebugPrivilege` on the current process token
* Invoked during class construction and prior to SYSTEM escalation

---

### `runProcAsAdminFromUser(std::wstring procName)`

* Implements registry-based execution redirection
* Triggers execution via `fodhelper.exe`
* Cleans registry artifacts post-execution

---

### `runProcAsSystemFromAdmin(std::wstring procName)`

* Enumerates SYSTEM processes
* Extracts and duplicates primary tokens
* Executes target binary under SYSTEM context

---

### `SelfElevate()`

* Retrieves current executable path
* Re-launches itself through staged escalation:

  * User → Administrator
  * Administrator → SYSTEM
* Terminates original process after successful transition

---

### `GetProcessUserName(DWORD pid)`

* Resolves process owner using `TokenUser`
* Converts SID to `DOMAIN\USERNAME` via `LookupAccountSidW`

---

### `createProcess(HANDLE token, LPCWSTR app)`

* Creates a process using a duplicated primary token
* Uses `CreateProcessWithTokenW`

---

## Usage

### Build

* Target: Windows x64
* Required libraries:

  * `advapi32.lib`
  * `user32.lib`
  * `shell32.lib`

---

### Execution

```id="3o2q5k"
PrivEsc.exe [optional_target_path]
```

* Default target:

  ```
  C:\Windows\System32\cmd.exe
  ```

---

## Behavior

* Automatically enables `SeDebugPrivilege` during initialization
* Determines execution path based on current Integrity Level
* Supports staged self-relaunch via `SelfElevate()`
* Cleans registry keys used during elevation
* Executes target process under highest attainable context
* Displays execution identity via `MessageBoxW`

---

## Security Considerations

* Requires write access to the current user registry hive
* SYSTEM escalation depends on:

  * Availability of accessible SYSTEM processes
  * Successful privilege adjustment (`SeDebugPrivilege`)
* Token access may be restricted in hardened environments

---

## Limitations

* May fail in environments with:

  * Restricted token access
  * Process protection mechanisms
  * Endpoint detection and response (EDR)

* Assumes sufficient rights to:

  * Query process tokens
  * Adjust privileges

---

## Attribution

* Adapted from: `S12cybersecurity/PrivilegeEscalationClass`
* Maintained by: **@undefinedable**
