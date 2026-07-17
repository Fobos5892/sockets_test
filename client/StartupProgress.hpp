#pragma once

namespace client_startup {

constexpr int kProgressMin = 0;
constexpr int kProgressMax = 100;

constexpr int kResolveConfigPercent = 5;
constexpr int kReadConfigBeginPercent = 10;
constexpr int kConfigReadBandWidth = 25;  // maps file 0..100 into +0..25 after begin
constexpr int kConfigReadyPercent = 40;
constexpr int kConnectingPercent = 45;
constexpr int kTcpConnectedPercent = 60;
constexpr int kWaitAssignIdPercent = 75;
constexpr int kGotIdPercent = 85;
constexpr int kRegisteringPercent = 92;
constexpr int kCompletePercent = 100;

inline int map_config_file_percent(int file_percent) {
    return kReadConfigBeginPercent + (file_percent * kConfigReadBandWidth) / kProgressMax;
}

}  // namespace client_startup
