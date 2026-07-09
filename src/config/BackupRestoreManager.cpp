#include "BackupRestoreManager.h"
#include "../../include/Version.h"

// Delegates to the configuration JSON serializer in
// src/storage/StorageBackupRuntime.inc, compiled as part of the InfoTerm.cpp
// unity build with external linkage.
extern bool serializeConfigurationToFile(const char* path);
extern bool deserializeConfigurationFromFile(const char* path);

namespace InfoTermBackupRestoreManager{
bool exportBackup(const char* path){return serializeConfigurationToFile(path);}
bool importBackup(const char* path){return deserializeConfigurationFromFile(path);}
bool createAutomaticBackup(const char* path){return serializeConfigurationToFile(path);}
BackupInfo inspectBackup(const char*){return {INFOTERM_APP_VERSION,2,"",false};}
}
