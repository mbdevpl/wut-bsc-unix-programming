TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = fifos

SOURCES += \
	fifos.c

HEADERS += \
	mbdev_unix.h \

OTHER_FILES += Makefile
