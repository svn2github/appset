#include "options.h"
#include "ui_options.h"

#include <QFile>

#include <QString>
#include <QVector>

#define F_CONF_NAME "/etc/appset/appset-qt.conf"

Options::Options(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);

    writeConfigFile(false);
}

void Options::writeConfigFile(bool overwrite){
    if(overwrite && QFile::exists(F_CONF_NAME))
        QFile::remove(F_CONF_NAME);

    QFile conf(F_CONF_NAME);
    conf.open(QIODevice::ReadWrite | QIODevice::Text);

    int found=0;
    QVector<QString> configs;
    if(!overwrite)
        while(!conf.atEnd()){
            configs.append(conf.readLine());
            found++;
        }

    if(found>0)
        startfullscreen = configs[0].toShort();
    else{
        conf.write((QString::number((int)ui->checkBox->isChecked())+QString("\n")).toAscii());
        startfullscreen = ui->checkBox->isChecked();
    }
    if(found>1)
        sbdelay = configs[1].toInt();
    else{
        conf.write((QString::number(ui->spinBox->value())+QString("\n")).toAscii());
        sbdelay = ui->spinBox->value();
    }
    if(found>2)
        browser=configs[2].trimmed();
    else{
        QString userDefined = ui->userBrowserCmd->text();
        userDefined = (userDefined.split(' ').at(0)).trimmed();
        conf.write(((ui->defaultBrowser->isChecked()?QString(""):userDefined)+QString("\n")).toAscii());
        browser = ui->defaultBrowser->isChecked()?QString(""):userDefined;
    }

    ui->checkBox->setChecked(startfullscreen);
    ui->spinBox->setValue(sbdelay);
    ui->userBrowser->setChecked(browser!="");
    ui->userBrowserCmd->setText(browser);

    conf.close();
}

Options::~Options()
{
    delete ui;
}
