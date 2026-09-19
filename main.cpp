#include "mainwindow.h"

#include <QApplication>
//#include <QDir> // 1. Подключаем библиотеку для работы с QDir

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 2. Переносим вызов внутрь функции main ПОСЛЕ создания QApplication
    // QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow w;
    //w.show(); // ваша закомментированная строка
    return QApplication::exec();
}
