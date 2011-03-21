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
    explicit Options(QWidget *parent = 0);
    ~Options();


    bool startfullscreen;
    int sbdelay;
    QString browser;

    void writeConfigFile(bool overwrite=true);

private:
    Ui::Options *ui;
};

#endif // OPTIONS_H
