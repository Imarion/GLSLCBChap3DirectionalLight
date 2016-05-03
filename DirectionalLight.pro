QT += gui core

CONFIG += c++11

TARGET = DirectionalLight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    dirlight.cpp \
    vertex.cpp \
    vertexcol.cpp \
    vertextex.cpp \
    torus.cpp

HEADERS += \
    dirlight.h \
    vertex.h \
    vertexcol.h \
    vertextex.h \
    torus.h

OTHER_FILES += \
    fshader_dirlight.txt \
    vshader_dirlight.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader_dirlight.txt \
    vshader_dirlight.txt
