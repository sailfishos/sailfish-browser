# Add more folders to ship with the application, here
folder_01.source = .
folder_01.target = app
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

OTHER_FILES = qml/*.qml

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += memory-dump-reader.cpp

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
#qtcAddDeployment()
