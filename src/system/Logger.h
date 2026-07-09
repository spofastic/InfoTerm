#pragma once

#include <Arduino.h>

class Logger {
public:
  enum class Level : uint8_t {
    Error = 0,
    Warning = 1,
    Info = 2,
    Debug = 3
  };

  static void begin(unsigned long baudRate = 115200);
  static void setLevel(Level level);
  static void setLevel(int level);
  static Level level();
  static int levelValue();
  static Level levelFromValue(int value);
  static const char* levelName(Level level);
  static const char* levelKey(Level level);
  static bool isEnabled(Level level);

  static void error(const String& message);
  static void error(const char* message);
  static void warning(const String& message);
  static void warning(const char* message);
  static void info(const String& message);
  static void info(const char* message);
  static void debug(const String& message);
  static void debug(const char* message);

  static void log(Level level, const String& message);
  static const String& lastMessage();
  static Level lastLevel();
  static unsigned long lastMessageMs();

  // In-RAM ring of the most recent emitted log lines (only lines that passed
  // the active level filter, i.e. exactly what Serial showed). Backs the
  // WebGUI log view; ~50 Strings of typical log length stay well under 8 KB.
  static constexpr int HISTORY_SIZE = 50;
  struct HistoryEntry {
    Level level;
    unsigned long ms;
    String message;
  };
  static int historyCount();
  // index 0 = newest entry, historyCount()-1 = oldest. The reference stays
  // valid only until the next log() call; index must be < historyCount().
  static const HistoryEntry& historyAt(int index);

private:
  static void writeSerial(Level level, const String& message);
};
