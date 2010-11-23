TEMPLATE = subdirs

OTHER_FILES += \
    appset.conf \
    appset/Arch/* \
    appset/Ubuntu/*

configuration.path = /etc
configuration.files += appset.conf
tools_configuration.path = /etc/appset
tools_configuration.files+= appset/*
INSTALLS += configuration \
    tools_configuration
