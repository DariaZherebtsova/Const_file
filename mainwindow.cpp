#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>


bool ok;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    //QString filename = QFileDialog::getOpenFileName();
    QString filename = "/home/dash/const_K_VU.txt";
    QFile file;
    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Не могу прочитать файл с константами"));
        return;
    };

    //читаем первую строку
    QString line = file.readLine();
    qDebug()<<line;

    //читаем строку c адресом
    line = file.readLine();
    qDebug()<<line;
    //удаляем последний символ - перенос стоки
    line = line.remove(line.length()-1,1);
    QStringList list = line.split(' ');
    qDebug()<<list[0];
    qDebug()<<list[1];
    unsigned short adr_st;
    unsigned short adr_ml;
    adr_st = list[0].toUShort(&ok, 16);
    if (!ok) {
        QMessageBox::warning(this, tr("Error"), tr("Некорректное старшее полуслово адреса"));
        return;
    }
    adr_ml = list[1].toUShort(&ok, 16);
    if (!ok) {
        QMessageBox::warning(this, tr("Error"), tr("Некорректное младшее полуслово адреса"));
        return;
    }
    qDebug("adr_str 0x%x adr_ml 0x%x", adr_st, adr_ml);


    //извлекаем данные из файла
 //   while (!file.atEnd()) {
        line  = file.readLine();
        qDebug()<<line;
        if (line.at(0) != '/')
        {
            qDebug()<<"rabotaem";
            //удаляем пробелы
            line = line.remove(' ');
            //удаляем последний символ - перенос стоки
            line = line.remove(line.length()-1,1);
            QStringList list = line.split(',');
            qDebug()<<"list.length() "<<list.length();
            qDebug()<<list[0];
            qDebug()<<list[1];
            qDebug()<<list[2];

            QString data_st;
            QString data_ml;
            unsigned short us_data_st;
            unsigned short us_data_ml;

            data_st = list[1].mid(2,2)+ list[1].mid(0,2);
            data_ml = list[1].mid(6,2)+ list[1].mid(4,2);
            qDebug()<<"data_st "<<data_st;
            qDebug()<<"data_ml "<<data_ml;

            us_data_st = data_st.toUShort(&ok, 16);
            if (!ok) {
                ui->textEdit->append(tr("Некорректное значение старшего полуслова"));
                return;
            }
            us_data_ml = data_ml.toUShort(&ok, 16);
            if (!ok) {
                ui->textEdit->append(tr("Некорректное значениe старшего полуслова"));
                return;
            }
            qDebug("us_data_st 0x%x us_data_ml 0x%x", us_data_st, us_data_ml);


          /*  Data_const dc;
            QString s_addr = list[1].mid(0,4);
            qDebug()<<"s_addr_st "<<s_addr;
            dc.addr_st = s_addr.toUShort(&ok, 16);
            if (!ok) {
                ui->textEdit->append(tr("Некорректное старшее полуслово адреса"));
                return;
            }
            s_addr = list[1].mid(4,4);
            qDebug()<<"s_addr_ml "<<s_addr;
            dc.addr_ml = s_addr.toUShort(&ok, 16);
            if (!ok) {
                ui->textEdit->append(tr("Некорректное младшее полуслово адреса"));
                return;
            }
            */
        }
        else qDebug()<<"comment";

  //  }


    //val = file.readAll();
    file.close();
}
