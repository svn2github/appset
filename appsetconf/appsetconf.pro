TEMPLATE = subdirs

OTHER_FILES += \
    appset.conf \
    appset/Arch/pacman \
    appset/Arch/packer \
    appset/Arch/pacman.quest

configuration.path = /etc
configuration.files += appset.conf
tools_configuration.path = /etc/appset
tools_configuration.files+= appset/*
INSTALLS += configuration \
    tools_configuration
