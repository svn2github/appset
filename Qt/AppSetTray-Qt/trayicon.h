/*
Copyright 2010 Simone Tobia

This file is part of AppSet.

AppSet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppSet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppSet; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

#include <QApplication>
#include <QStyle>
#include <QAction>

#include <asengine.h>

#include <QTimer>

#include <QProcess>

using namespace AS;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = 0);
    ~TrayIcon();

signals:

public slots:
    void launchAS();
    void activatedSlot(QSystemTrayIcon::ActivationReason ar);
    void checkUps();
    void checkRunning();
    void manualCheckUps();
    void manualCheckFinished(int s);

    void quitter();

private:
    Engine *as;

    QAction *launch;
    QAction *check;
    QAction *quit;

    QTimer *timer;
    QTimer *timer2;

    bool running;

    bool manualCheck;

    int visibility;
    int upgradables;

    QProcess *priv;
};

#endif // TRAYICON_H
