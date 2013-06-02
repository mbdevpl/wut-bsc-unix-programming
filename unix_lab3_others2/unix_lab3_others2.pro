TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = server

SOURCES += \
	server.c \
	client.c \

OTHER_FILES += \
	Makefile \
	.gitignore \
