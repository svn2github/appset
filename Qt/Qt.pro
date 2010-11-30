TEMPLATE = subdirs
SUBDIRS = libappset-qt AppSet-Qt AppSetTray-Qt
CONFIG += ordered
AppSet-Qt.deptends = libappset-qt
AppSetTray-Qt.deptends = libappset-qt
