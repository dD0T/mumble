include(../../compiler.pri)

!exists(../speexdsp-src/COPYING) {
	message("The speexdsp-src/ directory was not found. You need to do one of the following:")
	message("")
	message("Option 1: Use speexdsp as a git submodule:")
	message("git submodule init")
	message("git submodule update")
	message("")
	message("Option 2: Use system speexdsp:")
	message("qmake CONFIG+=no-bundled-speexdsp -recursive")
	message("")
	error("Aborting configuration")
}

CONFIG(debug, debug|release) {
  CONFIG += console
  DESTDIR	= ../../debug
}

CONFIG(release, debug|release) {
  DESTDIR	= ../../release
}

TEMPLATE = lib
CONFIG -= qt
CONFIG += debug_and_release
CONFIG -= warn_on
CONFIG += warn_off
CONFIG += no_include_pwd
VPATH	= ../speexdsp-src/libspeexdsp
TARGET = speexdsp
DEFINES += NDEBUG HAVE_CONFIG_H
INCLUDEPATH = ../speexdsp-src/include ../speexdsp-src/libspeex

win32 {
  INCLUDEPATH += ../speexdsp-src/win32
  DEFINES+=WIN32 _WINDOWS _USE_SSE _USE_MATH_DEFINES

  CONFIG -= static
  CONFIG += shared

  CONFIG(sse2) {
    TARGET = speexdsp.sse2
    DEFINES += _USE_SSE2
  } else {
    QMAKE_CFLAGS_RELEASE -= -arch:SSE
    QMAKE_CFLAGS_DEBUG -= -arch:SSE
  }
} else {
  CONFIG += staticlib
  INCLUDEPATH += ../speexdsp-build/
}

DEF_FILE = ../speexdsp-src/win32/libspeexdsp.def
DIST = config.h $${DEF_FILE} speex/speex_config_types.h

SOURCES *= buffer.c fftwrap.c filterbank.c jitter.c kiss_fft.c kiss_fftr.c mdf.c preprocess.c resample.c scal.c smallft.c

