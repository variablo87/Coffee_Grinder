Import("env")

# access to global build environment
#print env

# access to project build environment (is used source files in "src" folder)
#print projenv

#
# Dump build environment (for debug purpose)
# print env.Dump()
#

#
# Change build flags in runtime
#
#env.ProcessUnFlags("-DVECT_TAB_ADDR")
#env.Append(CPPDEFINES=("VECT_TAB_ADDR", 0x123456789))

#
# Upload actions
#

def before_upload(source, target, env):
    print("before_upload")
    # do some actions

    # call Node.JS or other script
    env.Execute("node --version")


def after_upload(source, target, env):
    print("after_upload")
    # do some actions

def before_spiffs(source, target, env):
    print("zip html files")
    #print source
    # call python script
    env.Execute("python zipWebdata.py")

#print "Current build targets", map(str, BUILD_TARGETS)

#env.AddPreAction("upload", before_upload)
#env.AddPostAction("upload", after_upload)

# custom action before building SPIFFS image. For example, compress HTML, etc.
env.AddPreAction("$BUILD_DIR/spiffs.bin", before_spiffs)

#
# Custom actions when building program/firmware
#
def before_build(source, target, env):
    print("create version header")
    #print source
    # call python script
    env.Execute("python mkversion-1.py  src/version.h")

env.AddPreAction("$BUILD_DIR/src/Coffee_Grinder.cpp.o", before_build)
#env.AddPostAction("buildprog", callback...)