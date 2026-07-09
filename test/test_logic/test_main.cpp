// Host-side unit tests for include/InfoTermLogic.h (run: pio test -e native).
// These cover the exact functions the firmware calls, not copies of them.
#include <unity.h>
#include <string>
#include "InfoTermLogic.h"

using namespace infoterm;

// Sunrise 05:16 and sunset 21:25 as minutes-of-day (the values the device
// reported live during the night-mode hardware test).
static const int SUNRISE = 5 * 60 + 16;   // 316
static const int SUNSET  = 21 * 60 + 25;  // 1285

void test_night_after_sunset() {
  TEST_ASSERT_TRUE(isNightWindow(23 * 60, SUNRISE, SUNSET));   // 23:00
}

void test_day_at_noon() {
  TEST_ASSERT_FALSE(isNightWindow(12 * 60, SUNRISE, SUNSET));  // 12:00
}

void test_night_before_sunrise() {
  TEST_ASSERT_TRUE(isNightWindow(1 * 60, SUNRISE, SUNSET));    // 01:00
}

void test_sunrise_edge_is_day() {
  TEST_ASSERT_FALSE(isNightWindow(SUNRISE, SUNRISE, SUNSET));  // exclusive
}

void test_sunset_edge_is_night() {
  TEST_ASSERT_TRUE(isNightWindow(SUNSET, SUNRISE, SUNSET));    // inclusive
}

void test_unknown_suntimes_are_day() {
  TEST_ASSERT_FALSE(isNightWindow(23 * 60, -1, SUNSET));
  TEST_ASSERT_FALSE(isNightWindow(23 * 60, SUNRISE, -1));
}

void test_mix_t0_keeps_base() {
  // t=0 ignores the target color entirely.
  TEST_ASSERT_EQUAL_UINT16(0x0000, mixRgb565(0x0000, 255, 255, 255, 0.0f));
  TEST_ASSERT_EQUAL_UINT16(0xFFFF, mixRgb565(0xFFFF, 0, 0, 0, 0.0f));
}

void test_mix_t1_reaches_target() {
  // t=1 fully adopts the target color; toward-black => pure black.
  TEST_ASSERT_EQUAL_UINT16(0x0000, mixRgb565(0xFFFF, 0, 0, 0, 1.0f));
}

void test_mix_night_bg_is_black() {
  // Night mode drives every background to TFT_BLACK via a toward-black blend.
  TEST_ASSERT_EQUAL_UINT16(0x0000, mixRgb565(0xF800 /*red*/, 0, 0, 0, 1.0f));
}

void test_htmlescape_covers_all_meta_chars() {
  TEST_ASSERT_EQUAL_STRING("&amp;&lt;&gt;&quot;&#39;",
                           htmlEscaped<std::string>("&<>\"'").c_str());
}

void test_htmlescape_blocks_singlequote_attribute_breakout() {
  // The stored-XSS shape from the July-2026 review: a stored value trying
  // to escape a single-quoted attribute like value='...'.
  TEST_ASSERT_EQUAL_STRING(
      "x&#39; onmouseover=&#39;alert(1)",
      htmlEscaped<std::string>("x' onmouseover='alert(1)").c_str());
}

void test_htmlescape_passes_plain_text_through() {
  TEST_ASSERT_EQUAL_STRING("Wetter 21.5 C_-~",
                           htmlEscaped<std::string>("Wetter 21.5 C_-~").c_str());
  TEST_ASSERT_EQUAL_STRING("", htmlEscaped<std::string>("").c_str());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_night_after_sunset);
  RUN_TEST(test_day_at_noon);
  RUN_TEST(test_night_before_sunrise);
  RUN_TEST(test_sunrise_edge_is_day);
  RUN_TEST(test_sunset_edge_is_night);
  RUN_TEST(test_unknown_suntimes_are_day);
  RUN_TEST(test_mix_t0_keeps_base);
  RUN_TEST(test_mix_t1_reaches_target);
  RUN_TEST(test_mix_night_bg_is_black);
  RUN_TEST(test_htmlescape_covers_all_meta_chars);
  RUN_TEST(test_htmlescape_blocks_singlequote_attribute_breakout);
  RUN_TEST(test_htmlescape_passes_plain_text_through);
  return UNITY_END();
}
