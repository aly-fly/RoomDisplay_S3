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
#shutil.copy2('./include/__CONFIG.h', 'C:/PIO_LIBDEPS/display_S3_JLINK_debug/TFT_eSPI/User_Setup.h')
shutil.copy2('./include/__CONFIG.h', 'C:/PIO_LIBDEPS/display_S3_RELEASE/TFT_eSPI/User_Setup.h')
shutil.copy2('./include/__CONFIG.h', 'C:/PIO_LIBDEPS/display_S3_DEBUG/TFT_eSPI/User_Setup.h')

# copy using Windows command line
# native "copy" command keeps file timestamp -> lib is compiled once
# env.Execute("copy .\\src\\_USER_DEFINES.h .\\.pio\\libdeps\\esp32dev\\TFT_eSPI")
# env.Execute("copy .\\src\\GLOBAL_DEFINES.h .\\.pio\\libdeps\\esp32dev\\TFT_eSPI\\User_Setup.h")

print("Done.")
