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
    bool forced;

public:
    explicit InputProvider(QObject *parent=0):QObject(parent),enabled(false),forced(false){}

    bool evaluate(const QString &content);

    void setEnabled(bool enabled) { this->enabled=enabled; }
    bool isEnabled() const { return enabled; }

    void setForced(bool forced) { this->forced=forced; }
    bool isForced() const { return forced; }

    void loadQuests(QString filepath);

signals:
    void userQuery(QString query);
};

#endif // INPUTPROVIDER_H
