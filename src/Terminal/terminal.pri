# Terminal Module - Qt Project Include File
# Add this to WindTerm.pro: include(src/Terminal/terminal.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/TerminalMode.h \
    $$PWD/SynchronizedBuffer.h \
    $$PWD/CSIParser.h \
    $$PWD/TerminalHandler.h \
    $$PWD/../Utility/Spin.h \
    $$PWD/../Utility/CircularBuffer.h

SOURCES += \
    $$PWD/TerminalMode.cpp \
    $$PWD/SynchronizedBuffer.cpp \
    $$PWD/CSIParser.cpp \
    $$PWD/TerminalHandler.cpp

# Optional: Build example and tests
# SOURCES += $$PWD/example_csi2026.cpp
# SOURCES += $$PWD/test_csi2026.cpp
