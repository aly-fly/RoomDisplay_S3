
#  https://github.com/sblantipodi/platformio_version_increment

import datetime
import os

Import("env")

VERSION_FILE = 'version.txt'
VERSION_HEADER = 'Version.h'
VERSION_PREFIX = '0.1.'
VERSION_PATCH_NUMBER = 0

print("===== Project version file update ===== ")

try:
    with open(VERSION_FILE) as FILE:
        VERSION_PATCH_NUMBER = FILE.readline()
        VERSION_PREFIX = VERSION_PATCH_NUMBER[0:VERSION_PATCH_NUMBER.rindex('.')+1]
        VERSION_PATCH_NUMBER = int(VERSION_PATCH_NUMBER[VERSION_PATCH_NUMBER.rindex('.')+1:])
        if not os.path.exists(".version_no_increment_update_date"):
            VERSION_PATCH_NUMBER = VERSION_PATCH_NUMBER + 1
except:
    print('No version file found or incorrect data in it. Starting from 0.1.0')
    VERSION_PATCH_NUMBER = 0
    
with open(VERSION_FILE, 'w+') as FILE:
    FILE.write(VERSION_PREFIX + str(VERSION_PATCH_NUMBER))
    print('Build number: {}'.format(VERSION_PREFIX + str(VERSION_PATCH_NUMBER)))

HEADER_FILE = """
// AUTO GENERATED FILE, DO NOT EDIT
#ifndef VERSION
    #define VERSION "{}"
#endif
#ifndef BUILD_TIMESTAMP
    #define BUILD_TIMESTAMP "{}"
#endif
""".format(VERSION_PREFIX + str(VERSION_PATCH_NUMBER), datetime.datetime.now())

if os.environ.get('PLATFORMIO_INCLUDE_DIR') is not None:
    VERSION_HEADER = os.environ.get('PLATFORMIO_INCLUDE_DIR') + os.sep + VERSION_HEADER
elif os.path.exists("include"):
    VERSION_HEADER = "include" + os.sep + VERSION_HEADER
else:
    PROJECT_DIR = env.subst("$PROJECT_DIR")
    os.mkdir(PROJECT_DIR + os.sep + "include")
    VERSION_HEADER = "include" + os.sep + VERSION_HEADER



with open(VERSION_HEADER, 'w+') as FILE:
    FILE.write(HEADER_FILE)


print("Done.")
