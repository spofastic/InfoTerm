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

// --- RSS text pipeline (infoterm::rsstext, extracted in 1.0.11 / issue #5) ---

using std::string;
namespace rt = infoterm::rsstext;

void test_rss_decode_named_entities() {
  TEST_ASSERT_EQUAL_STRING("&<>\"' ",
                           rt::decodeEntities<string>("&amp;&lt;&gt;&quot;&apos;&nbsp;").c_str());
}

void test_rss_decode_numeric_entities() {
  // decimal 228 = ä -> "ae", hex 2019 = typographic apostrophe -> '
  TEST_ASSERT_EQUAL_STRING("ae", rt::decodeEntities<string>("&#228;").c_str());
  TEST_ASSERT_EQUAL_STRING("'", rt::decodeEntities<string>("&#x2019;").c_str());
}

void test_rss_decode_keeps_unknown_and_lone_ampersand() {
  TEST_ASSERT_EQUAL_STRING("&copy;", rt::decodeEntities<string>("&copy;").c_str());
  TEST_ASSERT_EQUAL_STRING("Fisch & Chips", rt::decodeEntities<string>("Fisch & Chips").c_str());
}

void test_rss_utf8_transliteration() {
  TEST_ASSERT_EQUAL_STRING("Aerger, Groesse, Suess, EUR 5",
                           rt::utf8ToAscii<string>("Ärger, Größe, Süß, € 5").c_str());
  TEST_ASSERT_EQUAL_STRING("cafe olee", rt::utf8ToAscii<string>("café olée").c_str());
}

void test_rss_utf8_drops_unmapped_codepoints() {
  // CJK has no ASCII stand-in and is dropped; ASCII around it survives.
  TEST_ASSERT_EQUAL_STRING("x y", rt::utf8ToAscii<string>("x\xE6\x97\xA5 y").c_str());
}

void test_rss_striptags_removes_markup_keeps_cdata_content() {
  TEST_ASSERT_EQUAL_STRING("Hallo Welt",
                           rt::stripTags<string>("<![CDATA[Hallo <b>Welt</b>]]>").c_str());
}

void test_rss_striptags_linebreak_tags() {
  TEST_ASSERT_EQUAL_STRING("a\nb\nc", rt::stripTags<string>("a<br/>b</p>c").c_str());
}

void test_rss_extract_first_tag_content() {
  TEST_ASSERT_EQUAL_STRING("Titel",
                           rt::extractTag<string>("<rss><title>Titel</title><title>Zwei</title></rss>", "title").c_str());
}

void test_rss_extract_requires_tag_boundary() {
  // <subtitle> must never satisfy a search for <title>.
  TEST_ASSERT_EQUAL_STRING("",
                           rt::extractTag<string>("<subtitle>Nein</subtitle>", "title").c_str());
  TEST_ASSERT_EQUAL_STRING("Ja",
                           rt::extractTag<string>("<subtitle>Nein</subtitle><title>Ja</title>", "title").c_str());
}

void test_rss_extract_skips_selfclosing_and_handles_attributes() {
  TEST_ASSERT_EQUAL_STRING("Inhalt",
                           rt::extractTag<string>("<title/><title type='text'>Inhalt</title>", "title").c_str());
}

void test_rss_extract_missing_close_returns_empty() {
  TEST_ASSERT_EQUAL_STRING("", rt::extractTag<string>("<title>offen", "title").c_str());
}

void test_rss_cleantext_full_pipeline() {
  // Tags (also entity-encoded ones), entities, UTF-8 and whitespace runs in
  // one pass - the exact shape RSS descriptions arrive in.
  TEST_ASSERT_EQUAL_STRING(
      "Schnee &\nEis:\n-5 Grad",
      rt::cleanText<string>("<![CDATA[<p>Schnee &amp;</p>  &lt;b&gt;Eis:&lt;/b&gt;\r\n\r\n -5\tGrad  ]]>", 200).c_str());
}

void test_rss_cleantext_truncates_with_ellipsis() {
  TEST_ASSERT_EQUAL_STRING("abcde...", rt::cleanText<string>("abcdefgh", 5).c_str());
  TEST_ASSERT_EQUAL_STRING("abc", rt::cleanText<string>("abc", 5).c_str());
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
  RUN_TEST(test_rss_decode_named_entities);
  RUN_TEST(test_rss_decode_numeric_entities);
  RUN_TEST(test_rss_decode_keeps_unknown_and_lone_ampersand);
  RUN_TEST(test_rss_utf8_transliteration);
  RUN_TEST(test_rss_utf8_drops_unmapped_codepoints);
  RUN_TEST(test_rss_striptags_removes_markup_keeps_cdata_content);
  RUN_TEST(test_rss_striptags_linebreak_tags);
  RUN_TEST(test_rss_extract_first_tag_content);
  RUN_TEST(test_rss_extract_requires_tag_boundary);
  RUN_TEST(test_rss_extract_skips_selfclosing_and_handles_attributes);
  RUN_TEST(test_rss_extract_missing_close_returns_empty);
  RUN_TEST(test_rss_cleantext_full_pipeline);
  RUN_TEST(test_rss_cleantext_truncates_with_ellipsis);
  return UNITY_END();
}
