# Firmware: live timing on the Elecrow CrowPanel 7"

Connects straight from the ESP32-S3 to the official live-timing feeds and shows
a leaderboard plus session info on the 800x480 panel. Three series:

| Series | Source | Updates |
|---|---|---|
| Formula 1 | `livetiming.formula1.com/signalrcore` (SignalR Core) | pushed, sub-second |
| F1 Academy | `f2f3-prod-livetiming.azurewebsites.net/streaming` (SignalR 2, shared with F2/F3) | pushed, sub-second |
| Porsche Carrera Cup NA | Porsche's live-timing service (Al Kamel data), as used by racing.porsche.com | polled every 1 s during a session, every 10 s otherwise |

No accounts are needed.

## Setup

You need [PlatformIO](https://platformio.org/) (the VS Code extension or the
command-line tools) and a USB-C data cable.

1. Copy `include/config.example.h` to `include/config.h` and edit it. `config.h`
   is ignored by git, so your Wi-Fi password never ends up in the repository.
   - `WIFI_SSID` / `WIFI_PASSWORD`: your home network (2.4 GHz only).
   - `WIFI_SSID_2` / `WIFI_PASSWORD_2` (optional): e.g. your phone's hotspot for
     the track. The board joins whichever of the two it finds, and rejoins if
     the connection drops.
2. Optional: add the Porsche and F1 Academy keys and the series logos (both
   below).
3. Plug in the CrowPanel via USB and run:
   ```
   pio run -t upload
   pio device monitor
   ```

If PlatformIO picks the wrong serial port, add `upload_port = COMx` and
`monitor_port = COMx` under `[env:crowpanel7]` in `platformio.ini`.

`patch_websockets.py` runs automatically before each build. It raises the
WebSockets library's 15 KB frame limit, which the F1 Academy snapshot exceeds.

## Menu

Press and hold anywhere on the screen for about a second to open the menu:

- **Series:** tap the Formula 1, F1 Academy or Porsche Carrera Cup logo.
- **Schedule** (top right): upcoming sessions for all three series. Tap the
  list to see the next page (`MORE 1/2` in the top bar); it stays open until
  you tap CLOSE.
- **Brightness:** Low / Medium / High / Max (use Max in direct sun).
- **Auto-switch:** ON follows whichever series is on track (see below).
  Picking a series by hand turns it off.
- **Wi-Fi:** shows the network in use, IP address and signal strength. Tap it
  to change network (see below).

The menu stays open until you tap CLOSE (no screen changes by itself unless
auto-switch is on). Series, brightness and
auto-switch are remembered after power-off. From the serial monitor you can
also type `0`, `1` or `2` to pick a series, `m` to open the menu, `a` to
toggle auto-switch, `s` for the schedule page (`n` for its next page), `w`
for the Wi-Fi screen, `t<x>,<y>` for a tap at that point, or `p` for a
screenshot
(`python tools/screenshot.py COMx shot.png` saves one as a PNG).

## Changing Wi-Fi on the display

Menu > tap the Wi-Fi panel. The display scans and lists nearby 2.4 GHz
networks with their signal strength (the one in use is marked CONNECTED).

- Tap a network, type its password on the on-screen keyboard (shown as you
  type, in a font where 0/O and 1/l/I look different) and tap CONNECT.
  `123` / `ABC` switch between letters and symbols; `#+=` shows more symbols;
  SHIFT gives one capital letter.
- If it connects, the network is saved and live timing carries on. If not, the
  display goes back to the previous network and shows an error with your typing
  kept so you can fix it.
- **Other (hidden network)...** lets you type a network name first.
- Up to 5 networks are saved on the display (newest first), listed on the right
  with FORGET buttons. The two networks in `config.h` are always tried too; the
  display joins whichever known network it can find.

The Wi-Fi screens stay open until you tap CLOSE (or CANCEL on the keyboard).

## Auto-switch

While it's on, a background task (on the ESP32's second core, so the display
never waits) checks each series that isn't on screen in turn, about every
40 seconds per series:

- F1: `livetiming.formula1.com/static/SessionInfo.json` session status
- F1 Academy: a short second connection that asks only for the session status
- Porsche: the live-timing state; only a running **Carrera Cup** session counts
  (not another series at the same event), and not after the chequered flag

It decides in this order:

1. The series on screen has a session running, or one due within 3 minutes:
   stay.
2. Another series has a session running: switch to it.
3. Another series' session starts within 3 minutes (from the schedule): switch
   early, so the grid is already on screen at the start.
4. Nothing on track or due: show the **Up Next** screen (the next session and
   the list below it). Results stay up for 15 minutes after a session ends
   before Up Next takes over.

After any switch it stays put for at least 90 seconds. `AUTO` shows next to
the series name in the header while auto-switch is on.

## Schedule

Loaded at start-up and refreshed every 3 hours in the background:

- F1: [Jolpica](https://api.jolpi.ca/ergast/f1/current/races/) (start times;
  lengths assumed: practice/qualifying 60 min, race 120 min)
- F1 Academy: `api.formula1.com`, the API behind f1academy.com (start and end
  times; a gzip response, unpacked with the ESP32-S3's ROM decompressor)
- Porsche Carrera Cup: there is no schedule API, so enter the sessions in
  `PCC_SCHEDULE` in `include/config.h` once the COTA timetable is out, then
  upload again. Example:
  ```
  #define PCC_SCHEDULE \
    {"Practice",   "2026-10-22T10:00:00-05:00", 40}, \
    {"Qualifying", "2026-10-23T09:00:00-05:00", 30},
  ```
  (COTA in October is Central Daylight Time, `-05:00`.)

The leaderboard's bottom bar always shows the next session across all three
series.

## Porsche and F1 Academy keys

Two of the data sources need a key that is embedded in the series' own public
web pages. They are not included here; leave them empty and those parts simply
don't load.

- `PCC_API_KEY` (Porsche live timing): open
  racing.porsche.com/en-US/carrera-cup/live-timing in a desktop browser, open
  the developer tools' Network tab, and copy the `x-api-key` header from the
  request to the live-timing `state/alkamel` endpoint.
- `SCHEDULE_FA_KEY` (F1 Academy schedule): on f1academy.com, copy the `apikey`
  header from the request to `api.formula1.com/v1/f2f3-fom-results/races`.

F1 and F1 Academy live timing need no key.

## Series logos

The menu buttons can show the series' logos. They are the series' trademarks,
so they are not in this repository: without them, the buttons show the series
names. To add them for your own display:

```
pip install pillow pymupdf
python tools/make_logos.py
```

That downloads the official logo files and writes `include/logos.h`, which is
ignored by git.

## Screen

- **Header:** event name, series (in its accent colour), session and status,
  `LAP x / y` (when the feed gives a lap count) or the session countdown, flag
  state (green / yellow / SC / VSC / FCY / red / chequered) with the local time
  of day under it, connection dot (green = live, orange = Wi-Fi only,
  red = offline). Set the time zone and 12/24-hour format with `TIMEZONE` and
  `CLOCK_24H` in `config.h` (default: US Central, 12-hour).
- **Rows:** position, colour bar (F1: team; Porsche: class), driver code (F1,
  F1 Academy) or car number (Porsche), then
  - race: interval, gap to leader, last lap
  - practice/qualifying: best lap, gap to fastest, last lap
  - F1 only: tyre compound and laps on the set.
  Last lap is green for a personal best and purple for the overall fastest.
  Shows IN PIT / PIT OUT / OUT / STOP.
- Up to 22 cars use large rows; bigger fields (up to 40) shrink the rows.

## Notes

- The Porsche feed follows whatever is on track at the current Porsche/IMSA
  event. When it's another series (e.g. an IMSA support race), the session line
  shows that series' name.
- F1 Academy has no team colours in its feed, so its colour bar is grey.
- Between sessions every feed keeps showing the last session's result.
- Data use on a phone hotspot: Porsche polling is roughly 70 MB per hour while a
  session runs; F1 and F1 Academy use far less.
- The Porsche endpoint and key come from Porsche's public live-timing page
  (see `config.h`). If Porsche changes their site, they may need updating.
- **Touch not responding** (also on Elecrow's own demo): with the board powered,
  hold **BOOT and RESET together for about 10 seconds** and release. That
  brought touch back on two brand-new boards whose touch controller wasn't
  answering out of the box. To check, type `d` in the serial monitor: a working
  board lists the touch controller (`0x5D` or `0x14`) under "I2C devices" next
  to the expander (`0x18`). The firmware rechecks every few seconds and starts
  using touch as soon as the controller answers. Without touch, the serial
  commands above still switch series and auto-switch.
- The firmware never writes to the board's PCA9557 expander (which carries the
  touch controller's reset and INT lines), matching Elecrow's own V3.0 example.
  An earlier version pulsed that reset; touch later stopped on two boards, so it
  was removed to be safe.
- If the picture flickers or shifts under Wi-Fi load, lower `LCD_PCLK_HZ`
  in `config.h` (default 12 MHz; try 10 MHz, then 8.5 MHz).
- On Windows, keep the project in a short path (e.g. `C:\race-tracker-5000`);
  LovyanGFX fails to build when the path exceeds the 260-character limit.

## Adding your own series

Each series is one class that implements `TimingSource` (`src/source.h`) and
writes into the shared timing model, `F1State` (`src/f1state.h`). The UI only
reads `F1State`, so a new series needs no drawing code. F1 Academy
(`src/fa_feed.*` and `src/fa_parse.*`) is the simplest complete example.

1. **Find the feed.** Open the series' live-timing page in a desktop browser
   with the developer tools' Network tab open during a session. Look for a
   WebSocket (SignalR and similar) or a JSON endpoint that is polled. Note the
   URL, any key or header it sends, and what one update looks like.
2. **Write a parser** (`src/xx_parse.cpp/.h`) that turns one message into
   `F1State`: fill `session` (meeting, name, type, status, laps or clock, flag
   in `trackStatus`) and one `Driver` per car (position, label in `tla`, colour,
   gaps, lap times). Keep it free of Arduino headers, like `fa_parse.cpp`, so
   you can test it on a PC against a saved message.
3. **Write the source class** (`src/xx_feed.cpp/.h`) implementing
   `TimingSource`:
   - `title()`, `accent()`, `menuName()`: header text, accent colour
     (`0xRRGGBB`) and the name on the menu button. Override `driverHeading()`
     if the column should read something other than DRIVER.
   - `start()`, `stop()`, `loop()`, `live()`: connect, disconnect, pump the
     connection, and report whether data is arriving. Only the series on
     screen is started.
   - `running()`: whether the data in `F1State` shows a session in progress.
   - `probe()`: a quick one-off check, run from the background task while the
     series is off screen, for auto-switch. Return 1 (running), 0 (not) or -1
     (check failed). It must not use the live connection.
   - Shared helpers: `src/net_util.*` (HTTPS requests) and `src/psram.h`
     (large buffers in PSRAM).
4. **Register it** in `src/main.cpp`: add an instance to `sources[]` and a
   matching entry to `sourceLogos[]` (`nullptr` shows the name). The menu fits
   up to four series (`MENU_MAX_SERIES` in `src/ui.h`).
5. **Settings:** put the URL and any key in `config.example.h` (and your own
   `config.h`).
6. **Schedule (optional):** to have the series in the schedule page and the
   NEXT bar, add a fetch in `scheduleTick()` (`src/schedule.cpp`) and a parser
   in `src/schedule_parse.cpp`, or list its sessions in `config.h` the way
   `PCC_SCHEDULE` does.

Feeds that aren't published for outside use can change without notice, so
expect to revisit a parser now and then.
