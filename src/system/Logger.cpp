#include "Logger.h"

namespace {
Logger::Level currentLevel = Logger::Level::Info;
Logger::Level currentLastLevel = Logger::Level::Info;
String currentLastMessage;
unsigned long currentLastMessageMs = 0;
bool serialStarted = false;
Logger::HistoryEntry historyRing[Logger::HISTORY_SIZE];
int historyNext = 0;  // slot the next entry is written to
int historyFill = 0;
}

void Logger::begin(unsigned long baudRate) {
  if (!serialStarted) {
    Serial.begin(baudRate);
    serialStarted = true;
  }
}

void Logger::setLevel(Level level) {
  currentLevel = level;
}

void Logger::setLevel(int level) {
  currentLevel = levelFromValue(level);
}

Logger::Level Logger::level() {
  return currentLevel;
}

int Logger::levelValue() {
  return static_cast<int>(currentLevel);
}

Logger::Level Logger::levelFromValue(int value) {
  if (value <= static_cast<int>(Level::Error)) return Level::Error;
  if (value == static_cast<int>(Level::Warning)) return Level::Warning;
  if (value == static_cast<int>(Level::Debug)) return Level::Debug;
  return Level::Info;
}

const char* Logger::levelName(Level level) {
  switch (level) {
    case Level::Error: return "ERROR";
    case Level::Warning: return "WARNING";
    case Level::Info: return "INFO";
    case Level::Debug: return "DEBUG";
    default: return "INFO";
  }
}

const char* Logger::levelKey(Level level) {
  switch (level) {
    case Level::Error: return "error";
    case Level::Warning: return "warning";
    case Level::Info: return "info";
    case Level::Debug: return "debug";
    default: return "info";
  }
}

bool Logger::isEnabled(Level level) {
  return static_cast<int>(level) <= static_cast<int>(currentLevel);
}

void Logger::error(const String& message) {
  log(Level::Error, message);
}

void Logger::error(const char* message) {
  log(Level::Error, String(message));
}

void Logger::warning(const String& message) {
  log(Level::Warning, message);
}

void Logger::warning(const char* message) {
  log(Level::Warning, String(message));
}

void Logger::info(const String& message) {
  log(Level::Info, message);
}

void Logger::info(const char* message) {
  log(Level::Info, String(message));
}

void Logger::debug(const String& message) {
  log(Level::Debug, message);
}

void Logger::debug(const char* message) {
  log(Level::Debug, String(message));
}

void Logger::log(Level level, const String& message) {
  currentLastLevel = level;
  currentLastMessage = message;
  currentLastMessageMs = millis();

  if (!isEnabled(level)) {
    return;
  }

  HistoryEntry& slot = historyRing[historyNext];
  slot.level = level;
  slot.ms = currentLastMessageMs;
  slot.message = message;
  historyNext = (historyNext + 1) % HISTORY_SIZE;
  if (historyFill < HISTORY_SIZE) historyFill++;

  begin();
  writeSerial(level, message);
}

int Logger::historyCount() {
  return historyFill;
}

const Logger::HistoryEntry& Logger::historyAt(int index) {
  int pos = (historyNext - 1 - index + 2 * HISTORY_SIZE) % HISTORY_SIZE;
  return historyRing[pos];
}

const String& Logger::lastMessage() {
  return currentLastMessage;
}

Logger::Level Logger::lastLevel() {
  return currentLastLevel;
}

unsigned long Logger::lastMessageMs() {
  return currentLastMessageMs;
}

void Logger::writeSerial(Level level, const String& message) {
  Serial.print("[");
  Serial.print(levelName(level));
  Serial.print("] ");
  Serial.println(message);
}
