TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = program

SOURCES += main.c

HEADERS += mbdev_unix.h

OTHER_FILES += Makefile
