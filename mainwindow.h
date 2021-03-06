#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void calculateTestPassword(QQueue <unsigned short> *ptrComKPIqueue);
    void makePassword(QQueue <unsigned short> *ptrComKPIqueue, unsigned short int numberOfElement);
    void calculateTestKS(QQueue <unsigned short> *ptrComKPIqueue);
    unsigned short calculateKSdata(QQueue <unsigned short> *ptrComKPIqueue, unsigned short int numberOfElement);
    struct Data_const {
        QString name;
        unsigned short addr_st;
        unsigned short addr_ml;
        unsigned short size;
        QList<unsigned short> data;
    };

    QList<Data_const> list_data_const;

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
