#!/bin/bash

set -euo pipefail

VERSION=1.0
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

cd "$SCRIPT_DIR"

if [ "${EUID}" -ne 0 ]; then
	echo "You are not running this script as root!"
	exit 1
fi

echo "==== REMOVING OLD VERSIONS ===="
old_vers=$(dkms status | awk -F', ' '/^t150\// {split($1, parts, "/"); print parts[2]}')
if [ -n "$old_vers" ]; then
	IFS=$'\n'
	for version in $old_vers
	do
		[ -n "$version" ] || continue
		dkms remove -m t150 -v "$version" --all
	done
	unset IFS
fi
rmmod hid_t150 2>/dev/null || true

echo "==== CONFIG DKMS ===="
rm -rf "/usr/src/t150-$VERSION"
install -d "/usr/src/t150-$VERSION/build"

cp -TR ./hid-t150 "/usr/src/t150-$VERSION/hid-t150"
cp ./dkms_make.mak "/usr/src/t150-$VERSION/Makefile"
cp ./dkms.conf "/usr/src/t150-$VERSION/"

echo "==== DKMS ===="
dkms add -m t150 -v $VERSION --verbose
dkms build -m t150 -v $VERSION --verbose
dkms install -m t150 -v $VERSION --verbose

echo "==== INSTALLING UDEV RULES ===="
cp -vR ./files/. /
udevadm control --reload
udevadm trigger

echo "==== LOADING NEW MODULES ===="
modprobe hid_t150
