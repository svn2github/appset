#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

#include <QApplication>
#include <QStyle>

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = 0);

signals:

public slots:

};

#endif // TRAYICON_H
