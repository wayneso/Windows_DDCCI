#pragma comment(lib, "dxva2.lib")
#include <windows.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <highlevelmonitorconfigurationapi.h>

using namespace std;

int main()
{
    HWND hWnd = GetDesktopWindow();
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
    cout << "Monitor: " << hMonitor << endl;

    DWORD cPhysicalMonitors;
    BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &cPhysicalMonitors);
    cout << "GetNumber: " << bSuccess << ", number of physical monitors: " << cPhysicalMonitors << endl;

    LPPHYSICAL_MONITOR pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));
    bSuccess = GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors);
    cout << "GetPhysicalMonitor: " << bSuccess << endl
        << "Handle: " << pPhysicalMonitors->hPhysicalMonitor << endl
        << "Description: ";
    wcout << (WCHAR*)(pPhysicalMonitors->szPhysicalMonitorDescription);

    DWORD MinimumBrightness;
    DWORD CurrentBrightness;
    DWORD MaximumBrightness;
    BOOL err = GetMonitorBrightness(pPhysicalMonitors->hPhysicalMonitor, &MinimumBrightness, &CurrentBrightness, &MaximumBrightness);

    err = SetMonitorBrightness(pPhysicalMonitors->hPhysicalMonitor, 50);

    int err1 = GetLastError();
    DWORD MonitorCapabilities;
    DWORD SupportedColorTemperatures;
    err = GetMonitorCapabilities(pPhysicalMonitors->hPhysicalMonitor, &MonitorCapabilities, &SupportedColorTemperatures);
    
    DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
    free(pPhysicalMonitors);

}