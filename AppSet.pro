TEMPLATE = subdirs
SUBDIRS = libappset appsetconf AppSetHelper Qt#tests
CONFIG += ordered
#tests.depends = libappset
Qt.depends = libappset AppSetHelper
