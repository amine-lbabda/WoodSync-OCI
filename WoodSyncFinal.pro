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

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += opencv4

DISTFILES += \
    .env \
    .env.example \
    README.md \
    face_detection_yunet_2023mar.onnx \
    face_model.xml \
    face_recognition_sface_2021dec.onnx

unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lqtcsv

INCLUDEPATH += $$PWD/../../../../usr/local/include/qtcsv
DEPENDPATH += $$PWD/../../../../usr/local/include/qtcsv

unix:!macx: LIBS += -L$$PWD/../../Qt/6.7.3/gcc_64/lib/ -lqt6keychain

INCLUDEPATH += $$PWD/../../Qt/6.7.3/gcc_64/include/qt6keychain
DEPENDPATH += $$PWD/../../Qt/6.7.3/gcc_64/include/qt6keychain
