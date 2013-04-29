TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = server

SOURCES += \
	lab4server.c \
	mbdev_unix.c \

HEADERS += \
	mbdev_unix.h \

OTHER_FILES += \
	Makefile \
	.gitignore \
