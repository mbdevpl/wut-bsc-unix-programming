TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = server

SOURCES += \
	client.c \
	server.c \

HEADERS += \
	mbdev_unix.h \
	common.h \

OTHER_FILES += \
	Makefile \
	.gitignore \
