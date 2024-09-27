#pragma comment(lib, "dxva2.lib")
#include <atlstr.h>
#include <PhysicalMonitorEnumerationAPI.h>
#include <LowLevelMonitorConfigurationAPI.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>

// 存储物理显示器的句柄
std::vector<PHYSICAL_MONITOR> physicalMonitors{};

// 枚举监视器的回调函数
BOOL CALLBACK monitorEnumProcCallback(HMONITOR hMonitor, HDC hDeviceContext, LPRECT rect, LPARAM data)
{
    DWORD numberOfPhysicalMonitors;
    BOOL success = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numberOfPhysicalMonitors);
    if (success) {
        auto originalSize = physicalMonitors.size();
        physicalMonitors.resize(physicalMonitors.size() + numberOfPhysicalMonitors);
        success = GetPhysicalMonitorsFromHMONITOR(hMonitor, numberOfPhysicalMonitors, physicalMonitors.data() + originalSize);
    }
    return true;
}

// 将宽字符转换为UTF-8格式
std::string toUtf8(wchar_t* buffer)
{
    CW2A utf8(buffer, CP_UTF8);
    const char* data = utf8.m_psz;
    return std::string{ data };
}

// 打印用法信息
int printUsage()
{
    std::cout << "用法: winddcutil 命令 [<arg> ...]" << std::endl
        << "命令:" << std::endl
        << "\thelp                                           显示帮助" << std::endl
        << "\tdetect                                         检测监视器" << std::endl
        << "\tcapabilities <display-id>                     查询监视器能力" << std::endl
        << "\tgetvcp <display-id> <feature-code>            报告VCP特性值" << std::endl
        << "\tsetvcp <display-id> <feature-code> <new-value> 设置VCP特性值" << std::endl
        << "\texit                                          退出程序" << std::endl;
    return 1;
}

// 检测并打印物理监视器信息
int detect()
{
    int i = 0;
    for (auto& physicalMonitor : physicalMonitors) {
        std::cout << "显示器 " << i << ":\t" << toUtf8(physicalMonitor.szPhysicalMonitorDescription) << std::endl;
        i++;
    }
    return 0;
}

// 查询监视器的能力
int capabilities(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        std::cerr << "需要显示 ID" << std::endl;
        return printUsage();
    }

    size_t displayId = std::stoi(args[0]);

    if (displayId >= physicalMonitors.size()) {
        std::cerr << "无效的显示 ID" << std::endl;
        return 1;
    }

    auto physicalMonitorHandle = physicalMonitors[displayId].hPhysicalMonitor;

    DWORD capabilitiesStringLengthInCharacters;
    auto success = GetCapabilitiesStringLength(physicalMonitorHandle, &capabilitiesStringLengthInCharacters);
    if (!success) {
        std::cerr << "获取能力字符串长度失败" << std::endl;
        return 1;
    }

    std::unique_ptr<char[]> capabilitiesString{ new char[capabilitiesStringLengthInCharacters] };
    success = CapabilitiesRequestAndCapabilitiesReply(physicalMonitorHandle, capabilitiesString.get(), capabilitiesStringLengthInCharacters);
    if (!success) {
        std::cerr << "获取能力字符串失败" << std::endl;
        return 1;
    }

    std::cout << std::string(capabilitiesString.get()) << std::endl;

    return 0;
}

// 获取VCP特性值
int getVcp(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "需要显示 ID 和特性代码" << std::endl;
        return printUsage();
    }

    size_t displayId = std::stoi(args[0]);
    BYTE vcpCode = std::stoul(args[1], nullptr, 16);

    if (displayId >= physicalMonitors.size()) {
        std::cerr << "无效的显示 ID" << std::endl;
        return 1;
    }

    auto physicalMonitorHandle = physicalMonitors[displayId].hPhysicalMonitor;

    DWORD currentValue;
    bool success = GetVCPFeatureAndVCPFeatureReply(physicalMonitorHandle, vcpCode, NULL, &currentValue, NULL);
    if (!success) {
        std::cerr << "获取VCP代码值失败" << std::endl;
        return 1;
    }

    std::cout << "VCP " << args[1] << " 值: " << std::hex << currentValue << std::endl;

    return 0;
}

// 设置VCP特性值
int setVcp(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "参数数量无效" << std::endl;
        return printUsage();
    }

    size_t displayId = std::stoi(args[0]);
    BYTE vcpCode = std::stoul(args[1], nullptr, 16);
    DWORD newValue = std::stoul(args[2], nullptr, 16);

    if (displayId >= physicalMonitors.size()) {
        std::cerr << "无效的显示 ID" << std::endl;
        return 1;
    }

    bool success = SetVCPFeature(physicalMonitors[displayId].hPhysicalMonitor, vcpCode, newValue);
    if (!success) {
        std::cerr << "设置VCP特性失败" << std::endl;
        return 1;
    }
    return 0;
}

// 命令映射
std::unordered_map<std::string, std::function<int(const std::vector<std::string>&)>> commands
{
    { "help", [](const auto&) { return printUsage(); } },
    { "detect", [](const auto&) { return detect(); } },
    { "capabilities", capabilities },
    { "getvcp", getVcp },
    { "setvcp", setVcp },
};

// 主函数
int main()
{
    // 枚举显示器
    EnumDisplayMonitors(NULL, NULL, &monitorEnumProcCallback, 0);

    std::string input;
    while (true) {
        printUsage();
        std::cout << "请输入命令 (输入 'exit' 退出): ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break; // 退出循环
        }

        std::istringstream iss(input);
        std::string command;
        std::vector<std::string> args;
        while (iss >> command) {
            args.push_back(command);
        }

        if (args.empty()) {
            printUsage();
            continue;
        }

        auto commandFunc = commands.find(args[0]);
        if (commandFunc == commands.end()) {
            std::cerr << "未知命令" << std::endl;
            printUsage();
            continue;
        }
        args.erase(args.begin());

        // 执行命令
        commandFunc->second(args);
    }

    // 清理物理监视器
    DestroyPhysicalMonitors(physicalMonitors.size(), physicalMonitors.data());

    return 0;
}
