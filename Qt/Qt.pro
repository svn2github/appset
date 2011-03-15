TEMPLATE = subdirs
SUBDIRS = libappset-qt AppSet-Qt AppSetTray-Qt
CONFIG += ordered
AppSet-Qt.depends = libappset-qt
AppSetTray-Qt.depends = libappset-qt
