#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QSqlDatabase>
#include <QDateTime>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Reminder {
    int id;
    QString text;
    QDateTime dateTime;
    bool triggered = false;

    // Новые поля:
    int priority;    // 0 - Низкий, 1 - Средний, 2 - Высокий
    QString category; // "Работа", "Дом", "Здоровье" и т.д.
    int periodType;  // 0 - Однократно, 1 - Ежедневно, 2 - Еженедельно
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addReminder();
    void checkReminders();
    void deleteReminder(); // <- НОВЫЙ СЛОТ ДЛЯ КНОПКИ УДАЛЕНИЯ
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleWindow(); // Слот для переключения видимости окна
    void postponeReminder();
    void onRowChanged(int index);
    void clearTriggered(); // Очистить выполненные
    void filterList(const QString &text); // Поиск/фильтрация
    void toggleSilentMode(bool checked);  // Режим "Не беспокоить"

private:
    Ui::MainWindow *ui; // Указатель на интерфейс из Qt Designer
    QTimer *timer;
    QSystemTrayIcon *trayIcon;
    QSqlDatabase db;
    QVector<Reminder> reminders;

    QMenu *trayMenu;
    QAction *toggleAction;
    QAction *quitAction;

    bool isSilentMode = false; // Статус режима тишины
    QAction *silentAction;     // Пункт меню для трея

    void initDatabase();   // Создание файла БД и таблицы
    void loadReminders();   // Загрузка напоминаний из БД
    void updateListDisplay();
    void applyDarkTheme();
    void sendTelegramMessage(const QString &message); // Метод отправки в Telegram



protected:
    void closeEvent(QCloseEvent *event) override;
    //МЕТОД ДЛЯ ПЕРЕХВАТА СИСТЕМНЫХ ХОТКЕЕВ WINDOWS:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

};
#endif // MAINWINDOW_H
