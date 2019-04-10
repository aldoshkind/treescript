QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    $$TREE_PATH/tree/filepath_utils.cpp \
    $$TREE_PATH/tree/property_listener.cpp \
    $$TREE_PATH/tree/tree_node.cpp \
    $$TREE_PATH/tree/tree_node_inherited.cpp \
    interpreter.cpp

INCLUDEPATH += $$PWD/parsertl
INCLUDEPATH += $$PWD/lexertl
INCLUDEPATH += $$TREE_PATH

HEADERS += \
    $$TREE_PATH/tree/filepath_utils.h \
    $$TREE_PATH/tree/property.h \
    $$TREE_PATH/tree/property_listener.h \
    $$TREE_PATH/tree/tree_node.h \
    $$TREE_PATH/tree/tree_node_inherited.h \
    types.h \
    term.h \
    value.h \
    interpreter.h \
    typesystem.h \
    typeinfo.h
