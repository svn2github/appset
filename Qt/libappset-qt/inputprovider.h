#ifndef INPUTPROVIDER_H
#define INPUTPROVIDER_H

#include <QString>
#include <QRegExp>
#include <QList>
#include <QObject>

class InputProvider : public QObject
{
    Q_OBJECT

private:
    QList<QRegExp> quests;

    bool enabled;

public:
    explicit InputProvider(QObject *parent=0):QObject(parent),enabled(false){}

    bool evaluate(const QString &content);

    void setEnabled(bool enabled) { this->enabled=enabled; }
    bool isEnabled() const { return enabled; }

    void loadQuests(QString filepath);

signals:
    void userQuery(QString query);
};

#endif // INPUTPROVIDER_H
