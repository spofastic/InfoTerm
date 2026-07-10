#pragma once
#include <Arduino.h>

/******************************************************************************
 * InfoTerm
 * Modul   : Language
 *
 * Zentrale Sprachdatei fuer alle sichtbaren Texte der Weboberflaeche und
 * Anzeige. Reihenfolge der Sprachen: Deutsch, Englisch, Franzoesisch,
 * Spanisch, Italienisch, Russisch, Hindi, Chinesisch.
 *
 * Wichtig: Russisch, Hindi und Chinesisch nutzen nicht-lateinische Schriften,
 * die die eingebetteten TFT-Fonts nicht darstellen koennen. Deshalb gibt es
 * zwei Zugriffe:
 *   TXT()   -> display-sicher: faellt fuer ru/hi/zh auf Englisch zurueck.
 *   TXTW()  -> volle Sprache: nur fuer die WebGUI (Browser rendert Unicode).
 * Die Anzeige-Codepfade rufen weiterhin TXT(); nur WebGuiRuntime nutzt TXTW().
 ******************************************************************************/

enum UiLanguage {
  LANG_DE = 0,
  LANG_EN = 1,
  LANG_FR = 2,
  LANG_ES = 3,
  LANG_IT = 4,
  LANG_RU = 5,
  LANG_HI = 6,
  LANG_ZH = 7,
  LANG_COUNT = 8
};

extern UiLanguage uiLanguage;

enum LanguageKey {
  L_TAB_START,
  L_TAB_WEATHER,
  L_TAB_MQTT,
  L_TAB_STATUS,
  L_WIDGET_WEEK,
  L_WIDGET_RAIN,
  L_WIDGET_OUTDOOR,
  L_WIDGET_KP,
  L_WIDGET_UV,
  L_WIDGET_WIND,
  L_WIDGET_SUN,
  L_SUN,
  L_SUNRISE,
  L_SUNSET,
  L_WEATHER_OUTDOOR,
  L_WEATHER_RAIN,
  L_WEATHER_UV,
  L_WEATHER_WIND,
  L_WEATHER_KP,
  L_STATUS_WIFI,
  L_STATUS_SIGNAL,
  L_STATUS_IP,
  L_STATUS_UPTIME,
  L_STATUS_NETWORK,
  L_STATUS_FIRMWARE,
  L_STATUS_VERSION,
  L_STATUS_BROKER,
  L_STATUS_SYNC,
  L_STATUS_ENABLED,
  L_STATUS_DISABLED,
  L_STATUS_ONLINE,
  L_STATUS_OFFLINE,
  L_STATUS_MQTT_OFF,
  L_BOOT_SYNCING_TIME,
  L_WEB_INTRO,
  L_WEB_LANGUAGE,
  L_WEB_LANGUAGE_DE,
  L_WEB_LANGUAGE_EN,
  L_WEB_MQTT_WEATHER_TITLE,
  L_WEB_MQTT_WEATHER_DESC,
  L_WEB_USE_MQTT_WEATHER,
  L_WEB_USE_OPENMETEO_FALLBACK,
  L_WEB_BROKER,
  L_WEB_PORT,
  L_WEB_USER,
  L_WEB_PASSWORD,
  L_WEB_OBSERVATION_TOPIC,
  L_WEB_THEME_TITLE,
  L_WEB_THEME_DESC,
  L_WEB_ACCENT,
  L_WEB_BORDER_COLOR,
  L_WEB_TEXT_COLOR,
  L_WEB_THEME,
  L_WEB_SETTINGS,
  L_WEB_SETTINGS_DESC,
  L_WEB_GENERAL,
  L_WEB_DEVICE_NAME,
  L_WEB_UNITS,
  L_WEB_DATE_FORMAT,
  L_WEB_TIMEZONE,
  L_WEB_FIELD,
  L_WEB_LOCATION,
  L_WEB_LOCATION_DESC,
  L_WEB_LOCATION_NAME,
  L_WEB_LATITUDE,
  L_WEB_LONGITUDE,
  L_WEB_MQTT_VALUES_TITLE,
  L_WEB_DISPLAY_VALUES,
  L_WEB_DISPLAY_VALUES_DESC,
  L_WEB_LABEL,
  L_WEB_UNIT,
  L_WEB_TOPIC,
  L_WEB_SHOW,
  L_WEB_WIDGETS_TITLE,
  L_WEB_WIDGETS_DESC,
  L_WIDGET_EMPTY,
  L_LEVEL_LOW,
  L_LEVEL_MEDIUM,
  L_LEVEL_MODERATE,
  L_LEVEL_HIGH,
  L_LEVEL_VERY_HIGH,
  L_LEVEL_EXTREME,
  L_WEB_SAVE,
  L_WEB_SAVED_TITLE,
  L_WEB_SAVED_DESC,
  L_WEB_BACK,
  L_WEB_PASSWORD_KEEP_HINT,
  L_WEB_PASSWORD_CLEAR,
  // --- DataPoints tab ---
  L_WEB_DATAPOINTS,
  L_WEB_DP_DESC,
  L_WEB_SEARCH,
  L_WEB_DP_SEARCH_PH,
  L_WEB_SOURCE,
  L_WEB_ALL,
  L_WEB_APPLY,
  L_WEB_RESET,
  L_WEB_TOTAL,
  L_WEB_VALUE,
  L_WEB_AGE,
  L_WEB_STATUS,
  L_WEB_DP_NONE,
  L_WEB_DP_VISIBLE_PRE,
  L_WEB_DP_VISIBLE_OF,
  L_WEB_DP_VISIBLE_SUF,
  // --- Status hero ---
  L_WEB_TUNNEL_IP,
  // --- MQTT tab ---
  L_WEB_TEST_CONNECTION,
  // --- Settings tab ---
  L_WEB_WIFI_NETWORK,
  L_WEB_WIFI_CONNECTED,
  L_WEB_WIFI_IN_RANGE,
  L_WEB_WIFI_OUT_RANGE,
  L_WEB_WIFI_HINT,
  L_WEB_AUTO_SLEEP,
  L_WEB_NEVER,
  L_WEB_1_MINUTE,
  L_WEB_5_MINUTES,
  L_WEB_10_MINUTES,
  L_WEB_30_MINUTES,
  L_WEB_1_HOUR,
  L_WEB_SLEEP_HINT,
  L_WEB_TAB_CYCLE,
  L_WEB_TAB_CYCLE_HINT,
  L_WEB_EVERY_30S,
  L_WEB_EVERY_60S,
  L_WEB_EVERY_3M,
  L_WEB_EVERY_6M,
  L_WEB_UNIT_IMPERIAL,
  L_WEB_DATE_EUROPE,
  L_WEB_DATE_US,
  L_WEB_LOCATION_EXAMPLE,
  L_WEB_NETWORK_IP,
  L_WEB_IP_DESC,
  L_WEB_IP_MODE,
  L_WEB_IP_DHCP,
  L_WEB_IP_STATIC,
  L_WEB_SUBNET,
  L_WEB_DNS,
  // --- VPN panel ---
  L_WEB_VPN_DESC,
  L_WEB_UPLOAD,
  L_WEB_ENDPOINT,
  L_WEB_TUNNEL_ADDR,
  L_WEB_CONNECT,
  L_WEB_DISCONNECT,
  L_WEB_VPN_NONE,
  // --- Backup panel ---
  L_WEB_BACKUP_DESC,
  L_WEB_BACKUP_DOWNLOAD,
  L_WEB_RESTORE,
  // --- Firmware panel ---
  L_WEB_FW_TITLE,
  L_WEB_FW_CURRENT_VERSION,
  L_WEB_FW_DESC,
  L_WEB_FW_WARN,
  L_WEB_FW_UPLOAD,
  // --- Log panel ---
  L_WEB_LOG_DESC_PRE,
  L_WEB_LOG_DESC_POST,
  L_WEB_LOG_LOAD,
  // --- Design tab ---
  L_WEB_NIGHT_MODE,
  L_WEB_NIGHT_MODE_HINT,
  L_WEB_WIDGET_BORDER,
  L_WEB_WIDGET_BORDER_HINT,
  L_WEB_ROTATE,
  L_WEB_ROTATE_HINT,
  L_WEB_MENU_BG,
  L_WEB_BUTTON_BG,
  L_WEB_WIDGET_BG,
  // --- MQTT values panel ---
  L_WEB_MQTT_CFG_DESC,
  L_WEB_DECIMALS,
  L_WEB_ACTIVE,
  L_WEB_DP_DELETE_TITLE,
  L_WEB_DP_DELETE_CONFIRM,
  L_WEB_DP_ADD,
  L_WEB_MQTT_EXPORT_DESC,
  L_WEB_DP_EXPORT,
  L_WEB_DP_IMPORT,
  // --- Custom tab panels ---
  L_WEB_TAB_CFG_DESC,
  L_WEB_TAB_NAME,
  // --- JS messages ---
  L_JS_BACKUP_SELECT,
  L_JS_RESTORE_FAILED,
  L_JS_FW_SELECT,
  L_JS_FW_CONFIRM,
  L_JS_UPLOADING,
  L_JS_UPDATE_OK,
  L_JS_UPDATE_FAILED,
  L_JS_FW_UPDATE_FAILED,
  L_JS_VPN_SELECT,
  L_JS_VPN_UPLOAD_FAILED,
  L_JS_VPN_CONNECT_CONFIRM,
  L_JS_LOADING,
  L_JS_LOG_FAILED,
  L_JS_TESTING,
  L_JS_TEST_FAILED,
  L_JS_MQTT_SELECT,
  L_JS_MQTT_IMPORT_CONFIRM,
  L_JS_IMPORT_FAILED,
  // --- Handler responses ---
  L_WEB_MQTT_TEST_OK,
  L_WEB_MQTT_TEST_TIMEOUT,
  L_WEB_MQTT_TEST_UNREACHABLE,
  L_WEB_MQTT_TEST_AUTH,
  L_WEB_MQTT_TEST_UNAUTHORIZED,
  L_WEB_MQTT_TEST_CODE,
  L_WEB_EXPORT_FAILED,
  // --- Widget slot positions ---
  L_POS_TL,
  L_POS_TR,
  L_POS_ML,
  L_POS_MR,
  L_POS_BL,
  L_POS_BR,
  L_POS_UML,
  L_POS_UMR,
  L_POS_LML,
  L_POS_LMR,
  // --- File picker ---
  L_WEB_CHOOSE_FILE,
  L_WEB_NO_FILE,
  // --- Language names ---
  L_WEB_LANGUAGE_FR,
  L_WEB_LANGUAGE_ES,
  L_WEB_LANGUAGE_IT,
  L_WEB_LANGUAGE_RU,
  L_WEB_LANGUAGE_HI,
  L_WEB_LANGUAGE_ZH,
  // --- WiFi slot editor (1.0.5) ---
  L_WEB_WIFI_SLOTS_DESC,
  L_WEB_WIFI_SCAN,
  L_WEB_WIFI_SLOT_EMPTY,
  L_JS_WIFI_SCANNING,
  L_JS_WIFI_FOUND,
  L_JS_WIFI_SCAN_FAILED,
  // --- SoftAP onboarding portal (1.0.6) ---
  L_PORTAL_TITLE,
  L_PORTAL_INTRO,
  L_PORTAL_FOUND,
  L_PORTAL_PICK,
  L_PORTAL_CONNECTING,
  L_PORTAL_RESULT,
  L_PORTAL_DISP_HINT,
  L_WEB_PORTAL_START,
  L_JS_PORTAL_CONFIRM,
  L_COUNT
};

struct LanguageEntry {
  const char* t[LANG_COUNT];  // [de, en, fr, es, it, ru, hi, zh]
};

// Order per row: DE, EN, FR, ES, IT, RU, HI, ZH
static const LanguageEntry LANGUAGE_TABLE[] = {
  {{"Home", "Home", "Accueil", "Inicio", "Home", "Главная", "होम", "主页"}},
  {{"Wetter", "Weather", "Météo", "Tiempo", "Meteo", "Погода", "मौसम", "天气"}},
  {{"MQTT", "MQTT", "MQTT", "MQTT", "MQTT", "MQTT", "MQTT", "MQTT"}},
  {{"Info", "Info", "Info", "Info", "Info", "Инфо", "जानकारी", "信息"}},
  {{"Woche", "Week", "Semaine", "Semana", "Settimana", "Неделя", "सप्ताह", "周"}},
  {{"Regen", "Rain", "Pluie", "Lluvia", "Pioggia", "Дождь", "वर्षा", "降雨"}},
  {{"Temperatur", "Temperature", "Température", "Temperatura", "Temperatura", "Температура", "तापमान", "温度"}},
  {{"KP-Index", "KP index", "Indice KP", "Índice KP", "Indice KP", "Индекс KP", "KP सूचकांक", "KP 指数"}},
  {{"UV-Index", "UV index", "Indice UV", "Índice UV", "Indice UV", "УФ-индекс", "UV सूचकांक", "UV 指数"}},
  {{"Wind", "Wind", "Vent", "Viento", "Vento", "Ветер", "हवा", "风"}},
  {{"Sonne", "Sun", "Soleil", "Sol", "Sole", "Солнце", "सूर्य", "太阳"}},
  {{"Sonne", "Sun", "Soleil", "Sol", "Sole", "Солнце", "सूर्य", "太阳"}},
  {{"Sonnenaufgang", "Sunrise", "Lever du soleil", "Amanecer", "Alba", "Восход", "सूर्योदय", "日出"}},
  {{"Sonnenuntergang", "Sunset", "Coucher du soleil", "Atardecer", "Tramonto", "Закат", "सूर्यास्त", "日落"}},
  {{"Temperatur", "Temperature", "Température", "Temperatura", "Temperatura", "Температура", "तापमान", "温度"}},
  {{"Regen", "Rain", "Pluie", "Lluvia", "Pioggia", "Дождь", "वर्षा", "降雨"}},
  {{"UV-Index", "UV index", "Indice UV", "Índice UV", "Indice UV", "УФ-индекс", "UV सूचकांक", "UV 指数"}},
  {{"Wind", "Wind", "Vent", "Viento", "Vento", "Ветер", "हवा", "风"}},
  {{"KP-Index", "KP index", "Indice KP", "Índice KP", "Indice KP", "Индекс KP", "KP सूचकांक", "KP 指数"}},
  {{"WLAN", "WiFi", "WiFi", "WiFi", "WiFi", "WiFi", "वाईफ़ाई", "WiFi"}},
  {{"Signal", "Signal", "Signal", "Señal", "Segnale", "Сигнал", "सिग्नल", "信号"}},
  {{"IP-Adresse", "IP address", "Adresse IP", "Dirección IP", "Indirizzo IP", "IP-адрес", "IP पता", "IP 地址"}},
  {{"Laufzeit", "Uptime", "Fonctionnement", "Tiempo activo", "Tempo attivo", "Время работы", "अपटाइम", "运行时间"}},
  {{"Netzwerk", "Network", "Réseau", "Red", "Rete", "Сеть", "नेटवर्क", "网络"}},
  {{"Firmware", "Firmware", "Firmware", "Firmware", "Firmware", "Прошивка", "फर्मवेयर", "固件"}},
  {{"Version", "Version", "Version", "Versión", "Versione", "Версия", "संस्करण", "版本"}},
  {{"Broker", "Broker", "Broker", "Broker", "Broker", "Брокер", "ब्रोकर", "代理"}},
  {{"Synchronisierung", "Sync", "Sync", "Sinc.", "Sync", "Синх.", "सिंक", "同步"}},
  {{"Aktiviert", "Enabled", "Activé", "Activado", "Attivato", "Включено", "सक्षम", "已启用"}},
  {{"Deaktiviert", "Disabled", "Désactivé", "Desactivado", "Disattivato", "Отключено", "अक्षम", "已禁用"}},
  {{"Online", "Online", "En ligne", "En línea", "Online", "В сети", "ऑनलाइन", "在线"}},
  {{"Offline", "Offline", "Hors ligne", "Sin conexión", "Offline", "Не в сети", "ऑफ़लाइन", "离线"}},
  {{"MQTT aus", "MQTT off", "MQTT désactivé", "MQTT desactivado", "MQTT disattivato", "MQTT выкл.", "MQTT बंद", "MQTT 关闭"}},
  {{"Synchronisiere Zeit...", "Syncing time...", "Synchronisation de l'heure...", "Sincronizando hora...", "Sincronizzazione ora...", "Синхронизация времени...", "समय सिंक हो रहा है...", "正在同步时间..."}},
  {{"Passe InfoTerm mit Anzeigen, Design und praktischen Informationsfunktionen an.", "Customize InfoTerm with widgets, design and practical display features.", "Personnalisez InfoTerm avec des widgets, un design et des fonctions d'affichage pratiques.", "Personaliza InfoTerm con widgets, diseño y funciones de visualización prácticas.", "Personalizza InfoTerm con widget, design e pratiche funzioni di visualizzazione.", "Настройте InfoTerm с помощью виджетов, дизайна и практичных функций отображения.", "विजेट, डिज़ाइन और व्यावहारिक प्रदर्शन सुविधाओं के साथ InfoTerm को अनुकूलित करें।", "使用小部件、设计和实用的显示功能自定义 InfoTerm。"}},
  {{"Sprache", "Language", "Langue", "Idioma", "Lingua", "Язык", "भाषा", "语言"}},
  {{"Deutsch", "German", "Allemand", "Alemán", "Tedesco", "Немецкий", "जर्मन", "德语"}},
  {{"Englisch", "English", "Anglais", "Inglés", "Inglese", "Английский", "अंग्रेज़ी", "英语"}},
  {{"MQTT Broker / Wetter", "MQTT Broker / Weather", "MQTT Broker / Météo", "MQTT Broker / Tiempo", "MQTT Broker / Meteo", "MQTT-брокер / Погода", "MQTT ब्रोकर / मौसम", "MQTT 代理 / 天气"}},
  {{"MQTT versorgt den Wetter-Tab bevorzugt. Wenn keine frischen MQTT-Wetterdaten eintreffen, wird bei aktivierter Ersatzquelle die Standard-Wetterquelle verwendet.", "MQTT is preferred for the Weather tab. If no fresh MQTT weather data arrives and fallback is enabled, the default weather source is used.", "MQTT est prioritaire pour l'onglet Météo. Si aucune donnée météo MQTT récente n'arrive et que la source de secours est activée, la source météo par défaut est utilisée.", "MQTT tiene prioridad para la pestaña Tiempo. Si no llegan datos meteorológicos MQTT recientes y la fuente de reserva está activada, se usa la fuente meteorológica predeterminada.", "MQTT è preferito per la scheda Meteo. Se non arrivano dati meteo MQTT recenti e la sorgente di riserva è attiva, viene usata la sorgente meteo predefinita.", "MQTT имеет приоритет для вкладки Погода. Если свежие данные погоды MQTT не поступают и включён резервный источник, используется источник погоды по умолчанию.", "मौसम टैब के लिए MQTT को प्राथमिकता दी जाती है। यदि कोई ताज़ा MQTT मौसम डेटा नहीं आता और फ़ॉलबैक सक्षम है, तो डिफ़ॉल्ट मौसम स्रोत उपयोग किया जाता है।", "天气选项卡优先使用 MQTT。如果没有新的 MQTT 天气数据且启用了备用源，则使用默认天气源。"}},
  {{"MQTT-Wetterstationswerte verwenden", "Use MQTT weather station values", "Utiliser les valeurs de la station météo MQTT", "Usar valores de la estación meteorológica MQTT", "Usa i valori della stazione meteo MQTT", "Использовать значения метеостанции MQTT", "MQTT मौसम स्टेशन मान उपयोग करें", "使用 MQTT 气象站数值"}},
  {{"Open-Meteo als Ersatz verwenden, wenn MQTT nicht verfügbar ist", "Use Open-Meteo as fallback when MQTT is unavailable", "Utiliser Open-Meteo comme secours quand MQTT n'est pas disponible", "Usar Open-Meteo como reserva cuando MQTT no está disponible", "Usa Open-Meteo come riserva quando MQTT non è disponibile", "Использовать Open-Meteo как резерв, когда MQTT недоступен", "जब MQTT उपलब्ध न हो तो Open-Meteo को फ़ॉलबैक के रूप में उपयोग करें", "当 MQTT 不可用时使用 Open-Meteo 作为备用"}},
  {{"MQTT-Broker IP / Host", "MQTT broker IP / host", "IP / hôte du broker MQTT", "IP / host del broker MQTT", "IP / host del broker MQTT", "IP / хост MQTT-брокера", "MQTT ब्रोकर IP / होस्ट", "MQTT 代理 IP / 主机"}},
  {{"Port", "Port", "Port", "Puerto", "Porta", "Порт", "पोर्ट", "端口"}},
  {{"Benutzer", "User", "Utilisateur", "Usuario", "Utente", "Пользователь", "उपयोगकर्ता", "用户"}},
  {{"Passwort", "Password", "Mot de passe", "Contraseña", "Password", "Пароль", "पासवर्ड", "密码"}},
  {{"WeatherFlow Beobachtungs-JSON-Topic", "WeatherFlow observation JSON topic", "Topic JSON d'observation WeatherFlow", "Topic JSON de observación WeatherFlow", "Topic JSON di osservazione WeatherFlow", "JSON-топик наблюдений WeatherFlow", "WeatherFlow अवलोकन JSON टॉपिक", "WeatherFlow 观测 JSON 主题"}},
  {{"Design", "Design", "Design", "Diseño", "Design", "Оформление", "डिज़ाइन", "外观"}},
  {{"Darstellung, Farben und Widget-Rahmen.", "Appearance, colors and widget borders.", "Apparence, couleurs et bordures des widgets.", "Apariencia, colores y bordes de widgets.", "Aspetto, colori e bordi dei widget.", "Внешний вид, цвета и рамки виджетов.", "रूप, रंग और विजेट बॉर्डर।", "外观、颜色和小部件边框。"}},
  {{"Akzentfarbe", "Accent color", "Couleur d'accent", "Color de acento", "Colore d'accento", "Акцентный цвет", "एक्सेंट रंग", "强调色"}},
  {{"Rahmenfarbe", "Border color", "Couleur de bordure", "Color de borde", "Colore bordo", "Цвет рамки", "बॉर्डर रंग", "边框颜色"}},
  {{"Textfarbe", "Text color", "Couleur du texte", "Color de texto", "Colore testo", "Цвет текста", "टेक्स्ट रंग", "文字颜色"}},
  {{"Design", "Design", "Design", "Diseño", "Design", "Оформление", "डिज़ाइन", "外观"}},
  {{"Einstellungen", "Settings", "Paramètres", "Ajustes", "Impostazioni", "Настройки", "सेटिंग्स", "设置"}},
  {{"Grundverhalten und Anzeigeeinstellungen.", "Basic behavior and display settings.", "Comportement de base et réglages d'affichage.", "Comportamiento básico y ajustes de pantalla.", "Comportamento di base e impostazioni di visualizzazione.", "Основное поведение и настройки отображения.", "बुनियादी व्यवहार और प्रदर्शन सेटिंग्स।", "基本行为和显示设置。"}},
  {{"Allgemein", "General", "Général", "General", "Generale", "Общие", "सामान्य", "常规"}},
  {{"Gerätename", "Device name", "Nom de l'appareil", "Nombre del dispositivo", "Nome dispositivo", "Имя устройства", "डिवाइस का नाम", "设备名称"}},
  {{"Einheitensystem", "Unit system", "Système d'unités", "Sistema de unidades", "Sistema di unità", "Система единиц", "इकाई प्रणाली", "单位制"}},
  {{"Datumsformat", "Date format", "Format de date", "Formato de fecha", "Formato data", "Формат даты", "दिनांक प्रारूप", "日期格式"}},
  {{"Zeitzone", "Time zone", "Fuseau horaire", "Zona horaria", "Fuso orario", "Часовой пояс", "समय क्षेत्र", "时区"}},
  {{"Feld", "Slot", "Emplacement", "Ranura", "Slot", "Ячейка", "स्लॉट", "插槽"}},
  {{"Standort", "Location", "Localisation", "Ubicación", "Località", "Местоположение", "स्थान", "位置"}},
  {{"Wird für Wetterdaten und Sonnenzeiten verwendet.", "Used for weather data and sunrise/sunset times.", "Utilisé pour les données météo et les heures de lever/coucher du soleil.", "Se usa para datos meteorológicos y horas de amanecer/atardecer.", "Usato per i dati meteo e gli orari di alba/tramonto.", "Используется для данных о погоде и времени восхода/захода.", "मौसम डेटा और सूर्योदय/सूर्यास्त समय के लिए उपयोग किया जाता है।", "用于天气数据和日出/日落时间。"}},
  {{"Standortname", "Location name", "Nom du lieu", "Nombre de ubicación", "Nome località", "Название местоположения", "स्थान का नाम", "地点名称"}},
  {{"Breitengrad", "Latitude", "Latitude", "Latitud", "Latitudine", "Широта", "अक्षांश", "纬度"}},
  {{"Längengrad", "Longitude", "Longitude", "Longitud", "Longitudine", "Долгота", "देशांतर", "经度"}},
  {{"MQTT Datenpunkte", "MQTT DataPoints", "Points de données MQTT", "Puntos de datos MQTT", "Punti dati MQTT", "Точки данных MQTT", "MQTT डेटा पॉइंट", "MQTT 数据点"}},
  {{"Anzeigewerte", "Display values", "Valeurs affichées", "Valores mostrados", "Valori visualizzati", "Отображаемые значения", "प्रदर्शन मान", "显示数值"}},
  {{"Bezeichnung, Topic und Sichtbarkeit je Wert.", "Label, topic and visibility per value.", "Libellé, topic et visibilité par valeur.", "Etiqueta, topic y visibilidad por valor.", "Etichetta, topic e visibilità per valore.", "Название, топик и видимость для каждого значения.", "प्रत्येक मान के लिए लेबल, टॉपिक और दृश्यता।", "每个数值的标签、主题和可见性。"}},
  {{"Bezeichnung", "Label", "Libellé", "Etiqueta", "Etichetta", "Название", "लेबल", "标签"}},
  {{"Einheit", "Unit", "Unité", "Unidad", "Unità", "Единица", "इकाई", "单位"}},
  {{"Topic", "Topic", "Topic", "Topic", "Topic", "Топик", "टॉपिक", "主题"}},
  {{"Anzeigen", "Show", "Afficher", "Mostrar", "Mostra", "Показать", "दिखाएँ", "显示"}},
  {{"Widget-Auswahl", "Widget selection", "Sélection des widgets", "Selección de widgets", "Selezione widget", "Выбор виджетов", "विजेट चयन", "小部件选择"}},
  {{"Wähle aus, welche Widgets in den sechs Home-Feldern angezeigt werden.", "Choose which widgets are shown in the six home slots.", "Choisissez quels widgets sont affichés dans les six emplacements de l'accueil.", "Elige qué widgets se muestran en las seis ranuras de inicio.", "Scegli quali widget mostrare nelle sei posizioni della home.", "Выберите, какие виджеты отображаются в шести ячейках главного экрана.", "चुनें कि छह होम स्लॉट में कौन से विजेट दिखाए जाएँ।", "选择在六个主页位置显示哪些小部件。"}},
  {{"Leer", "Empty", "Vide", "Vacío", "Vuoto", "Пусто", "खाली", "空"}},
  {{"Niedrig", "Low", "Faible", "Bajo", "Basso", "Низкий", "कम", "低"}},
  {{"Mittel", "Medium", "Moyen", "Medio", "Medio", "Средний", "मध्यम", "中"}},
  {{"Moderat", "Moderate", "Modéré", "Moderado", "Moderato", "Умеренный", "मध्यम", "适中"}},
  {{"Hoch", "High", "Élevé", "Alto", "Alto", "Высокий", "उच्च", "高"}},
  {{"Sehr hoch", "Very High", "Très élevé", "Muy alto", "Molto alto", "Очень высокий", "बहुत उच्च", "很高"}},
  {{"Extrem", "Extreme", "Extrême", "Extremo", "Estremo", "Экстремальный", "अत्यधिक", "极高"}},
  {{"Speichern", "Save", "Enregistrer", "Guardar", "Salva", "Сохранить", "सहेजें", "保存"}},
  {{"Gespeichert", "Saved", "Enregistré", "Guardado", "Salvato", "Сохранено", "सहेजा गया", "已保存"}},
  {{"Die Einstellungen wurden übernommen. Der InfoTerm aktualisiert sich jetzt.", "The settings have been applied. InfoTerm is updating now.", "Les paramètres ont été appliqués. InfoTerm se met à jour maintenant.", "Los ajustes se han aplicado. InfoTerm se está actualizando ahora.", "Le impostazioni sono state applicate. InfoTerm si sta aggiornando ora.", "Настройки применены. InfoTerm обновляется.", "सेटिंग्स लागू हो गईं। InfoTerm अब अपडेट हो रहा है।", "设置已应用。InfoTerm 正在更新。"}},
  {{"Zurück", "Back", "Retour", "Atrás", "Indietro", "Назад", "वापस", "返回"}},
  {{"Gespeichert – leer lassen, um es zu behalten", "Stored – leave empty to keep it", "Enregistré – laisser vide pour le conserver", "Guardado – dejar vacío para conservarlo", "Salvato – lasciare vuoto per mantenerlo", "Сохранено – оставьте пустым, чтобы сохранить", "सहेजा गया – रखने के लिए खाली छोड़ें", "已保存 – 留空以保留"}},
  {{"Gespeichertes Passwort entfernen", "Remove stored password", "Supprimer le mot de passe enregistré", "Eliminar la contraseña guardada", "Rimuovi la password salvata", "Удалить сохранённый пароль", "सहेजा गया पासवर्ड हटाएँ", "删除已保存的密码"}},
  // --- DataPoints tab ---
  {{"Datenpunkte", "DataPoints", "Points de données", "Puntos de datos", "Punti dati", "Точки данных", "डेटा पॉइंट", "数据点"}},
  {{"Aktuelle Runtime-Werte aus Wetter, MQTT, System und internen Quellen.", "Current runtime values from weather, MQTT, system and internal sources.", "Valeurs actuelles issues des sources météo, MQTT, système et internes.", "Valores actuales de fuentes meteorológicas, MQTT, sistema e internas.", "Valori attuali da sorgenti meteo, MQTT, sistema e interne.", "Текущие значения из источников погоды, MQTT, системы и внутренних.", "मौसम, MQTT, सिस्टम और आंतरिक स्रोतों से वर्तमान मान।", "来自天气、MQTT、系统和内部源的当前运行值。"}},
  {{"Suche", "Search", "Rechercher", "Buscar", "Cerca", "Поиск", "खोज", "搜索"}},
  {{"ID, Name oder Wert", "ID, name or value", "ID, nom ou valeur", "ID, nombre o valor", "ID, nome o valore", "ID, имя или значение", "ID, नाम या मान", "ID、名称或数值"}},
  {{"Quelle", "Source", "Source", "Fuente", "Sorgente", "Источник", "स्रोत", "来源"}},
  {{"Alle", "All", "Tout", "Todos", "Tutti", "Все", "सभी", "全部"}},
  {{"Anwenden", "Apply", "Appliquer", "Aplicar", "Applica", "Применить", "लागू करें", "应用"}},
  {{"Zurücksetzen", "Reset", "Réinitialiser", "Restablecer", "Reimposta", "Сбросить", "रीसेट", "重置"}},
  {{"Gesamt", "Total", "Total", "Total", "Totale", "Всего", "कुल", "总计"}},
  {{"Wert", "Value", "Valeur", "Valor", "Valore", "Значение", "मान", "数值"}},
  {{"Alter", "Age", "Âge", "Edad", "Età", "Возраст", "आयु", "时长"}},
  {{"Status", "Status", "Statut", "Estado", "Stato", "Статус", "स्थिति", "状态"}},
  {{"Keine passenden Datenpunkte gefunden.", "No matching data points found.", "Aucun point de données correspondant.", "No se encontraron puntos de datos coincidentes.", "Nessun punto dati corrispondente trovato.", "Подходящие точки данных не найдены.", "कोई मेल खाने वाला डेटा पॉइंट नहीं मिला।", "未找到匹配的数据点。"}},
  {{"Sichtbar: ", "Visible: ", "Visibles : ", "Visibles: ", "Visibili: ", "Видно: ", "दृश्यमान: ", "可见: "}},
  {{" von ", " of ", " sur ", " de ", " di ", " из ", " / ", " / "}},
  {{" Datenpunkten.", " data points.", " points de données.", " puntos de datos.", " punti dati.", " точек данных.", " डेटा पॉइंट।", " 个数据点。"}},
  // --- Status hero ---
  {{"Tunnel-IP", "Tunnel IP", "IP du tunnel", "IP del túnel", "IP del tunnel", "IP туннеля", "टनल IP", "隧道 IP"}},
  // --- MQTT tab ---
  {{"Verbindung testen", "Test connection", "Tester la connexion", "Probar conexión", "Prova connessione", "Проверить соединение", "कनेक्शन जाँचें", "测试连接"}},
  // --- Settings tab ---
  {{"WLAN-Netzwerk", "WiFi network", "Réseau WiFi", "Red WiFi", "Rete WiFi", "Сеть WiFi", "वाईफ़ाई नेटवर्क", "WiFi 网络"}},
  {{" - aktuell verbunden", " - currently connected", " - actuellement connecté", " - conectado actualmente", " - attualmente connesso", " - подключено сейчас", " - अभी कनेक्टेड", " - 当前已连接"}},
  {{" - in Reichweite", " - in range", " - à portée", " - dentro del alcance", " - a portata", " - в зоне действия", " - सीमा में", " - 在范围内"}},
  {{" - nicht in Reichweite", " - out of range", " - hors de portée", " - fuera de alcance", " - fuori portata", " - вне зоны действия", " - सीमा से बाहर", " - 超出范围"}},
  {{"Bevorzugtes Netz - klappt die Verbindung nicht, werden die anderen hinterlegten Netze der Reihe nach automatisch versucht.", "Preferred network - if the connection fails, the other stored networks are tried automatically one after another.", "Réseau préféré - si la connexion échoue, les autres réseaux enregistrés sont essayés automatiquement l'un après l'autre.", "Red preferida - si la conexión falla, se prueban automáticamente las otras redes guardadas una tras otra.", "Rete preferita - se la connessione fallisce, le altre reti memorizzate vengono provate automaticamente una dopo l'altra.", "Предпочтительная сеть - если подключение не удаётся, остальные сохранённые сети пробуются автоматически по очереди.", "पसंदीदा नेटवर्क - यदि कनेक्शन विफल होता है, तो अन्य सहेजे गए नेटवर्क स्वचालित रूप से एक-एक करके आज़माए जाते हैं।", "首选网络 - 如果连接失败，将自动依次尝试其他已保存的网络。"}},
  {{"Automatischer Schlafmodus", "Automatic sleep mode", "Mode veille automatique", "Modo de reposo automático", "Modalità sospensione automatica", "Автоматический спящий режим", "स्वचालित स्लीप मोड", "自动休眠模式"}},
  {{"Nie", "Never", "Jamais", "Nunca", "Mai", "Никогда", "कभी नहीं", "从不"}},
  {{"1 Minute", "1 minute", "1 minute", "1 minuto", "1 minuto", "1 минута", "1 मिनट", "1 分钟"}},
  {{"5 Minuten", "5 minutes", "5 minutes", "5 minutos", "5 minuti", "5 минут", "5 मिनट", "5 分钟"}},
  {{"10 Minuten", "10 minutes", "10 minutes", "10 minutos", "10 minuti", "10 минут", "10 मिनट", "10 分钟"}},
  {{"30 Minuten", "30 minutes", "30 minutes", "30 minutos", "30 minuti", "30 минут", "30 मिनट", "30 分钟"}},
  {{"1 Stunde", "1 hour", "1 heure", "1 hora", "1 ora", "1 час", "1 घंटा", "1 小时"}},
  {{"Der Schlafmodus dimmt zuerst das Display und schaltet es nach 60 Sekunden ganz aus.", "Sleep mode first dims the display and turns it off completely after 60 seconds.", "Le mode veille assombrit d'abord l'écran puis l'éteint complètement après 60 secondes.", "El modo de reposo primero atenúa la pantalla y la apaga por completo tras 60 segundos.", "La modalità sospensione prima attenua il display e lo spegne completamente dopo 60 secondi.", "Спящий режим сначала затемняет дисплей, а через 60 секунд полностью выключает его.", "स्लीप मोड पहले डिस्प्ले को मंद करता है और 60 सेकंड बाद उसे पूरी तरह बंद कर देता है।", "休眠模式先调暗显示屏，60 秒后完全关闭。"}},
  {{"Tab-Rotation (Einbrennschutz)", "Tab rotation (burn-in guard)", "Rotation des onglets (anti-brûlure)", "Rotación de pestañas (protección anti-quemado)", "Rotazione schede (anti burn-in)", "Ротация вкладок (защита от выгорания)", "टैब रोटेशन (बर्न-इन सुरक्षा)", "标签轮播（防烧屏）"}},
  {{"Angezeigte Tabs (ausser Info) automatisch durchlaufen - nur wirksam, wenn der Schlafmodus auf \"Nie\" steht", "Cycle through the visible tabs (except Info) automatically - only effective when sleep mode is set to \"Never\"", "Faire défiler automatiquement les onglets affichés (sauf Info) - actif uniquement si le mode veille est réglé sur \"Jamais\"", "Recorrer automáticamente las pestañas visibles (excepto Info) - solo funciona si el modo de reposo está en \"Nunca\"", "Scorri automaticamente le schede visibili (tranne Info) - attivo solo se la sospensione è impostata su \"Mai\"", "Автоматически переключать видимые вкладки (кроме Инфо) - работает только когда спящий режим установлен на \"Никогда\"", "दृश्यमान टैब (Info को छोड़कर) को स्वचालित रूप से बदलें - केवल तभी प्रभावी जब स्लीप मोड \"कभी नहीं\" पर सेट हो", "自动轮播可见的标签页（信息页除外）- 仅在休眠模式设为\"从不\"时有效"}},
  {{"alle 30 Sekunden", "every 30 seconds", "toutes les 30 secondes", "cada 30 segundos", "ogni 30 secondi", "каждые 30 секунд", "हर 30 सेकंड", "每 30 秒"}},
  {{"alle 60 Sekunden", "every 60 seconds", "toutes les 60 secondes", "cada 60 segundos", "ogni 60 secondi", "каждые 60 секунд", "हर 60 सेकंड", "每 60 秒"}},
  {{"alle 3 Minuten", "every 3 minutes", "toutes les 3 minutes", "cada 3 minutos", "ogni 3 minuti", "каждые 3 минуты", "हर 3 मिनट", "每 3 分钟"}},
  {{"alle 6 Minuten", "every 6 minutes", "toutes les 6 minutes", "cada 6 minutos", "ogni 6 minuti", "каждые 6 минут", "हर 6 मिनट", "每 6 分钟"}},
  {{"Fahrenheit / Zoll", "Fahrenheit / inch", "Fahrenheit / pouce", "Fahrenheit / pulgada", "Fahrenheit / pollice", "Фаренгейт / дюйм", "फ़ारेनहाइट / इंच", "华氏度 / 英寸"}},
  {{"Europa: TT.MM.JJJJ", "Europe: DD.MM.YYYY", "Europe : JJ.MM.AAAA", "Europa: DD.MM.AAAA", "Europa: GG.MM.AAAA", "Европа: ДД.ММ.ГГГГ", "यूरोप: DD.MM.YYYY", "欧洲：DD.MM.YYYY"}},
  {{"USA: MM/TT/JJJJ", "USA: MM/DD/YYYY", "USA : MM/JJ/AAAA", "EE.UU.: MM/DD/AAAA", "USA: MM/GG/AAAA", "США: ММ/ДД/ГГГГ", "यूएसए: MM/DD/YYYY", "美国：MM/DD/YYYY"}},
  {{"Beispiel Berlin: Breitengrad 52.5200, Längengrad 13.4050.", "Example Berlin: latitude 52.5200, longitude 13.4050.", "Exemple Berlin : latitude 52.5200, longitude 13.4050.", "Ejemplo Berlín: latitud 52.5200, longitud 13.4050.", "Esempio Berlino: latitudine 52.5200, longitudine 13.4050.", "Пример Берлин: широта 52.5200, долгота 13.4050.", "उदाहरण बर्लिन: अक्षांश 52.5200, देशांतर 13.4050।", "示例柏林：纬度 52.5200，经度 13.4050。"}},
  {{"Netzwerk / IP-Adresse", "Network / IP address", "Réseau / adresse IP", "Red / dirección IP", "Rete / indirizzo IP", "Сеть / IP-адрес", "नेटवर्क / IP पता", "网络 / IP 地址"}},
  {{"Standard ist automatischer Bezug (DHCP). Fuer Netze ohne DHCP-Server eine feste IP vergeben. Eine Aenderung baut die WLAN-Verbindung neu auf - das Geraet ist danach unter der neuen Adresse erreichbar. Achtung: eine falsche feste IP kann das Geraet unerreichbar machen (Wiederherstellung per Backup-Import oder USB-Reflash).", "Default is automatic assignment (DHCP). For networks without a DHCP server, assign a fixed IP. A change rebuilds the WiFi connection - the device is then reachable at the new address. Caution: an incorrect fixed IP can make the device unreachable (recovery via backup import or USB reflash).", "Par défaut, l'attribution est automatique (DHCP). Pour les réseaux sans serveur DHCP, attribuez une IP fixe. Une modification reconstruit la connexion WiFi - l'appareil est ensuite accessible à la nouvelle adresse. Attention : une IP fixe incorrecte peut rendre l'appareil inaccessible (récupération via import de sauvegarde ou reflash USB).", "Por defecto la asignación es automática (DHCP). Para redes sin servidor DHCP, asigna una IP fija. Un cambio reconstruye la conexión WiFi - el dispositivo será accesible en la nueva dirección. Atención: una IP fija incorrecta puede dejar el dispositivo inaccesible (recuperación mediante importación de copia de seguridad o reflasheo USB).", "Per impostazione predefinita l'assegnazione è automatica (DHCP). Per reti senza server DHCP, assegna un IP fisso. Una modifica ricostruisce la connessione WiFi - il dispositivo sarà poi raggiungibile al nuovo indirizzo. Attenzione: un IP fisso errato può rendere il dispositivo irraggiungibile (ripristino tramite importazione backup o riflash USB).", "По умолчанию используется автоматическое назначение (DHCP). Для сетей без DHCP-сервера назначьте фиксированный IP. Изменение перестраивает WiFi-соединение - после этого устройство доступно по новому адресу. Внимание: неверный фиксированный IP может сделать устройство недоступным (восстановление через импорт резервной копии или перепрошивку по USB).", "डिफ़ॉल्ट रूप से स्वचालित असाइनमेंट (DHCP) होता है। DHCP सर्वर के बिना नेटवर्क के लिए एक स्थिर IP असाइन करें। बदलाव WiFi कनेक्शन को फिर से बनाता है - फिर डिवाइस नए पते पर पहुँच योग्य होता है। सावधान: गलत स्थिर IP डिवाइस को पहुँच से बाहर कर सकता है (बैकअप आयात या USB रीफ़्लैश द्वारा पुनर्प्राप्ति)।", "默认使用自动分配 (DHCP)。对于没有 DHCP 服务器的网络，请分配固定 IP。更改会重建 WiFi 连接 - 之后设备可通过新地址访问。注意：错误的固定 IP 可能导致设备无法访问（通过备份导入或 USB 重新刷写恢复）。"}},
  {{"IP-Zuweisung", "IP assignment", "Attribution IP", "Asignación de IP", "Assegnazione IP", "Назначение IP", "IP असाइनमेंट", "IP 分配"}},
  {{"Automatisch (DHCP)", "Automatic (DHCP)", "Automatique (DHCP)", "Automática (DHCP)", "Automatico (DHCP)", "Автоматически (DHCP)", "स्वचालित (DHCP)", "自动 (DHCP)"}},
  {{"Statisch (feste IP)", "Static (fixed IP)", "Statique (IP fixe)", "Estática (IP fija)", "Statico (IP fisso)", "Статически (фикс. IP)", "स्थिर (फ़िक्स्ड IP)", "静态（固定 IP）"}},
  {{"Subnetzmaske", "Subnet mask", "Masque de sous-réseau", "Máscara de subred", "Maschera di sottorete", "Маска подсети", "सबनेट मास्क", "子网掩码"}},
  {{"DNS-Server (optional)", "DNS server (optional)", "Serveur DNS (optionnel)", "Servidor DNS (opcional)", "Server DNS (opzionale)", "DNS-сервер (необязательно)", "DNS सर्वर (वैकल्पिक)", "DNS 服务器（可选）"}},
  // --- VPN panel ---
  {{"WireGuard-Konfigurationsdatei (.conf) hochladen und den Tunnel bei Bedarf aktivieren. <b>Wichtig:</b> waehrend der Tunnel aktiv ist, ist die WebGUI im lokalen Netz nicht erreichbar - nur ueber die Tunnel-IP.", "Upload a WireGuard configuration file (.conf) and activate the tunnel when needed. <b>Important:</b> while the tunnel is active, the WebGUI is not reachable on the local network - only via the tunnel IP.", "Téléversez un fichier de configuration WireGuard (.conf) et activez le tunnel si nécessaire. <b>Important :</b> tant que le tunnel est actif, l'interface web n'est pas accessible sur le réseau local - uniquement via l'IP du tunnel.", "Sube un archivo de configuración WireGuard (.conf) y activa el túnel cuando lo necesites. <b>Importante:</b> mientras el túnel está activo, la interfaz web no es accesible en la red local - solo a través de la IP del túnel.", "Carica un file di configurazione WireGuard (.conf) e attiva il tunnel quando necessario. <b>Importante:</b> mentre il tunnel è attivo, la WebGUI non è raggiungibile sulla rete locale - solo tramite l'IP del tunnel.", "Загрузите файл конфигурации WireGuard (.conf) и при необходимости активируйте туннель. <b>Важно:</b> пока туннель активен, WebGUI недоступна в локальной сети - только через IP туннеля.", "एक WireGuard कॉन्फ़िगरेशन फ़ाइल (.conf) अपलोड करें और आवश्यकता होने पर टनल सक्रिय करें। <b>महत्वपूर्ण:</b> जब तक टनल सक्रिय है, WebGUI स्थानीय नेटवर्क पर पहुँच योग्य नहीं है - केवल टनल IP के माध्यम से।", "上传 WireGuard 配置文件 (.conf) 并在需要时激活隧道。<b>重要：</b>隧道激活时，WebGUI 在本地网络中无法访问 - 只能通过隧道 IP 访问。"}},
  {{"Hochladen", "Upload", "Téléverser", "Subir", "Carica", "Загрузить", "अपलोड", "上传"}},
  {{"Endpunkt", "Endpoint", "Point de terminaison", "Extremo", "Endpoint", "Конечная точка", "एंडपॉइंट", "端点"}},
  {{"Tunnel-Adresse", "Tunnel address", "Adresse du tunnel", "Dirección del túnel", "Indirizzo tunnel", "Адрес туннеля", "टनल पता", "隧道地址"}},
  {{"Verbinden", "Connect", "Connecter", "Conectar", "Connetti", "Подключить", "कनेक्ट करें", "连接"}},
  {{"Trennen", "Disconnect", "Déconnecter", "Desconectar", "Disconnetti", "Отключить", "डिस्कनेक्ट करें", "断开"}},
  {{"Noch keine VPN-Konfiguration hochgeladen.", "No VPN configuration uploaded yet.", "Aucune configuration VPN téléversée pour le moment.", "Aún no se ha subido ninguna configuración VPN.", "Nessuna configurazione VPN ancora caricata.", "Конфигурация VPN ещё не загружена.", "अभी तक कोई VPN कॉन्फ़िगरेशन अपलोड नहीं किया गया।", "尚未上传 VPN 配置。"}},
  // --- Backup panel ---
  {{"Aktuelle Konfiguration als JSON-Datei herunterladen oder aus einer Datei wiederherstellen.", "Download the current configuration as a JSON file or restore it from a file.", "Télécharger la configuration actuelle en fichier JSON ou la restaurer depuis un fichier.", "Descargar la configuración actual como archivo JSON o restaurarla desde un archivo.", "Scarica la configurazione attuale come file JSON o ripristinala da un file.", "Скачать текущую конфигурацию как файл JSON или восстановить из файла.", "वर्तमान कॉन्फ़िगरेशन को JSON फ़ाइल के रूप में डाउनलोड करें या किसी फ़ाइल से पुनर्स्थापित करें।", "将当前配置下载为 JSON 文件，或从文件恢复。"}},
  {{"Backup herunterladen", "Download backup", "Télécharger la sauvegarde", "Descargar copia de seguridad", "Scarica backup", "Скачать резервную копию", "बैकअप डाउनलोड करें", "下载备份"}},
  {{"Wiederherstellen", "Restore", "Restaurer", "Restaurar", "Ripristina", "Восстановить", "पुनर्स्थापित करें", "恢复"}},
  // --- Firmware panel ---
  {{"Firmware-Update", "Firmware update", "Mise à jour du firmware", "Actualización de firmware", "Aggiornamento firmware", "Обновление прошивки", "फर्मवेयर अपडेट", "固件更新"}},
  {{"Aktuelle Version: ", "Current version: ", "Version actuelle : ", "Versión actual: ", "Versione attuale: ", "Текущая версия: ", "वर्तमान संस्करण: ", "当前版本: "}},
  {{". Neue .bin-Firmwaredatei hochladen, um das Geraet ohne USB-Kabel zu aktualisieren.", ". Upload a new .bin firmware file to update the device without a USB cable.", ". Téléversez un nouveau fichier firmware .bin pour mettre à jour l'appareil sans câble USB.", ". Sube un nuevo archivo de firmware .bin para actualizar el dispositivo sin cable USB.", ". Carica un nuovo file firmware .bin per aggiornare il dispositivo senza cavo USB.", ". Загрузите новый файл прошивки .bin, чтобы обновить устройство без USB-кабеля.", ". USB केबल के बिना डिवाइस को अपडेट करने के लिए एक नई .bin फर्मवेयर फ़ाइल अपलोड करें।", "。上传新的 .bin 固件文件以在不使用 USB 线缆的情况下更新设备。"}},
  {{"Waehrend des Uploads nicht ausschalten und die WLAN-Verbindung nicht unterbrechen. Bricht die Uebertragung ab, startet das Geraet mit der alten Firmware nicht mehr und muss per USB neu geflasht werden.", "Do not switch off during the upload and do not interrupt the WiFi connection. If the transfer aborts, the device will no longer start with the old firmware and must be reflashed via USB.", "N'éteignez pas l'appareil pendant le téléversement et n'interrompez pas la connexion WiFi. Si le transfert échoue, l'appareil ne démarrera plus avec l'ancien firmware et devra être reflashé via USB.", "No apague el dispositivo durante la subida ni interrumpa la conexión WiFi. Si la transferencia se cancela, el dispositivo ya no arrancará con el firmware antiguo y deberá reflashearse por USB.", "Non spegnere il dispositivo durante il caricamento e non interrompere la connessione WiFi. Se il trasferimento si interrompe, il dispositivo non si avvierà più con il vecchio firmware e dovrà essere riflashato via USB.", "Не выключайте устройство во время загрузки и не прерывайте соединение WiFi. Если передача прервётся, устройство больше не запустится со старой прошивкой и потребует перепрошивки по USB.", "अपलोड के दौरान बंद न करें और WiFi कनेक्शन को बाधित न करें। यदि स्थानांतरण रुक जाता है, तो डिवाइस पुराने फर्मवेयर के साथ शुरू नहीं होगा और उसे USB के माध्यम से फिर से फ़्लैश करना होगा।", "上传期间请勿关机，也不要中断 WiFi 连接。如果传输中断，设备将无法再用旧固件启动，必须通过 USB 重新刷写。"}},
  {{"Firmware hochladen", "Upload firmware", "Téléverser le firmware", "Subir firmware", "Carica firmware", "Загрузить прошивку", "फर्मवेयर अपलोड करें", "上传固件"}},
  // --- Log panel ---
  {{"Die letzten ", "The last ", "Les ", "Las últimas ", "Le ultime ", "Последние ", "पिछली ", "最近 "}},
  {{" Logzeilen (neueste zuerst, Zeitangabe = Geräte-Laufzeit). Welche Meldungen aufgezeichnet werden, steuert das Log-Level in den Einstellungen.", " log lines (newest first, timestamp = device uptime). The log level in settings controls which messages are recorded.", " dernières lignes de log (les plus récentes en premier, horodatage = durée de fonctionnement). Le niveau de log dans les paramètres détermine quels messages sont enregistrés.", " líneas de log (más recientes primero, marca de tiempo = tiempo activo). El nivel de log en los ajustes determina qué mensajes se registran.", " righe di log (più recenti per prime, orario = tempo di attività). Il livello di log nelle impostazioni determina quali messaggi vengono registrati.", " строк журнала (сначала новые, отметка времени = время работы устройства). Уровень журнала в настройках определяет, какие сообщения записываются.", " लॉग लाइनें (नवीनतम पहले, टाइमस्टैम्प = डिवाइस अपटाइम)। सेटिंग्स में लॉग स्तर निर्धारित करता है कि कौन से संदेश रिकॉर्ड किए जाते हैं।", " 条日志（最新的在前，时间戳 = 设备运行时间）。设置中的日志级别决定记录哪些消息。"}},
  {{"Log laden", "Load log", "Charger le log", "Cargar log", "Carica log", "Загрузить журнал", "लॉग लोड करें", "加载日志"}},
  // --- Design tab ---
  {{"Nachtmodus", "Night mode", "Mode nuit", "Modo nocturno", "Modalità notte", "Ночной режим", "नाइट मोड", "夜间模式"}},
  {{"Automatisch zwischen Sonnenuntergang und -aufgang auf gedämpfte Rottöne umschalten", "Automatically switch to dimmed red tones between sunset and sunrise", "Passer automatiquement à des tons rouges atténués entre le coucher et le lever du soleil", "Cambiar automáticamente a tonos rojos atenuados entre el atardecer y el amanecer", "Passa automaticamente a toni rossi attenuati tra il tramonto e l'alba", "Автоматически переключаться на приглушённые красные тона между закатом и восходом", "सूर्यास्त और सूर्योदय के बीच स्वचालित रूप से मंद लाल रंगों में बदलें", "在日落和日出之间自动切换为柔和的红色调"}},
  {{"Rahmen um Widgets", "Widget borders", "Bordures des widgets", "Bordes de widgets", "Bordi dei widget", "Рамки виджетов", "विजेट बॉर्डर", "小部件边框"}},
  {{"Rahmen auf Display-Widgets anzeigen", "Show borders on display widgets", "Afficher les bordures sur les widgets de l'écran", "Mostrar bordes en los widgets de la pantalla", "Mostra i bordi sui widget del display", "Показывать рамки на виджетах дисплея", "डिस्प्ले विजेट पर बॉर्डर दिखाएँ", "在显示屏小部件上显示边框"}},
  {{"Anzeige drehen", "Rotate display", "Pivoter l'affichage", "Girar pantalla", "Ruota display", "Повернуть дисплей", "डिस्प्ले घुमाएँ", "旋转显示"}},
  {{"Darstellung um 180° drehen, für über Kopf montierte Geräte", "Rotate the display by 180° for overhead-mounted devices", "Pivoter l'affichage de 180° pour les appareils montés à l'envers", "Girar la pantalla 180° para dispositivos montados al revés", "Ruota il display di 180° per dispositivi montati capovolti", "Повернуть отображение на 180° для устройств, установленных вверх ногами", "उल्टा लगे उपकरणों के लिए डिस्प्ले को 180° घुमाएँ", "将显示旋转 180°，适用于倒装的设备"}},
  {{"Menühintergrund", "Menu background", "Fond du menu", "Fondo del menú", "Sfondo menu", "Фон меню", "मेन्यू पृष्ठभूमि", "菜单背景"}},
  {{"Buttonhintergrund", "Button background", "Fond des boutons", "Fondo de botones", "Sfondo pulsanti", "Фон кнопок", "बटन पृष्ठभूमि", "按钮背景"}},
  {{"Widgethintergrund", "Widget background", "Fond des widgets", "Fondo de widgets", "Sfondo widget", "Фон виджетов", "विजेट पृष्ठभूमि", "小部件背景"}},
  // --- MQTT values panel ---
  {{"MQTT-Datenpunkte konfigurieren (maximal 32 Datenpunkte).", "Configure MQTT data points (max. 32 data points).", "Configurer les points de données MQTT (32 maximum).", "Configurar puntos de datos MQTT (máx. 32).", "Configura i punti dati MQTT (max 32).", "Настройте точки данных MQTT (максимум 32).", "MQTT डेटा पॉइंट कॉन्फ़िगर करें (अधिकतम 32)।", "配置 MQTT 数据点（最多 32 个）。"}},
  {{"Dez.", "Dec.", "Déc.", "Dec.", "Dec.", "Дес.", "दश.", "小数"}},
  {{"Aktiv", "Active", "Actif", "Activo", "Attivo", "Активно", "सक्रिय", "启用"}},
  {{"DataPoint löschen", "Delete data point", "Supprimer le point de données", "Eliminar punto de datos", "Elimina punto dati", "Удалить точку данных", "डेटा पॉइंट हटाएँ", "删除数据点"}},
  {{"Diesen MQTT-DataPoint löschen?", "Delete this MQTT data point?", "Supprimer ce point de données MQTT ?", "¿Eliminar este punto de datos MQTT?", "Eliminare questo punto dati MQTT?", "Удалить эту точку данных MQTT?", "इस MQTT डेटा पॉइंट को हटाएँ?", "删除此 MQTT 数据点？"}},
  {{"Datenpunkt anlegen", "Add data point", "Ajouter un point de données", "Añadir punto de datos", "Aggiungi punto dati", "Добавить точку данных", "डेटा पॉइंट जोड़ें", "添加数据点"}},
  {{"Nur die MQTT-Datenpunkte als JSON sichern oder von einem anderen Terminal uebernehmen (ueberschreibt die gespeicherten Datenpunkte, nicht die uebrige Konfiguration). Vor dem Export nicht gespeicherte Aenderungen bitte zuerst speichern.", "Save only the MQTT data points as JSON or import them from another terminal (overwrites the stored data points, not the rest of the configuration). Please save any unsaved changes before exporting.", "Enregistrer uniquement les points de données MQTT en JSON ou les importer depuis un autre terminal (écrase les points de données enregistrés, pas le reste de la configuration). Veuillez enregistrer les modifications non sauvegardées avant l'export.", "Guardar solo los puntos de datos MQTT como JSON o importarlos de otro terminal (sobrescribe los puntos de datos guardados, no el resto de la configuración). Guarde los cambios no guardados antes de exportar.", "Salva solo i punti dati MQTT come JSON o importali da un altro terminale (sovrascrive i punti dati salvati, non il resto della configurazione). Salva le modifiche non salvate prima di esportare.", "Сохраните только точки данных MQTT как JSON или импортируйте их с другого терминала (перезаписывает сохранённые точки данных, а не остальную конфигурацию). Пожалуйста, сохраните несохранённые изменения перед экспортом.", "केवल MQTT डेटा पॉइंट को JSON के रूप में सहेजें या किसी अन्य टर्मिनल से आयात करें (सहेजे गए डेटा पॉइंट को अधिलेखित करता है, शेष कॉन्फ़िगरेशन को नहीं)। कृपया निर्यात से पहले असहेजे गए परिवर्तन सहेजें।", "仅将 MQTT 数据点保存为 JSON，或从另一台终端导入（覆盖已保存的数据点，而非其余配置）。导出前请先保存未保存的更改。"}},
  {{"Datenpunkte exportieren", "Export data points", "Exporter les points de données", "Exportar puntos de datos", "Esporta punti dati", "Экспортировать точки данных", "डेटा पॉइंट निर्यात करें", "导出数据点"}},
  {{"Datenpunkte importieren", "Import data points", "Importer les points de données", "Importar puntos de datos", "Importa punti dati", "Импортировать точки данных", "डेटा पॉइंट आयात करें", "导入数据点"}},
  // --- Custom tab panels ---
  {{"Tab-Name, Sichtbarkeit und Widgets konfigurieren.", "Configure tab name, visibility and widgets.", "Configurer le nom de l'onglet, la visibilité et les widgets.", "Configurar nombre de pestaña, visibilidad y widgets.", "Configura nome scheda, visibilità e widget.", "Настройте имя вкладки, видимость и виджеты.", "टैब का नाम, दृश्यता और विजेट कॉन्फ़िगर करें।", "配置标签名称、可见性和小部件。"}},
  {{"Tab Name", "Tab name", "Nom de l'onglet", "Nombre de pestaña", "Nome scheda", "Имя вкладки", "टैब का नाम", "标签名称"}},
  // --- JS messages ---
  {{"Bitte eine Backup-Datei auswaehlen.", "Please select a backup file.", "Veuillez sélectionner un fichier de sauvegarde.", "Seleccione un archivo de copia de seguridad.", "Seleziona un file di backup.", "Пожалуйста, выберите файл резервной копии.", "कृपया एक बैकअप फ़ाइल चुनें।", "请选择一个备份文件。"}},
  {{"Wiederherstellen fehlgeschlagen.", "Restore failed.", "Échec de la restauration.", "Error al restaurar.", "Ripristino non riuscito.", "Восстановление не удалось.", "पुनर्स्थापना विफल रही।", "恢复失败。"}},
  {{"Bitte eine Firmware-Datei (.bin) auswaehlen.", "Please select a firmware file (.bin).", "Veuillez sélectionner un fichier firmware (.bin).", "Seleccione un archivo de firmware (.bin).", "Seleziona un file firmware (.bin).", "Пожалуйста, выберите файл прошивки (.bin).", "कृपया एक फर्मवेयर फ़ाइल (.bin) चुनें।", "请选择一个固件文件 (.bin)。"}},
  {{"Firmware jetzt aktualisieren? Das Geraet startet danach neu.", "Update firmware now? The device will restart afterwards.", "Mettre à jour le firmware maintenant ? L'appareil redémarrera ensuite.", "¿Actualizar el firmware ahora? El dispositivo se reiniciará después.", "Aggiornare il firmware ora? Il dispositivo si riavvierà dopo.", "Обновить прошивку сейчас? После этого устройство перезагрузится.", "अभी फर्मवेयर अपडेट करें? इसके बाद डिवाइस पुनः प्रारंभ होगा।", "现在更新固件？设备之后将重启。"}},
  {{"Wird hochgeladen... ", "Uploading... ", "Téléversement... ", "Subiendo... ", "Caricamento... ", "Загрузка... ", "अपलोड हो रहा है... ", "正在上传... "}},
  {{"Update erfolgreich, Geraet startet neu...", "Update successful, device is restarting...", "Mise à jour réussie, l'appareil redémarre...", "Actualización correcta, el dispositivo se reinicia...", "Aggiornamento riuscito, il dispositivo si riavvia...", "Обновление успешно, устройство перезагружается...", "अपडेट सफल, डिवाइस पुनः प्रारंभ हो रहा है...", "更新成功，设备正在重启..."}},
  {{"Update fehlgeschlagen.", "Update failed.", "Échec de la mise à jour.", "Error en la actualización.", "Aggiornamento non riuscito.", "Обновление не удалось.", "अपडेट विफल रहा।", "更新失败。"}},
  {{"Firmware-Update fehlgeschlagen.", "Firmware update failed.", "Échec de la mise à jour du firmware.", "Error en la actualización del firmware.", "Aggiornamento firmware non riuscito.", "Обновление прошивки не удалось.", "फर्मवेयर अपडेट विफल रहा।", "固件更新失败。"}},
  {{"Bitte eine WireGuard .conf-Datei auswaehlen.", "Please select a WireGuard .conf file.", "Veuillez sélectionner un fichier WireGuard .conf.", "Seleccione un archivo WireGuard .conf.", "Seleziona un file WireGuard .conf.", "Пожалуйста, выберите файл WireGuard .conf.", "कृपया एक WireGuard .conf फ़ाइल चुनें।", "请选择一个 WireGuard .conf 文件。"}},
  {{"VPN-Upload fehlgeschlagen.", "VPN upload failed.", "Échec du téléversement VPN.", "Error al subir la VPN.", "Caricamento VPN non riuscito.", "Загрузка VPN не удалась.", "VPN अपलोड विफल रहा।", "VPN 上传失败。"}},
  {{"VPN-Tunnel jetzt aktivieren? Die WebGUI ist im lokalen Netz dann NICHT mehr erreichbar, bis der Tunnel wieder getrennt wird (nur ueber die Tunnel-IP).", "Activate the VPN tunnel now? The WebGUI will then NO longer be reachable on the local network until the tunnel is disconnected again (only via the tunnel IP).", "Activer le tunnel VPN maintenant ? L'interface web ne sera alors PLUS accessible sur le réseau local jusqu'à ce que le tunnel soit à nouveau déconnecté (uniquement via l'IP du tunnel).", "¿Activar el túnel VPN ahora? La interfaz web ya NO será accesible en la red local hasta que se desconecte el túnel de nuevo (solo a través de la IP del túnel).", "Attivare il tunnel VPN ora? La WebGUI NON sarà più raggiungibile sulla rete locale finché il tunnel non viene disconnesso di nuovo (solo tramite l'IP del tunnel).", "Активировать VPN-туннель сейчас? После этого WebGUI больше НЕ будет доступна в локальной сети, пока туннель снова не будет отключён (только через IP туннеля).", "अभी VPN टनल सक्रिय करें? इसके बाद WebGUI स्थानीय नेटवर्क पर अब पहुँच योग्य नहीं होगा जब तक टनल फिर से डिस्कनेक्ट न हो जाए (केवल टनल IP के माध्यम से)।", "现在激活 VPN 隧道？之后 WebGUI 将不再能在本地网络访问，直到再次断开隧道（仅通过隧道 IP）。"}},
  {{"Laedt...", "Loading...", "Chargement...", "Cargando...", "Caricamento...", "Загрузка...", "लोड हो रहा है...", "加载中..."}},
  {{"Log konnte nicht geladen werden.", "Could not load log.", "Impossible de charger le log.", "No se pudo cargar el log.", "Impossibile caricare il log.", "Не удалось загрузить журнал.", "लॉग लोड नहीं हो सका।", "无法加载日志。"}},
  {{"Teste Verbindung...", "Testing connection...", "Test de la connexion...", "Probando conexión...", "Prova della connessione...", "Проверка соединения...", "कनेक्शन जाँच रहा है...", "正在测试连接..."}},
  {{"Test fehlgeschlagen (Netzwerkfehler).", "Test failed (network error).", "Échec du test (erreur réseau).", "Prueba fallida (error de red).", "Prova non riuscita (errore di rete).", "Проверка не удалась (ошибка сети).", "परीक्षण विफल (नेटवर्क त्रुटि)।", "测试失败（网络错误）。"}},
  {{"Bitte eine MQTT-Datenpunkte-Datei auswaehlen.", "Please select an MQTT data points file.", "Veuillez sélectionner un fichier de points de données MQTT.", "Seleccione un archivo de puntos de datos MQTT.", "Seleziona un file di punti dati MQTT.", "Пожалуйста, выберите файл точек данных MQTT.", "कृपया एक MQTT डेटा पॉइंट फ़ाइल चुनें।", "请选择一个 MQTT 数据点文件。"}},
  {{"MQTT-Datenpunkte aus der Datei uebernehmen? Die gespeicherten Datenpunkte werden ersetzt.", "Import MQTT data points from the file? The stored data points will be replaced.", "Importer les points de données MQTT du fichier ? Les points de données enregistrés seront remplacés.", "¿Importar los puntos de datos MQTT del archivo? Los puntos de datos guardados se reemplazarán.", "Importare i punti dati MQTT dal file? I punti dati salvati verranno sostituiti.", "Импортировать точки данных MQTT из файла? Сохранённые точки данных будут заменены.", "फ़ाइल से MQTT डेटा पॉइंट आयात करें? सहेजे गए डेटा पॉइंट बदल दिए जाएँगे।", "从文件导入 MQTT 数据点？已保存的数据点将被替换。"}},
  {{"Import fehlgeschlagen.", "Import failed.", "Échec de l'importation.", "Error en la importación.", "Importazione non riuscita.", "Импорт не удался.", "आयात विफल रहा।", "导入失败。"}},
  // --- Handler responses ---
  {{"Verbindung erfolgreich (", "Connection successful (", "Connexion réussie (", "Conexión correcta (", "Connessione riuscita (", "Соединение успешно (", "कनेक्शन सफल (", "连接成功 ("}},
  {{"Fehlgeschlagen: Zeitueberschreitung", "Failed: timeout", "Échec : délai dépassé", "Error: tiempo agotado", "Non riuscito: timeout", "Не удалось: тайм-аут", "विफल: टाइमआउट", "失败：超时"}},
  {{"Fehlgeschlagen: Broker nicht erreichbar", "Failed: broker unreachable", "Échec : broker inaccessible", "Error: broker inaccesible", "Non riuscito: broker irraggiungibile", "Не удалось: брокер недоступен", "विफल: ब्रोकर अगम्य", "失败：无法连接代理"}},
  {{"Fehlgeschlagen: Benutzer/Passwort falsch", "Failed: wrong user/password", "Échec : utilisateur/mot de passe incorrect", "Error: usuario/contraseña incorrectos", "Non riuscito: utente/password errati", "Не удалось: неверный логин/пароль", "विफल: गलत उपयोगकर्ता/पासवर्ड", "失败：用户名/密码错误"}},
  {{"Fehlgeschlagen: nicht autorisiert", "Failed: not authorized", "Échec : non autorisé", "Error: no autorizado", "Non riuscito: non autorizzato", "Не удалось: не авторизовано", "विफल: अधिकृत नहीं", "失败：未授权"}},
  {{"Fehlgeschlagen (Code ", "Failed (code ", "Échec (code ", "Error (código ", "Non riuscito (codice ", "Не удалось (код ", "विफल (कोड ", "失败（代码 "}},
  {{"Export fehlgeschlagen", "Export failed", "Échec de l'export", "Error al exportar", "Esportazione non riuscita", "Экспорт не удался", "निर्यात विफल", "导出失败"}},
  // --- Widget slot positions ---
  {{"Oben links", "Top left", "En haut à gauche", "Arriba izquierda", "In alto a sinistra", "Вверху слева", "ऊपर बाएँ", "左上"}},
  {{"Oben rechts", "Top right", "En haut à droite", "Arriba derecha", "In alto a destra", "Вверху справа", "ऊपर दाएँ", "右上"}},
  {{"Mitte links", "Middle left", "Milieu gauche", "Centro izquierda", "Centro sinistra", "В центре слева", "मध्य बाएँ", "中左"}},
  {{"Mitte rechts", "Middle right", "Milieu droite", "Centro derecha", "Centro destra", "В центре справа", "मध्य दाएँ", "中右"}},
  {{"Unten links", "Bottom left", "En bas à gauche", "Abajo izquierda", "In basso a sinistra", "Внизу слева", "नीचे बाएँ", "左下"}},
  {{"Unten rechts", "Bottom right", "En bas à droite", "Abajo derecha", "In basso a destra", "Внизу справа", "नीचे दाएँ", "右下"}},
  {{"Mitte oben links", "Upper middle left", "Milieu haut gauche", "Centro superior izquierda", "Centro alto sinistra", "Средний верх слева", "ऊपरी मध्य बाएँ", "中上左"}},
  {{"Mitte oben rechts", "Upper middle right", "Milieu haut droite", "Centro superior derecha", "Centro alto destra", "Средний верх справа", "ऊपरी मध्य दाएँ", "中上右"}},
  {{"Mitte unten links", "Lower middle left", "Milieu bas gauche", "Centro inferior izquierda", "Centro basso sinistra", "Средний низ слева", "निचला मध्य बाएँ", "中下左"}},
  {{"Mitte unten rechts", "Lower middle right", "Milieu bas droite", "Centro inferior derecha", "Centro basso destra", "Средний низ справа", "निचला मध्य दाएँ", "中下右"}},
  // --- File picker ---
  {{"Datei auswählen", "Choose file", "Choisir un fichier", "Elegir archivo", "Scegli file", "Выбрать файл", "फ़ाइल चुनें", "选择文件"}},
  {{"Keine Datei ausgewählt", "No file selected", "Aucun fichier sélectionné", "Ningún archivo seleccionado", "Nessun file selezionato", "Файл не выбран", "कोई फ़ाइल नहीं चुनी गई", "未选择文件"}},
  // --- Language names ---
  {{"Französisch", "French", "Français", "Francés", "Francese", "Французский", "फ़्रेंच", "法语"}},
  {{"Spanisch", "Spanish", "Espagnol", "Español", "Spagnolo", "Испанский", "स्पेनिश", "西班牙语"}},
  {{"Italienisch", "Italian", "Italien", "Italiano", "Italiano", "Итальянский", "इतालवी", "意大利语"}},
  {{"Russisch", "Russian", "Russe", "Ruso", "Russo", "Русский", "रूसी", "俄语"}},
  {{"Hindi", "Hindi", "Hindi", "Hindi", "Hindi", "Хинди", "हिन्दी", "印地语"}},
  {{"Chinesisch", "Chinese", "Chinois", "Chino", "Cinese", "Китайский", "चीनी", "中文"}},
  // --- WiFi slot editor (1.0.5) ---
  {{"SSID aus der Liste wählen oder eintippen und Passwort setzen. Ein leeres SSID-Feld deaktiviert den Slot. Passwörter werden nie angezeigt oder exportiert.", "Pick an SSID from the list or type it and set a password. An empty SSID field disables the slot. Passwords are never shown or exported.", "Choisissez un SSID dans la liste ou saisissez-le et définissez un mot de passe. Un champ SSID vide désactive l'emplacement. Les mots de passe ne sont jamais affichés ni exportés.", "Elige un SSID de la lista o escríbelo y define una contraseña. Un campo SSID vacío desactiva la ranura. Las contraseñas nunca se muestran ni se exportan.", "Scegli un SSID dall'elenco o digitalo e imposta una password. Un campo SSID vuoto disattiva lo slot. Le password non vengono mai mostrate né esportate.", "Выберите SSID из списка или введите его и задайте пароль. Пустое поле SSID отключает слот. Пароли никогда не отображаются и не экспортируются.", "सूची से एक SSID चुनें या टाइप करें और पासवर्ड सेट करें। खाली SSID फ़ील्ड स्लॉट को अक्षम कर देता है। पासवर्ड कभी नहीं दिखाए या निर्यात किए जाते हैं।", "从列表中选择或输入 SSID 并设置密码。SSID 字段为空则禁用该槽位。密码从不显示或导出。"}},
  {{"WLAN suchen", "Scan networks", "Rechercher les réseaux", "Buscar redes", "Cerca reti", "Искать сети", "नेटवर्क खोजें", "扫描网络"}},
  {{"(leer)", "(empty)", "(vide)", "(vacío)", "(vuoto)", "(пусто)", "(खाली)", "（空）"}},
  {{"Suche Netze...", "Scanning networks...", "Recherche des réseaux...", "Buscando redes...", "Ricerca reti...", "Поиск сетей...", "नेटवर्क खोज रहे हैं...", "正在扫描网络..."}},
  {{"WLANs gefunden, im SSID-Feld auswählen", "networks found, select in an SSID field", "réseaux trouvés, à choisir dans un champ SSID", "redes encontradas, elígela en un campo SSID", "reti trovate, selezionala in un campo SSID", "сетей найдено, выберите в поле SSID", "नेटवर्क मिले, SSID फ़ील्ड में चुनें", "个网络，在 SSID 字段中选择"}},
  {{"Suche fehlgeschlagen.", "Scan failed.", "Échec de la recherche.", "Error al buscar.", "Ricerca non riuscita.", "Поиск не удался.", "खोज विफल।", "扫描失败。"}},
  // --- SoftAP onboarding portal (1.0.6) ---
  {{"WLAN-Einrichtung", "WiFi setup", "Configuration WiFi", "Configuración WiFi", "Configurazione WiFi", "Настройка WiFi", "वाईफ़ाई सेटअप", "WiFi 设置"}},
  {{"Wähle dein WLAN aus der Liste, gib das Passwort ein und verbinde. Das InfoTerm startet danach im gewählten Netz.", "Pick your Wi-Fi from the list, enter the password and connect. InfoTerm will then join the selected network.", "Choisissez votre WiFi dans la liste, saisissez le mot de passe et connectez-vous. InfoTerm rejoindra ensuite le réseau sélectionné.", "Elige tu WiFi de la lista, introduce la contraseña y conecta. InfoTerm se unirá a la red seleccionada.", "Scegli la tua rete WiFi dall'elenco, inserisci la password e connetti. InfoTerm si unirà alla rete selezionata.", "Выберите свою сеть WiFi из списка, введите пароль и подключитесь. Затем InfoTerm подключится к выбранной сети.", "सूची से अपना वाईफ़ाई चुनें, पासवर्ड डालें और कनेक्ट करें। InfoTerm फिर चयनित नेटवर्क से जुड़ जाएगा।", "从列表中选择你的 WiFi，输入密码并连接。InfoTerm 随后将加入所选网络。"}},
  {{"Netze gefunden", "networks found", "réseaux trouvés", "redes encontradas", "reti trovate", "сетей найдено", "नेटवर्क मिले", "个网络"}},
  {{"Bitte ein Netz wählen.", "Please pick a network.", "Veuillez choisir un réseau.", "Elige una red.", "Scegli una rete.", "Выберите сеть.", "कृपया एक नेटवर्क चुनें।", "请选择一个网络。"}},
  {{"Verbinde...", "Connecting...", "Connexion...", "Conectando...", "Connessione...", "Подключение...", "कनेक्ट हो रहा है...", "正在连接..."}},
  {{"Zugangsdaten gespeichert - das InfoTerm verbindet sich jetzt. Der Setup-AP wird beendet, sobald die Verbindung steht.", "Credentials saved - InfoTerm is connecting now. The setup AP closes once the connection is up.", "Identifiants enregistrés - InfoTerm se connecte. Le point d'accès de configuration se ferme une fois la connexion établie.", "Credenciales guardadas - InfoTerm se está conectando. El AP de configuración se cierra cuando haya conexión.", "Credenziali salvate - InfoTerm si sta connettendo. L'AP di configurazione si chiude appena la connessione è attiva.", "Данные сохранены - InfoTerm подключается. Точка доступа настройки закроется, как только связь установится.", "क्रेडेंशियल सहेजे गए - InfoTerm अब कनेक्ट हो रहा है। कनेक्शन बनते ही सेटअप AP बंद हो जाएगा।", "凭据已保存 - InfoTerm 正在连接。连接建立后设置热点将关闭。"}},
  {{"Handy mit diesem WLAN verbinden:", "Connect your phone to this WiFi:", "Connectez votre téléphone à ce WiFi :", "Conecta tu móvil a este WiFi:", "Collega il telefono a questo WiFi:", "Подключите телефон к этому WiFi:", "अपना फ़ोन इस वाईफ़ाई से कनेक्ट करें:", "将手机连接到此 WiFi："}},
  {{"Setup-Portal starten", "Start setup portal", "Démarrer le portail de config", "Iniciar portal de config.", "Avvia portale di setup", "Запустить портал настройки", "सेटअप पोर्टल शुरू करें", "启动设置门户"}},
  {{"Setup-Portal jetzt starten? Das Gerät trennt die WLAN-Verbindung und oeffnet einen Setup-Access-Point - die WebGUI ist dann nur ueber diesen erreichbar, bis die Einrichtung abgeschlossen ist.", "Start the setup portal now? The device disconnects from Wi-Fi and opens a setup access point - the WebGUI is then only reachable via that AP until setup is done.", "Démarrer le portail de configuration maintenant ? L'appareil se déconnecte du WiFi et ouvre un point d'accès de configuration - l'interface web n'est alors accessible que via ce point d'accès jusqu'à la fin de la configuration.", "¿Iniciar el portal de configuración ahora? El dispositivo se desconecta del WiFi y abre un punto de acceso de configuración; la interfaz web solo será accesible por ese AP hasta terminar.", "Avviare ora il portale di setup? Il dispositivo si disconnette dal WiFi e apre un access point di configurazione - la WebGUI sarà raggiungibile solo tramite quello fino al termine.", "Запустить портал настройки сейчас? Устройство отключится от WiFi и откроет точку доступа настройки - WebGUI будет доступна только через неё до завершения настройки.", "सेटअप पोर्टल अभी शुरू करें? डिवाइस वाईफ़ाई से डिस्कनेक्ट होकर एक सेटअप एक्सेस पॉइंट खोलेगा - सेटअप पूरा होने तक WebGUI केवल उसी के माध्यम से पहुँच योग्य होगी।", "现在启动设置门户？设备将断开 WiFi 并开启设置热点 - 在完成设置前，WebGUI 只能通过该热点访问。"}}
};

static_assert(sizeof(LANGUAGE_TABLE) / sizeof(LANGUAGE_TABLE[0]) == L_COUNT,
              "LANGUAGE_TABLE entry count must match the LanguageKey enum (translations out of sync)");

// Full language index (0..LANG_COUNT-1). Used by the WebGUI (TXTW).
inline int clampLanguageIndex() {
  int l = (int)uiLanguage;
  if (l < 0 || l >= LANG_COUNT) return 0;
  return l;
}

// Display-safe index: the TFT fonts have no Cyrillic/Devanagari/CJK glyphs, so
// ru/hi/zh fall back to English on the physical display. Latin languages pass
// through unchanged. Always returns an index in [0, LANG_IT].
inline int displayLangIndex() {
  int l = clampLanguageIndex();
  return (l <= (int)LANG_IT) ? l : (int)LANG_EN;
}

// Display-safe accessor (used by all on-screen rendering + shared helpers).
inline const char* TXT(LanguageKey key) {
  if (key < 0 || key >= L_COUNT) return "";
  return LANGUAGE_TABLE[key].t[displayLangIndex()];
}

// Full-language accessor for the WebGUI (browser renders any Unicode script).
inline const char* TXTW(LanguageKey key) {
  if (key < 0 || key >= L_COUNT) return "";
  return LANGUAGE_TABLE[key].t[clampLanguageIndex()];
}

inline const char* languageCode() {
  switch (uiLanguage) {
    case LANG_EN: return "en";
    case LANG_FR: return "fr";
    case LANG_ES: return "es";
    case LANG_IT: return "it";
    case LANG_RU: return "ru";
    case LANG_HI: return "hi";
    case LANG_ZH: return "zh";
    default:      return "de";
  }
}

// Weekday/month names are display-only (device clock) and therefore indexed by
// the display-safe language index: only the five Latin languages are stored.
inline const char* localizedWeekdayShort(int tmWday) {
  static const char* days[5][7] = {
    {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"},
    {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"},
    {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"}
  };
  if (tmWday < 0 || tmWday > 6) return "";
  return days[displayLangIndex()][tmWday];
}

inline const char* localizedWeekdayLong(int tmWday) {
  static const char* days[5][7] = {
    {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
    {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"},
    {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"},
    {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"}
  };
  if (tmWday < 0 || tmWday > 6) return "";
  return days[displayLangIndex()][tmWday];
}

inline const char* localizedMonthShort(int tmMon) {
  static const char* months[5][12] = {
    {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"},
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    {"Jan", "Fév", "Mar", "Avr", "Mai", "Juin", "Juil", "Aoû", "Sep", "Oct", "Nov", "Déc"},
    {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"},
    {"Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic"}
  };
  if (tmMon < 0 || tmMon > 11) return "";
  return months[displayLangIndex()][tmMon];
}

inline UiLanguage languageFromCode(const String& value) {
  if (value == "en") return LANG_EN;
  if (value == "fr") return LANG_FR;
  if (value == "es") return LANG_ES;
  if (value == "it") return LANG_IT;
  if (value == "ru") return LANG_RU;
  if (value == "hi") return LANG_HI;
  if (value == "zh") return LANG_ZH;
  return LANG_DE;
}
