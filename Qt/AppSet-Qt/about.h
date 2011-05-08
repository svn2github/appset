#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QTimer>

namespace Ui {
    class About;
}

class About : public QDialog
{
    Q_OBJECT

public slots:
    void loader(bool ok);
    void reload();

public:
    explicit About(QWidget *parent = 0);
    ~About();

private:
    Ui::About *ui;

    QTimer *retry;
};

#endif // ABOUT_H
