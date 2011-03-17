#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

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


    void loadConfigFile();
    void writeConfigFile();

private:
    Ui::Options *ui;
};

#endif // OPTIONS_H