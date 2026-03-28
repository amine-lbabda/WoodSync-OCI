QT       += core gui sql charts network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    connection.cpp \
    employes.cpp \
    main.cpp \
    mainwindow.cpp \
    material.cpp \
    geminiclient.cpp \
    smtpclient.cpp \
    emaildialog.cpp

HEADERS += \
    connection.h \
    mainwindow.h \
    material.h \
    geminiclient.h \
    smtpclient.h \
    emaildialog.h \
    employes.h \
    dotenv.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ressources.qrc

DISTFILES += \
    .env \
    .env.example \
    README.md \
    face_detection_yunet_2023mar.onnx \
    face_model.xml \
    face_recognition_sface_2021dec.onnx

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4

    unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lqtcsv
    INCLUDEPATH += $$PWD/../../../../usr/local/include/qtcsv
    DEPENDPATH += $$PWD/../../../../usr/local/include/qtcsv

    unix:!macx: LIBS += -L$$PWD/../../Qt/6.7.3/gcc_64/lib/ -lqt6keychain
    INCLUDEPATH += $$PWD/../../Qt/6.7.3/gcc_64/include/qt6keychain
    DEPENDPATH += $$PWD/../../Qt/6.7.3/gcc_64/include/qt6keychain
}

win32 {
    # OpenCV package in this workspace provides only release binaries on Windows.
    # In Debug builds, disable OpenCV debug namespace guard to match release symbols.
    CONFIG(debug, debug|release): DEFINES += CV_IGNORE_DEBUG_BUILD_GUARD

    OPENCV_ROOT = $$PWD/../../../../../opencv_total/install
    OPENCV_LIB_DIR = $$OPENCV_ROOT/x64/vc18/lib

    LIBS += -L$$OPENCV_LIB_DIR -lopencv_world4140
    INCLUDEPATH += $$OPENCV_ROOT/include
    DEPENDPATH += $$OPENCV_ROOT/include

    win32-g++:exists($$OPENCV_LIB_DIR/libopencv_world4140.a): PRE_TARGETDEPS += $$OPENCV_LIB_DIR/libopencv_world4140.a
    else:win32:!win32-g++:exists($$OPENCV_LIB_DIR/opencv_world4140.lib): PRE_TARGETDEPS += $$OPENCV_LIB_DIR/opencv_world4140.lib

    QT6KEYCHAIN_ROOT = $$PWD/../../../../../opencv_total/qt6keychain_final
    QT6KEYCHAIN_LIB_DIR = $$QT6KEYCHAIN_ROOT/lib

    LIBS += -L$$QT6KEYCHAIN_LIB_DIR -lqt6keychain
    INCLUDEPATH += $$QT6KEYCHAIN_ROOT/include
    DEPENDPATH += $$QT6KEYCHAIN_ROOT/include

    win32-g++:exists($$QT6KEYCHAIN_LIB_DIR/libqt6keychain.a): PRE_TARGETDEPS += $$QT6KEYCHAIN_LIB_DIR/libqt6keychain.a
    else:win32:!win32-g++:exists($$QT6KEYCHAIN_LIB_DIR/qt6keychain.lib): PRE_TARGETDEPS += $$QT6KEYCHAIN_LIB_DIR/qt6keychain.lib

    QTCSV_ROOT = $$PWD/../../../../../opencv_total/qtcsv
    QTCSV_DEBUG_DIR = $$QTCSV_ROOT/debug
    QTCSV_RELEASE_DIR = $$QTCSV_ROOT/release

    CONFIG(release, debug|release): LIBS += -L$$QTCSV_RELEASE_DIR -lqtcsv
    else:CONFIG(debug, debug|release) {
        exists($$QTCSV_DEBUG_DIR/qtcsv.lib): LIBS += -L$$QTCSV_DEBUG_DIR -lqtcsv
        else: LIBS += -L$$QTCSV_RELEASE_DIR -lqtcsv
    }

    INCLUDEPATH += $$QTCSV_ROOT/include
    DEPENDPATH += $$QTCSV_ROOT/include

    win32-g++:CONFIG(release, debug|release):exists($$QTCSV_RELEASE_DIR/libqtcsv.a): PRE_TARGETDEPS += $$QTCSV_RELEASE_DIR/libqtcsv.a
    else:win32-g++:CONFIG(debug, debug|release) {
        exists($$QTCSV_DEBUG_DIR/libqtcsv.a): PRE_TARGETDEPS += $$QTCSV_DEBUG_DIR/libqtcsv.a
        else:exists($$QTCSV_RELEASE_DIR/libqtcsv.a): PRE_TARGETDEPS += $$QTCSV_RELEASE_DIR/libqtcsv.a
    }
    else:win32:!win32-g++:CONFIG(release, debug|release):exists($$QTCSV_RELEASE_DIR/qtcsv.lib): PRE_TARGETDEPS += $$QTCSV_RELEASE_DIR/qtcsv.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release) {
        exists($$QTCSV_DEBUG_DIR/qtcsv.lib): PRE_TARGETDEPS += $$QTCSV_DEBUG_DIR/qtcsv.lib
        else:exists($$QTCSV_RELEASE_DIR/qtcsv.lib): PRE_TARGETDEPS += $$QTCSV_RELEASE_DIR/qtcsv.lib
    }

    OPENCV_DLL = $$OPENCV_ROOT/x64/vc18/bin/opencv_world4140.dll
    QT6KEYCHAIN_DLL = $$QT6KEYCHAIN_ROOT/bin/qt6keychain.dll
    QTCSV_DLL_DEBUG = $$QTCSV_DEBUG_DIR/qtcsv.dll
    QTCSV_DLL_RELEASE = $$QTCSV_RELEASE_DIR/qtcsv.dll

    DLL_TARGET_DIR_DEBUG = $$OUT_PWD/debug
    DLL_TARGET_DIR_RELEASE = $$OUT_PWD/release

    # Copy third-party runtime DLLs next to the executable to prevent startup failures.
    exists($$OPENCV_DLL) {
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_DEBUG)" copy /Y "$$shell_path($$OPENCV_DLL)" "$$shell_path($$DLL_TARGET_DIR_DEBUG)\\" $$escape_expand(\n\t)
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_RELEASE)" copy /Y "$$shell_path($$OPENCV_DLL)" "$$shell_path($$DLL_TARGET_DIR_RELEASE)\\" $$escape_expand(\n\t)
    }

    exists($$QT6KEYCHAIN_DLL) {
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_DEBUG)" copy /Y "$$shell_path($$QT6KEYCHAIN_DLL)" "$$shell_path($$DLL_TARGET_DIR_DEBUG)\\" $$escape_expand(\n\t)
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_RELEASE)" copy /Y "$$shell_path($$QT6KEYCHAIN_DLL)" "$$shell_path($$DLL_TARGET_DIR_RELEASE)\\" $$escape_expand(\n\t)
    }

    exists($$QTCSV_DLL_DEBUG) {
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_DEBUG)" copy /Y "$$shell_path($$QTCSV_DLL_DEBUG)" "$$shell_path($$DLL_TARGET_DIR_DEBUG)\\" $$escape_expand(\n\t)
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_RELEASE)" copy /Y "$$shell_path($$QTCSV_DLL_DEBUG)" "$$shell_path($$DLL_TARGET_DIR_RELEASE)\\" $$escape_expand(\n\t)
    } else:exists($$QTCSV_DLL_RELEASE) {
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_DEBUG)" copy /Y "$$shell_path($$QTCSV_DLL_RELEASE)" "$$shell_path($$DLL_TARGET_DIR_DEBUG)\\" $$escape_expand(\n\t)
        QMAKE_POST_LINK += if exist "$$shell_path($$DLL_TARGET_DIR_RELEASE)" copy /Y "$$shell_path($$QTCSV_DLL_RELEASE)" "$$shell_path($$DLL_TARGET_DIR_RELEASE)\\" $$escape_expand(\n\t)
    }
}
