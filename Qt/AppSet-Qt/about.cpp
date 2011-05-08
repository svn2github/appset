#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    retry=new QTimer(this);
    retry->setSingleShot(true);

    reload();

    connect(ui->webView,SIGNAL(loadFinished(bool)),SLOT(loader(bool)));
    connect(retry,SIGNAL(timeout()),this,SLOT(reload()));
}

void About::reload(){
    ui->webView->setUrl(QUrl("http://appset.altervista.org/index.htm"));
}

void About::loader(bool ok){
    if(ok){
        disconnect(ui->webView,SIGNAL(loadFinished(bool)),this,SLOT(loader(bool)));
    }else{
        retry->start(5000);
    }
}

About::~About(){
    delete ui;
}
