Import("env")
import shutil

# print("Current CLI targets", COMMAND_LINE_TARGETS)
# print("Current Build targets", BUILD_TARGETS)

print("===== copying TFT config files ===== ")

# https://stackoverflow.com/questions/123198/how-to-copy-files

# source file location: EleksTubeHAX_pio\src\GLOBAL_DEFINES.h  &  _USER_DEFINES.h
# target file location: EleksTubeHAX_pio\.pio\libdeps\esp32dev\TFT_eSPI\User_Setup.h

# copy using Python libraries
# "copy" changes file timestamp -> lib is always recompiled.
# "copy2" keeps file timestamp -> lib is compiled once

import os
import shutil

# List of target file paths
targets = [
    'C:/PIO_LIBDEPS/display_S3_RELEASE/TFT_eSPI/User_Setup.h',
    'C:/PIO_LIBDEPS/display_S3_RELEASE_OTA/TFT_eSPI/User_Setup.h',
    'C:/PIO_LIBDEPS/display_S3_DEBUG/TFT_eSPI/User_Setup.h'
]

source = './Firmware/include/__CONFIG.h'

for dst in targets:
    target_dir = os.path.dirname(dst)
    
    # Check if the target folder exists
    if os.path.isdir(target_dir):
        shutil.copy2(source, dst)
        print(f"Copied successfully to: {dst}")
    else:
        print(f"Skipped: Target folder does not exist -> {target_dir}")

print("Done.")
