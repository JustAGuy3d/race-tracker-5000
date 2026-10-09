#pragma once
// Copy this file to config.h (same folder) and fill in your details.
// config.h is ignored by git, so your Wi-Fi password stays on your PC.

// ---- Wi-Fi ----------------------------------------------------------------
#define WIFI_SSID     "your-network"
#define WIFI_PASSWORD "your-password"

// Optional second network, e.g. your phone's hotspot at the track (set the
// hotspot to 2.4 GHz / "maximize compatibility"). Leave empty if unused.
// The board joins whichever of the two it can find.
#define WIFI_SSID_2     ""
#define WIFI_PASSWORD_2 ""

// ---- Display ----------------------------------------------------------------
// Pixel clock for the RGB panel. If the picture flickers or shifts while
// Wi-Fi is busy, lower this (10 MHz, then 8.5 MHz).
#define LCD_PCLK_HZ   12000000
#define LCD_BRIGHTNESS 200   // 0-255

// ---- Feeds ------------------------------------------------------------------
// Press and hold the screen for about a second to switch series. The choice is
// remembered across power cycles; DEFAULT_SERIES is used on first boot.
// 0 = Formula 1, 1 = F1 Academy, 2 = Porsche Carrera Cup North America
#define DEFAULT_SERIES 0

// Auto-switch (toggle in the menu; this is only the first-boot default): when
// the series on screen has no session running and another one does, switch to it.
#define DEFAULT_AUTO_SWITCH 0

// ---- Local time -------------------------------------------------------------
// POSIX time zone for the clock under the flag box. COTA is US Central.
//   Eastern  "EST5EDT,M3.2.0,M11.1.0"     Mountain "MST7MDT,M3.2.0,M11.1.0"
//   Central  "CST6CDT,M3.2.0,M11.1.0"     Pacific  "PST8PDT,M3.2.0,M11.1.0"
#define TIMEZONE  "CST6CDT,M3.2.0,M11.1.0"
#define CLOCK_24H 0   // 1 for 14:35, 0 for 2:35 PM

#define F1_HOST "livetiming.formula1.com"
#define F1_PATH "/signalrcore"

// F1 Academy shares the F2/F3 timing service.
#define FA_HOST   "f2f3-prod-livetiming.azurewebsites.net"
#define FA_PATH   "/streaming"
#define FA_SERIES "F1 Academy"

// Endpoint and key used by racing.porsche.com/en-US/carrera-cup/live-timing
// (the key is public, embedded in that page). If Porsche changes their site,
// these may need updating.
#define PCC_URL     "https://app-live-timing-dev-nestjs.azurewebsites.net/v1/state/alkamel"
#define PCC_API_KEY ""   // see README: "Porsche and F1 Academy keys"

// ---- Schedule -----------------------------------------------------------------
// F1: Jolpica (community successor to the Ergast API), no key needed.
#define SCHEDULE_F1_URL "https://api.jolpi.ca/ergast/f1/current/races/?limit=100"
// F1 Academy: the API behind f1academy.com (public key from that site).
#define SCHEDULE_FA_URL "https://api.formula1.com/v1/f2f3-fom-results/races?website=fa"
#define SCHEDULE_FA_KEY ""   // see README: "Porsche and F1 Academy keys"

// Porsche Carrera Cup has no schedule API, so list its sessions here once the
// COTA timetable is published: { "name", "local start with UTC offset", minutes }.
// COTA in October is Central Daylight Time, offset -05:00. The example lines
// below are placeholders: replace them with the real times and delete the
// /* and */ around each line you use. Keep the comma and the backslash at the
// end of every line.
#define PCC_SCHEDULE \
  /* {"Practice",   "2026-10-22T10:00:00-05:00", 40}, */ \
  /* {"Qualifying", "2026-10-23T09:00:00-05:00", 30}, */ \
  /* {"Race 1",     "2026-10-24T11:00:00-05:00", 40}, */ \
  /* {"Race 2",     "2026-10-25T09:30:00-05:00", 40}, */
