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

    if(!QFile::exists(F_CONF_NAME)){
        writeConfigFile();
    }

    loadConfigFile();
}

void Options::loadConfigFile(){
    QFile conf(F_CONF_NAME);
    conf.open(QIODevice::ReadOnly | QIODevice::Text);

    QVector<QString> configs;

    int i=0;
    while(!conf.atEnd()){
        configs.append(conf.readLine());
        ++i;
    }

    conf.close();

    if(i>0){
        startfullscreen = configs[0].toShort();
        ui->checkBox->setChecked(startfullscreen);
    }
    if(i>1){
        sbdelay = configs[1].toInt();
        ui->spinBox->setValue(sbdelay);
    }
}

void Options::writeConfigFile(){

    QFile newConf(F_CONF_NAME);
    newConf.open(QIODevice::WriteOnly | QIODevice::Text);

    newConf.write((QString::number((int)ui->checkBox->isChecked())+QString("\n")).toAscii());
    newConf.write((QString::number(ui->spinBox->value())+QString("\n")).toAscii());

    newConf.close();
}

Options::~Options()
{
    delete ui;
}
