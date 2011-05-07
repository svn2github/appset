#include "options.h"
#include "ui_options.h"

#include <QFile>

#include <QString>
#include <QVector>

Options::Options(QWidget *parent,QString path) :
    QDialog(parent),
    ui(new Ui::Options)
{
    this->path=path;

    ui->setupUi(this);

    ui->showRepos->setDisabled(ui->enhancedGraph->isChecked());

    extraInfo=true;

    writeConfigFile(false);
}

void Options::writeConfigFile(bool overwrite, bool ei){
    if(overwrite && QFile::exists(path))
        QFile::remove(path);

    QFile conf(path);
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
        //userDefined = (userDefined.split(' ').at(0)).trimmed();
        conf.write(((ui->defaultBrowser->isChecked()?QString(""):userDefined)+QString("\n")).toAscii());
        browser = ui->defaultBrowser->isChecked()?QString(""):userDefined;
    }
    if(found>3)
        updelay=configs[3].toInt();
    else{
        conf.write((QString::number(ui->spinBox_2->value())+QString("\n")).toAscii());
        updelay = ui->spinBox_2->value();
    }
    if(found>4)
        backOutput=configs[4].toShort();
    else{
        conf.write((QString::number((int)ui->backOutput->isChecked())+QString("\n")).toAscii());
        backOutput = ui->backOutput->isChecked();
    }
    if(found>5)
        statShow=configs[5].toShort();
    else{
        conf.write((QString::number((int)ui->statShow->isChecked())+QString("\n")).toAscii());
        statShow = ui->statShow->isChecked();
    }if(found>6)
        confirmCountdown=configs[6].toShort();
    else{
        conf.write((QString::number((int)ui->confirmCountdown->isChecked())+QString("\n")).toAscii());
        confirmCountdown=ui->confirmCountdown->isChecked();
    }
    if(found>7)
        showRepos=configs[7].toShort();
    else{
        conf.write((QString::number((int)ui->showRepos->isChecked())+QString("\n")).toAscii());
        showRepos=ui->showRepos->isChecked();
    }
    if(found>8)
        enhanced=configs[8].toShort();
    else{
        conf.write((QString::number((int)ui->enhancedGraph->isChecked())+QString("\n")).toAscii());
        enhanced=ui->enhancedGraph->isChecked();
    }
    if(found>9 && !ei)
        extraInfo=configs[9].toShort();
    else{
        conf.write((QString::number((int)extraInfo)+QString("\n")).toAscii());
    }
    if(found>10)
        autoupgrade=configs[10].toShort();
    else{
        conf.write((QString::number((int)ui->autoupgrade->isChecked())+QString("\n")).toAscii());
        autoupgrade=ui->autoupgrade->isChecked();
    }


    ui->checkBox->setChecked(startfullscreen);
    ui->spinBox->setValue(sbdelay);
    ui->userBrowser->setChecked(browser!="");
    ui->userBrowserCmd->setText(browser);
    ui->spinBox_2->setValue(updelay);
    ui->backOutput->setChecked(backOutput);
    ui->statShow->setChecked(statShow);
    ui->confirmCountdown->setChecked(confirmCountdown);
    ui->showRepos->setChecked(showRepos);
    ui->enhancedGraph->setChecked(enhanced);
    ui->standardGraph->setChecked(!enhanced);
    ui->autoupgrade->setChecked(autoupgrade);

    conf.close();
}

Options::~Options()
{
    delete ui;
}
