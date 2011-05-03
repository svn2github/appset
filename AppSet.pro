TEMPLATE = subdirs
SUBDIRS = libappset appsetconf AppSetHelper Qt tools#tests
CONFIG += ordered
#tests.depends = libappset
Qt.depends = libappset AppSetHelper
