#include "trayicon.h"

#include <QMenu>

TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent){

    QMenu *trayMenu = new QMenu();

    trayMenu->addAction("ProvaTray");

    setContextMenu(trayMenu);

    setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));


}
