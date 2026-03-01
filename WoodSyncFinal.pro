QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    connection.cpp \
    employes.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    connection.h \
    dotenv.h \
    employes.h \
    mainwindow.h

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
