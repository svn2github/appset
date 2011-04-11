#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    ui->webView->setUrl(QUrl("http://appset.altervista.org/index.htm"));
}

About::~About()
{
    delete ui;
}
