#pragma once
#include <Arduino.h>
namespace InfoTermBackupRestoreManager{
struct BackupInfo{String firmwareVersion; uint16_t schemaVersion; String timestamp; bool valid;};
bool exportBackup(const char* path);
bool importBackup(const char* path);
bool createAutomaticBackup(const char* path);
BackupInfo inspectBackup(const char* path);
}
