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

    QStringList answers;
    answers << tr("Automatic") << tr("Semi-Automatic") << tr("Ask everything");
    ui->labelInter->setText(ui->labelInter->text()+QString(" (")+tr("in English")+")");
    ui->inter->addItems(answers);
    ui->inter->setCurrentIndex(1);

    extraInfo=true;

    writeConfigFile(false);

    ui->userBrowser->setIcon(QIcon::fromTheme(ui->userBrowserCmd->text().split(' ').at(0)));
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
    if(found>11)
        preload=configs[11].toShort();
    else{
        conf.write((QString::number((int)ui->preload->isChecked())+QString("\n")).toAscii());
        preload=ui->preload->isChecked();
    }
    if(found>12)
        interactions=configs[12].toInt();
    else{
        conf.write((QString::number((int)ui->inter->currentIndex())+QString("\n")).toAscii());
        interactions=ui->inter->currentIndex();
    }
    if(found>13)
        trayVisibility=configs[13].toInt();
    else{
        conf.write((QString::number((int)ui->trayVisibility->currentIndex())+QString("\n")).toAscii());
        trayVisibility=ui->trayVisibility->currentIndex();
    }
    if(found>14)
        rssShow=configs[14].toShort();
    else{
        conf.write((QString::number((int)ui->rssShow->isChecked())+QString("\n")).toAscii());
        rssShow=ui->rssShow->isChecked();
    }
    if(found>15)
        loadHomes=configs[15].toShort();
    else{
        conf.write((QString::number((int)ui->loadHomes->isChecked())+QString("\n")).toAscii());
        loadHomes=ui->loadHomes->isChecked();
    }
    if(found>16)
        firstPage=configs[16].toInt();
    else{
        conf.write((QString::number((int)ui->firstPageCombo->currentIndex())+QString("\n")).toAscii());
        firstPage=ui->firstPageCombo->currentIndex();
    }
    if(found>17)
        xTermCmd=configs[17].trimmed();
    else{
        QString userDefined = ui->xTermCmd->text().trimmed();
        conf.write((((userDefined=="")?QString("xterm -e"):userDefined)+QString("\n")).toAscii());
        xTermCmd = ((userDefined=="")?QString("xterm -e"):userDefined);
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
    ui->preload->setChecked(preload);
    ui->inter->setCurrentIndex(interactions);
    ui->trayVisibility->setCurrentIndex(trayVisibility);
    ui->rssShow->setChecked(rssShow);
    ui->loadHomes->setChecked(loadHomes);
    ui->firstPageCombo->setCurrentIndex(firstPage);
    ui->xTermCmd->setText(xTermCmd);

    conf.close();
}

Options::~Options()
{
    delete ui;
}
