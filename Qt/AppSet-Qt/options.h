#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QString>

namespace Ui {
    class Options;
}

class Options : public QDialog
{
    Q_OBJECT

public:
    explicit Options(QWidget *parent = 0, QString path="/etc/appset/appset-qt.conf");
    ~Options();


    bool startfullscreen;
    int sbdelay;
    QString browser;
    bool backOutput;
    bool statShow;
    bool confirmCountdown;
    bool showRepos;
    bool enhanced;
    bool extraInfo;
    bool autoupgrade;

    int updelay;

    void writeConfigFile(bool overwrite=true, bool extraInfo=false);

private:
    Ui::Options *ui;

    QString path;
};

#endif // OPTIONS_H
