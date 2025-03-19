Import("env")
import shutil

# print("Current CLI targets", COMMAND_LINE_TARGETS)
# print("Current Build targets", BUILD_TARGETS)

print("===== copying board define file ===== ")

# https://stackoverflow.com/questions/123198/how-to-copy-files

# source file location: .\esp32s3_r8n16.json
# target file location: EleksTubeHAX_pio\.pio\libdeps\esp32dev\TFT_eSPI\User_Setup.h
# https://docs.platformio.org/en/latest/platforms/creating_board.html
# https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/core_dir.html
# 
# copy using Python libraries
# "copy" changes file timestamp -> lib is always recompiled.
# "copy2" keeps file timestamp -> lib is compiled once
# shutil.copy2('./esp32s3_r8n16.json', '%HOMEPATH%/.platformio/boards/esp32s3_r8n16.json')

# copy using Windows command line
# native "copy" command keeps file timestamp -> lib is compiled once
env.Execute("copy .\\esp32s3_r8n16.json %HOMEDRIVE%%HOMEPATH%\\.platformio\\boards\\")

print("Done.")
