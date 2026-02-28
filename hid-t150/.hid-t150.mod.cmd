savedcmd_hid-t150.mod := printf '%s\n'   hid-t150.o | awk '!x[$$0]++ { print("./"$$0) }' > hid-t150.mod
