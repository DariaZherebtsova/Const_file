#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

//новое командное слово
// в битах адреса (5 бит) - адрес =5 (УШЛ в МКО1?)
// в битах подадреса (5 бит):
//- ст. бит = признак РК/КПИ)
//- мл 4 бита = адрес устройства УШЛ в МКО2
#define newCW(ADDR,DIR,SUBADDR,NWORDS) ((USHL_add<<11) | (DIR) | ((((SUBADDR-1)<<4) | ADDR)<<5) | (NWORDS & 0x1F))
#define RT_RECEIVE 0
#define KPI_MKO2sub 2
#define KPIdata_len 31
#define USHL_add 5

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

    QList<unsigned short> data_list;


    //извлекаем данные из файла
    while (!file.atEnd()) {
        line  = file.readLine();
        //qDebug()<<line;
        if (line.at(0) != '/')
        {
           // qDebug()<<"rabotaem";
            //удаляем пробелы
            line = line.remove(' ');
            //удаляем последний символ - перенос стоки
            line = line.remove(line.length()-1,1);
            QStringList list = line.split(',');
            //qDebug()<<"list.length() "<<list.length();
            //qDebug()<<list[0];
            qDebug()<<list[1];
            //qDebug()<<list[2];

            QString data_st;
            QString data_ml;
            unsigned short us_data_st;
            unsigned short us_data_ml;

            data_st = list[1].mid(2,2)+ list[1].mid(0,2);
            data_ml = list[1].mid(6,2)+ list[1].mid(4,2);
            //qDebug()<<"data_st "<<data_st;
            //qDebug()<<"data_ml "<<data_ml;

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

            data_list.append(us_data_st);
            data_list.append(us_data_ml);
            qDebug()<<"  ";

        }
        else {
            qDebug()<<"comment";
            qDebug()<<line;
       }

    }

    file.close();

    qDebug()<<data_list.size();


//----------------------------------------------
//Формируем команды КПИ
    QQueue <unsigned short> comKPIqueue;


    //--------Сброс ошибки (Reset_Error)-------------------------------------------------
    comKPIqueue.append(0x0F01); // Командное слово

    //--------Проверка общей контрольной суммы(Test_KS)--------------------        КС1
    comKPIqueue.append(0x0305); // Командное слово
    comKPIqueue.append(0);      // Младшее слово длины
    comKPIqueue.append(0);      // Старшее слово длины
    comKPIqueue.append(0);      // Младшее слово КС
    comKPIqueue.append(0);      // Старшее слово КС

    //-------Завершение интерпретации КПИ при ошибках (If_Error_Stop)------
    comKPIqueue.append(0x0D01); // Командное слово

    //-------Разрешение опасных команд КПИ (Test_Password)-----------------        КС2
    comKPIqueue.append(0xFE04); // Командное слово
    comKPIqueue.append(0);      // Первый и второй байты пароля
    comKPIqueue.append(0);      // Третий и четвертый байты пароля
    comKPIqueue.append(0);      // Пятый и шестой байты пароля

    //--------Запись байтов из команды КПИ в ОЗУ (Write_RAM)--------------------        КС1
    comKPIqueue.append(0x0A06); // Командное слово
    comKPIqueue.append(adr_ml);      // Младшее слово адреса участка ОЗУ
    comKPIqueue.append(adr_st);      // Старшее слово адреса участка ОЗУ
    comKPIqueue.append(data_list.size()*2);      // Число байтов, копируемых из КПИ в ОЗУ
    //добавляем данные
    for (int i = 0; i < data_list.size(); ++i) {
        comKPIqueue.append(data_list.at(i));
    }

    //--------Окончание последовательности команд KPI (End_of_KPI)---------
    comKPIqueue.append(0x0001); // Командное слово

    // Подсчет КС в команде Test_Password
    calculateTestPassword(&comKPIqueue);

    qDebug() <<"calculateTestPassword done";

    // Подсчет КС в команде Test_KS
    calculateTestKS(&comKPIqueue);

    qDebug() <<"calculateTestKS done";

    qDebug()<<"comKPIqueue.size() "<<comKPIqueue.size();

//------------------------------------------------------
//Делаем пакеты KPI

    // Адрес в буфере
    unsigned short len = comKPIqueue.size();
    unsigned int adrInBuf = len*2;
     qDebug()<<"adrInBuf ish"<<adrInBuf;

    unsigned short arr[33]; //буду использовать с 1 до 32
    //на всякий случай заполняем нулями
    for(ushort j = 0; j < 33; j++) arr[j] = 0;

    //добавляем в конец нули, чтобы резалось на куски по 28
    while(comKPIqueue.size() % 28) ComKPIqueue.append(0);

    //--------Код команды МКО--------------------------------------------
    uint adrModBWM = 2; //!!для рабочей платы (бывает ещё 3,4,5)
    arr[1] = newCW(adrModBWM, RT_RECEIVE, KPI_MKO2sub, KPIdata_len);

    //--------Адрес внутри буфера----------------------------------------
    adrInBuf -= (len % 28? len % 28 : 28)*2; //*2 - в байтах
    qDebug()<<"adrInBuf tec"<<adrInBuf;
    arr[2] = ((adrInBuf & 0xFFFFFFFE) | 1); //+1 в младш. разряде

    //--------число полуслов данных--------------------------------------
    arr[3] = ((len % 28? len % 28 : 28) << 8);
    len -= (len % 28? len % 28 : 28);

}

// Счет контрольной суммы для команды Test_Password
void MainWindow::calculateTestPassword(QQueue <unsigned short> *ptrComKPIqueue)
{
    unsigned short int word;

    for(int i = 0; i < ptrComKPIqueue->length();)
    {
        // Чтение слова из очереди
        word = ptrComKPIqueue->value(i);

        // Слово равно 0xFE04?
        if(word == 0xFE04)
        {
            // Формирование пароля и вписывание его в команду
            //makePassword(указатель на очередь с командами, номер в очереди)
            makePassword(ptrComKPIqueue, (i + 4));
            return;
        }

        if(word == 0 || word == 0x0001) return;
        // Определение длины команды
        unsigned int commandLength = word & 0x00FF;
        // Перевод указателя на следующую команду
        i += commandLength;
    }
}

// Генерирование пароля
void MainWindow::makePassword(QQueue<unsigned short> *ptrComKPIqueue, unsigned short int numberOfElement)
{
    uint i;
    union
    {
        unsigned char b[6];
        unsigned short int w[3];
    } data;

    unsigned short int len = (ptrComKPIqueue->length() - numberOfElement) * 2;

    //data.w[0] = ((unsigned int)ptrComKPIqueue & 0xFFFF); //FP_OFF(p);
    //data.w[1] = ((unsigned int)ptrComKPIqueue >> 16);    //FP_SEG(p);
    //data.w[2] = len;

    data.w[0] = random() % 32767; //random(32767)
    data.w[1] = len;
    data.w[2] = calculateKSdata(ptrComKPIqueue,numberOfElement);

    unsigned char BitPtr[48] =
    {
        0x15, 0x0F, 0x25, 0x21, 0x1B, 0x16, 0x09, 0x24,
        0x07, 0x2E, 0x26, 0x14, 0x28, 0x0D, 0x0A, 0x20,
        0x10, 0x01, 0x02, 0x1A, 0x13, 0x11, 0x06, 0x08,
        0x2D, 0x17, 0x2B, 0x0E, 0x00, 0x1E, 0x12, 0x19,
        0x05, 0x22, 0x18, 0x2F, 0x0C, 0x1D, 0x1F, 0x27,
        0x03, 0x04, 0x23, 0x2C, 0x1C, 0x2A, 0x29, 0x0B
    };
    unsigned char maska;
    union
    {
        unsigned char b[6];
        unsigned short int w[3];
    } password;

    for(i = 0; i < 6; i++) password.b[i] = 0;

    for(i = 0; i < 48; i++)
    {
        if(data.b[i >> 3] & (0x01 << (i & 0x07)))
        {
            maska = BitPtr[i];
            password.b[(maska >> 3)] |= (0x01 << (maska & 0x07));
        };
    };

    ptrComKPIqueue->replace((numberOfElement-1),password.w[0]);
    ptrComKPIqueue->replace((numberOfElement-2),password.w[1]);
    ptrComKPIqueue->replace((numberOfElement-3),password.w[2]);
}

void MainWindow::calculateTestKS(QQueue <unsigned short> *ptrComKPIqueue)
{
    unsigned short int word;
    unsigned long int KS;

    for(int i = 0; i < ptrComKPIqueue->length();)
    {
        // Чтение слова массива, на которое указывает указатель
        word = ptrComKPIqueue->value(i);
        // Слово равно 0x0305?
        if(word == 0x0305)
        {
            // Чтение длины массива из команды
            unsigned long int len = ptrComKPIqueue->value(i+1);

            // В команде КПИ задана нулевая длина.
            if(len == 0)
            {
                // Формирование длины защищаемого контрольной суммой массива слов
                len = (ptrComKPIqueue->length() - (i + 5)) * 2;

                // Запись вычисленной длины массива в команду.
                ptrComKPIqueue->replace((i+1),(len & 0xFFFF));
                ptrComKPIqueue->replace((i+2),((len >> 16)& 0xFFFF));
            };

            // Формирование контрольной суммы и вписывание ее в команду
            KS = 0;
            for(uint j = 0; j < len / 2; j++)
            {
                KS += (ptrComKPIqueue->value(i+5+j)) >> 8;
                KS += (ptrComKPIqueue->value(i+5+j)) & 0xFF;
            }
            ptrComKPIqueue->replace((i+3),(KS & 0xFFFF));
            ptrComKPIqueue->replace((i+4),((KS >> 16) & 0xFFFF));
            return;
        }
        if(word == 0 || word == 0x0001) return;

        // Определение длины текущей команды
        unsigned short int commandLength = word & 0x00FF;
        // Перевод указателя на следующую команду
        i += commandLength;
    }
}

unsigned short MainWindow::calculateKSdata(QQueue <unsigned short> *ptrComKPIqueue, unsigned short int numberOfElement)
{
    unsigned long int ks;
    // Обнуление счётчика контрольной суммы
    ks = 0;
    // Установка счётчика суммируемых слов
    // Уменьшение счетчика суммируемых слов
    for(int i = numberOfElement; i < ptrComKPIqueue->length(); i++)
    {
        // Циклический сдвиг контрольной суммы на 1 разряд влево
        ks = (ks + ks + ((ks & 0x00008000L) ? 1 : 0)) & 0xFFFF;
        // Прибавление к контрольной сумме очередного слова
        // Перевод указателя на следующее слово
        ks += ptrComKPIqueue->value(i);
    }
    // Выход
    return(ks);
}
