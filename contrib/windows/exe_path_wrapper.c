#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>

/* The maximum path length we'll allow */
#define MAX_PATH_LEN 1920

/* PATH_ENTRIES is our simulated RPATH, usually of the form 'TEXT("../path1"), TEXT("../path2"), ...' */
#ifndef PATH_ENTRIES
#define PATH_ENTRIES  TEXT("")
#endif

LPWSTR pathEntries[] = {
    PATH_ENTRIES
};

/* JULIA_EXE_PATH is the relative path to julia.exe */
#ifndef JULIA_EXE_PATH
#define JULIA_EXE_PATH "../libexec/julia.exe"
#endif

int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
    // Determine absolute path to true julia.exe sitting in `libexec/`
    WCHAR currFileDir[MAX_PATH_LEN];
    WCHAR juliaPath[MAX_PATH_LEN];
    if (!GetModuleFileName(NULL, currFileDir, MAX_PATH_LEN)) {
        fprintf(stderr, "ERROR: GetModuleFileName() failed with code %d\n", GetLastError());
        exit(1);
    }
    PathRemoveFileSpec(currFileDir);
    PathCombine(juliaPath, currFileDir, TEXT(JULIA_EXE_PATH));

    // On windows, we simulate RPATH by pushing onto PATH
    LPWSTR pathVal = (LPWSTR) malloc(MAX_PATH_LEN*sizeof(WCHAR));
    DWORD dwRet = GetEnvironmentVariable(TEXT("PATH"), pathVal, MAX_PATH_LEN);
    DWORD numPathEntries = sizeof(pathEntries)/sizeof(pathEntries[0]);
    if (dwRet == 0) {
        // If we cannot get PATH, then our job is easy!
        pathVal[0] = '\0';
    }
    else {
        // Otherwise, we append, if we have enough space to:
        DWORD currFileDirLen = wcslen(currFileDir);
        DWORD totalPathLen = dwRet + 1 + currFileDirLen;
        int env_idx;
        for (env_idx = 0; env_idx < numPathEntries; ++env_idx) {
            totalPathLen += 1 + currFileDirLen + 1 + wcslen(pathEntries[env_idx]);
        }
        if (MAX_PATH_LEN < totalPathLen) {
            fprintf(stderr, "ERROR: Cannot append entries to PATH: not enough space in environment block.  Reduce size of PATH!");
            exit(1);
        }
        lstrcat(pathVal, TEXT(";"));
    }
    // We always add the current directory (e.g. `bin/`) to PATH so that we can find e.g. libjulia.dll
    lstrcat(pathVal, currFileDir);

    // For each entry in PATH_ENTRIES, tack it on to the end, relative to the current directory:
    int env_idx;
    for (env_idx = 0; env_idx < numPathEntries; ++env_idx) {
        lstrcat(pathVal, TEXT(";"));
        lstrcat(pathVal, currFileDir);
        lstrcat(pathVal, TEXT("\\"));
        lstrcat(pathVal, pathEntries[env_idx]);
    }
    SetEnvironmentVariable(TEXT("PATH"), pathVal);
    free(pathVal);

    STARTUPINFO info;
    PROCESS_INFORMATION processInfo;
    DWORD exit_code = 1;
    GetStartupInfo(&info);
    if (CreateProcess(juliaPath, GetCommandLine(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        GetExitCodeProcess(processInfo.hProcess, &exit_code);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    return exit_code;
}
