
// minimal wallpaper.cpp

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

HWND g_workerw = nullptr;

std::vector<HWND> g_windows;
std::vector<HANDLE> g_processes;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND shellView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shellView)
    {
        HWND *ret = (HWND *)lParam;
        *ret = FindWindowExW(hwnd, nullptr, L"WorkerW", nullptr);
        return FALSE;
    }
    return TRUE;
}

HWND get_workerw()
{
    HWND progman = FindWindowW(L"Progman", nullptr);

    // msg, spawn workerw
    SendMessageW(progman, 0x052C, 0, 0);

    HWND workerw = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);

    if (!workerw)
    {
        std::cerr << "cant find workerw, falling back to progman\n";
        workerw = progman;
    }
    return workerw;
}

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMon,
    HDC,
    LPRECT lprcMonitor,
    LPARAM lParam)
{
    std::vector<RECT> *monitors = (std::vector<RECT> *)lParam;
    monitors->push_back(*lprcMonitor);
    return TRUE;
}

void cleanup()
{
    // kill mpv processes
    for (HANDLE proc : g_processes)
    {
        if (proc)
        {
            TerminateProcess(proc, 0);
            CloseHandle(proc);
        }
    }
    g_processes.clear();

    // destroy wallpaper windows
    for (HWND wnd : g_windows)
    {
        if (wnd && IsWindow(wnd))
        {
            DestroyWindow(wnd);
        }
    }
    g_windows.clear();
}

void set_wallpaper(const std::string &videoPath)
{
    // remove old wallpaper first
    cleanup();

    HWND workerw = get_workerw();
    if (!workerw)
        return;

    // get all monitors
    std::vector<RECT> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);

    for (auto &m : monitors)
    {
        HWND wnd = CreateWindowExW(
            WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            m.left,
            m.top,
            m.right - m.left,
            m.bottom - m.top,
            workerw,
            NULL,
            GetModuleHandle(nullptr),
            NULL);

        if (!wnd)
        {
            std::cerr << "failed to create wallpaper window for monitors\n";
            return;
        }

        g_windows.push_back(wnd);

        std::string cmd = "mpv --wid=" + std::to_string((uintptr_t)wnd) + " --loop --no-border --no-audio --no-input-default-bindings --input-cursor=no \"" + videoPath + "\"";

        std::cout << "launching: " << cmd << "\n";

        // create process to launch mpv
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;

        char cmdBuffer[1024];
        strcpy(cmdBuffer, cmd.c_str()); // CreateProcess needs mutable string

        BOOL success = CreateProcessA(
            NULL,
            cmdBuffer,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi);

        if (!success)
        {
            std::cout << "Failed to launch mpv for window " << wnd << "\n";
            return;
        }

        g_processes.push_back(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int main()
{
    HANDLE pipe = CreateNamedPipeA(
        "\\\\.\\pipe\\wallpaper_engine",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024,
        1024,
        0,
        NULL);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        std::cerr << "failed to create pipe\n";
        return 1;
    }
    std::cout << "wallpaper engine running\n";

    while (true)
    {
        BOOL connected = ConnectNamedPipe(pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected)
            continue;

        char buffer[1024];
        DWORD bytesRead = 0;

        if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            buffer[bytesRead] = '\0';

            std::string cmd(buffer);

            if (cmd.rfind("set ", 0) == 0)
            {
                std::string file = cmd.substr(4);
                set_wallpaper(file);
            }
            else if (cmd == "stop")
            {
                cleanup();
            }
            else if (cmd == "exit")
            {
                cleanup();
                break;
            }
        }
        DisconnectNamedPipe(pipe);
    }
    CloseHandle(pipe);
    return 0;
}
