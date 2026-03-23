# PrivEsc

> [!WARNING]
> This project is intended strictly for educational purposes, security research, and authorized penetration testing activities. Unauthorized use against systems without explicit permission may violate applicable laws and regulations.

## Overview

**PrivEsc** is a C++ privilege escalation utility for Windows x64 environments. It provides a minimal abstraction to transition execution context across Windows Integrity Levels:

* Execute a process as **Administrator** from a **standard user**
* Execute a process as **NT AUTHORITY\SYSTEM** from an **Administrator**

The implementation focuses on Windows internals, including token manipulation, privilege adjustment, and process creation using elevated security contexts.

This project is an adapted implementation of `S12cybersecurity/PrivilegeEscalationClass`, maintained by **@undefinedable**.

---

## Core Functionality

### 1. User → Administrator

* Detects current Integrity Level using `TokenElevation`
* Performs elevation to high integrity
* Executes a specified binary with administrative privileges

---

### 2. Administrator → NT AUTHORITY\SYSTEM

* Enables `SeDebugPrivilege`
* Enumerates active processes
* Identifies processes running under `NT AUTHORITY\SYSTEM`
* Extracts and duplicates their primary tokens
* Spawns a new process using the duplicated SYSTEM token

---

## Key Concepts

* **Integrity Levels**: Determines execution context (User, High, SYSTEM)
* **SeDebugPrivilege**: Required to access and duplicate tokens of privileged processes
* **Primary Token Duplication**: Achieved via `DuplicateTokenEx`
* **Process Creation with Token**: Implemented using `CreateProcessWithTokenW`

---

## Usage

### Build

* Target: Windows x64
* Link against:

  * `advapi32.lib`
  * `user32.lib`
  * `shell32.lib`

---

### Run

```
PrivEsc.exe [optional_target_path]
```

* Default target:

  ```
  C:\Windows\System32\cmd.exe
  ```

---

## Behavior

* Automatically determines current privilege level
* Executes appropriate escalation path:

  * User → Administrator
  * Administrator → SYSTEM
* Launches the target process under the highest achievable context
* Optionally displays execution identity via `MessageBoxW`

---

## Notes

> [!IMPORTANT]
> SYSTEM execution depends on successful acquisition and duplication of a valid SYSTEM process token.

> [!CAUTION]
> Failure to enable `SeDebugPrivilege` will prevent access to privileged process tokens.

---

## Attribution

* Adapted from: `S12cybersecurity/PrivilegeEscalationClass`
* Maintained by: **@undefinedable**
