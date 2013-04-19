TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = server

SOURCES += \
	server.c \
    client.c

#HEADERS += \
#	mbdev_unix.h \

OTHER_FILES += Makefile
