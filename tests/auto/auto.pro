# TODO: Change this to subdirs once we get first C++ test
TEMPLATE = aux

OTHER_FILES += \
    *.xml

# Enable something like
#check.commands += cd tst_xyz && ./tst_xyz && cd .. \
#              && cd qmltest && qmltestrunner
#QMAKE_EXTRA_TARGETS += check
