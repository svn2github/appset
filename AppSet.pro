TEMPLATE = subdirs
SUBDIRS = libappset appsetconf AppSet-Qt #AppSetHelper AppSetTray-Qt#tests
CONFIG += ordered
#tests.depends = libappset
AppSet-Qt.depends = libappset
#AppSetTray-Qt.depends = AppSetHelper
