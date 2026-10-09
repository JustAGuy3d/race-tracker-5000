# PlatformIO pre-build script.
#
# links2004/WebSockets hard-codes a 15 KB maximum frame size on ESP32 and drops the
# connection on anything bigger. The F1 Academy timing server sends its ~35 KB
# snapshot as a single frame, so raise the limit (payloads are allocated in PSRAM).
import os

Import("env")  # noqa: F821  (provided by PlatformIO)

OLD = "#define WEBSOCKETS_MAX_DATA_SIZE (15 * 1024)"
NEW = "#define WEBSOCKETS_MAX_DATA_SIZE (256 * 1024)  /* patched by patch_websockets.py */"

header = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"),  # noqa: F821
                      "WebSockets", "src", "WebSockets.h")

if not os.path.isfile(header):
    print("patch_websockets: WebSockets.h not found yet at", header)
else:
    with open(header, encoding="utf-8") as f:
        text = f.read()
    if NEW not in text:
        # The first occurrence is the ESP8266/ESP32 branch.
        text = text.replace(OLD, NEW, 1)
        with open(header, "w", encoding="utf-8") as f:
            f.write(text)
        print("patch_websockets: raised WEBSOCKETS_MAX_DATA_SIZE to 256 KB")
