TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = server

SOURCES += \
	arithmetic_server.c \
	arithmetic_clientLocal.c \
	arithmetic_clientTCP.c \
	arithmetic_clientUDP.c \

OTHER_FILES += \
	Makefile \
	.gitignore \
