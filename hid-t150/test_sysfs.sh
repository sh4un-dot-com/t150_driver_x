#!/usr/bin/env bash
set -euo pipefail

# Simple tester for the hid-t150 sysfs attributes.
# - Finds the first device with a firmware_version attribute
# - Prints current settings
# - Triggers a reload
# - Optionally sets gain when run as root

attr=$(find /sys/devices -name firmware_version 2>/dev/null | head -n1 || true)
if [ -z "$attr" ]; then
  echo "firmware_version not found. Is the module loaded?"
  exit 1
fi

devdir=$(dirname "$attr")
echo "Device dir: $devdir"

echo "Firmware version:"
cat "$devdir/firmware_version" || true

echo "Range:"
cat "$devdir/range" || true

echo "Autocenter:"
cat "$devdir/autocenter" || true

echo "Gain:"
cat "$devdir/gain" || true

echo
echo "Refreshing settings (writing to reload_settings)..."
if [ -w "$devdir/reload_settings" ]; then
  echo 1 > "$devdir/reload_settings"
  echo "Reload written"
else
  echo "Cannot write reload_settings; try with sudo"
  sudo sh -c "echo 1 > '$devdir/reload_settings'" || true
fi

echo "Updated values:"
cat "$devdir/firmware_version" || true
cat "$devdir/range" || true
cat "$devdir/gain" || true

if [ "$(id -u)" -ne 0 ]; then
  echo "Not running as root; skipping write tests (gain change)."
  exit 0
fi

echo "Setting gain to 32768..."
printf "%d\n" 32768 > "$devdir/gain"
echo "Gain now: $(cat $devdir/gain)"

echo "Done."