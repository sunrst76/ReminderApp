#include "mainwindow.h"
#include "ui_mainwindow.h" // Файл генерируется автоматически из mainwindow.ui
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QStyle>
#include <QDebug>
#include <QIcon>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QStandardPaths>
#include <QStyleFactory>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Принудительно задаем стиль Fusion только для поля даты, чтобы календарь был красивым
    ui->dateEdit->setStyle(QStyleFactory::create("Fusion"));

    setWindowTitle("Планировщик напоминаний");
    setWindowIcon(QIcon(":/icon.png"));

    // При старте выставляем текущую дату в календарь
    ui->dateEdit->setDate(QDate::currentDate());

    // Выставляем текущие Часы и Минуты в наши новые спинбоксы (строки 57-59 на вашем фото)
    ui->hoursSpinBox->setValue(QTime::currentTime().hour());
    ui->minutesSpinBox->setValue(QTime::currentTime().minute());


    // Создаем контекстное меню для трея
    trayMenu = new QMenu(this);
    trayMenu->setStyleSheet(
        "QMenu {"
        "   background-color: #2b2b2b;" // Глубокий темно-серый фон
        "   color: #ffffff;"             // Белый текст
        "   border: 1px solid #555555;"
        "}"
        "QMenu::item {"
        "   padding: 6px 20px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #4a90e2;" // Синяя подсветка при наведении
        "   color: #ffffff;"
        "}"
        );

    // Создаем действие "Развернуть/Свернуть"
    toggleAction = new QAction("Развернуть", this);
    connect(toggleAction, &QAction::triggered, this, &MainWindow::toggleWindow);
    trayMenu->addAction(toggleAction);

    // Добавляем разделительную черту
    trayMenu->addSeparator();

    // Создаем действие "Режим тишины" с возможностью ставить галочку (checkable)
    silentAction = new QAction("Режим \"Не беспокоить\"", this);
    silentAction->setCheckable(true);
    connect(silentAction, &QAction::toggled, this, &MainWindow::toggleSilentMode);
    trayMenu->addAction(silentAction);

    trayMenu->addSeparator(); // Разделитель перед кнопкой "Выход"


    // Создаем действие "Выход"
    quitAction = new QAction("Выход", this);
    // Напрямую связываем встроенный слот Qt для закрытия всего приложения
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayMenu->addAction(quitAction);

    // Инициализируем иконку трея и передаем ей наше меню
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icon.png"));
    trayIcon->setContextMenu(trayMenu); // <- Связываем меню с треем
    trayIcon->setToolTip("Планировщик напоминаний\nЗадач в ожидании: 0");

    // Оставляем старое подключение для клика левой кнопкой мыши
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

    // Подключаем кнопку переноса времени
    connect(ui->postponeButton, &QPushButton::clicked, this, &MainWindow::postponeReminder);

    // Связываем клик по элементу списка с функцией автозаполнения полей
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &MainWindow::onRowChanged);

    // Подключение кнопки очистки выполненных
    connect(ui->clearTriggeredButton, &QPushButton::clicked, this, &MainWindow::clearTriggered);

    // Подключение живого поиска (срабатывает при каждом изменении текста)
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::filterList);

    // Кнопка +5 Минут
    connect(ui->btnPlus5Min, &QPushButton::clicked, this, [this]() {
        QDateTime dt = QDateTime::currentDateTime().addSecs(5 * 60);
        ui->dateEdit->setDate(dt.date());
        ui->hoursSpinBox->setValue(dt.time().hour());
        ui->minutesSpinBox->setValue(dt.time().minute());
    });

    // Кнопка +1 Час
    connect(ui->btnPlus1Hour, &QPushButton::clicked, this, [this]() {
        QDateTime dt = QDateTime::currentDateTime().addSecs(60 * 60);
        ui->dateEdit->setDate(dt.date());
        ui->hoursSpinBox->setValue(dt.time().hour());
        ui->minutesSpinBox->setValue(dt.time().minute());
    });


    // Кнопка +1 День (Завтра)
    connect(ui->btnPlus1Day, &QPushButton::clicked, this, [this]() {
        QDateTime dt = QDateTime::currentDateTime().addDays(1);
        ui->dateEdit->setDate(dt.date());

        // !!! ИСПРАВЛЯЕМ СТРОКУ 128 СЮДА !!!
        ui->hoursSpinBox->setValue(dt.time().hour());
        ui->minutesSpinBox->setValue(dt.time().minute());
    });


    trayIcon->show();
    applyDarkTheme();



#ifdef Q_OS_WIN
    // MOD_CONTROL (0x0002) + MOD_SHIFT (0x0004) = Ctrl + Shift
    // 0x4E — это шестнадцатеричный код клавиши 'N' в Windows
    // Идентификатор хоткея зададим как 100
    RegisterHotKey((HWND)this->winId(), 100, MOD_CONTROL | MOD_SHIFT, 0x4E);
#endif


// --- БЛОК НАСТРОЙКИ АВТОЗАПУСКА ДЛЯ WINDOWS ---
//#ifdef Q_OS_WIN
    // Путь к ветке автозапуска в реестре текущего пользователя
//    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    // Имя параметра в реестре (название вашей программы)
//    QString appName = QCoreApplication::applicationName();
//    if(appName.isEmpty()) appName = "ReminderApp";

    // Получаем полный путь к текущему запущенному .exe файлу
//    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    // Записываем путь в реестр Windows
//    settings.setValue(appName, "\"" + appPath + "\"");
//#endif
    // ----------------------------------------------

    // Настройка БД и загрузка старых напоминаний
    initDatabase();
    loadReminders();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkReminders);
    timer->start(1000);

    // Подключение кнопки (имя objectName в Qt Designer должно быть addButton)
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::addReminder);

    // Подключаем кнопку удаления (имя кнопки в Qt Designer должно быть deleteButton)
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::deleteReminder);

}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    // Освобождаем горячую клавишу при закрытии приложения
    UnregisterHotKey((HWND)this->winId(), 100);
#endif
    delete ui;
}

void MainWindow::initDatabase() {
    db = QSqlDatabase::addDatabase("QSQLITE");

    // НАДЕЖНЫЙ ПУТЬ: Создаем базу данных в папке AppData текущего пользователя
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Автоматически создаем папку для приложения, если её еще нет в системе
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Соединяем путь к папке с именем файла базы данных
    QString dbPath = dir.filePath("reminders.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, "Ошибка БД", db.lastError().text());
        return;
    }

    QSqlQuery query;
    // Новая расширенная таблица
    QString sql = "CREATE TABLE IF NOT EXISTS reminders ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "text TEXT, "
                  "datetime TEXT, "
                  "triggered INTEGER, "
                  "priority INTEGER, "
                  "category TEXT, "
                  "period_type INTEGER)";

    if (!query.exec(sql)) {
        qDebug() << "Ошибка создания таблицы:" << query.lastError().text();
    }
}


void MainWindow::loadReminders() {
    reminders.clear();
    ui->historyListWidget->clear(); // Очищаем старую историю на экране

    QSqlQuery query("SELECT id, text, datetime, triggered, priority, category, period_type FROM reminders");

    while (query.next()) {
        Reminder r;
        r.id = query.value(0).toInt();
        r.text = query.value(1).toString();
        r.dateTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        r.triggered = query.value(3).toInt() == 1;
        r.priority = query.value(4).toInt();
        r.category = query.value(5).toString();
        r.periodType = query.value(6).toInt();
        reminders.append(r);

        // Если задача из базы данных уже имеет статус "Сработала",
        // сразу отправляем её в окно истории
        if (r.triggered) {
            QString historyText = QString("[%1] %2")
            .arg(r.dateTime.toString("dd.MM HH:mm"))
                .arg(r.text);
            QListWidgetItem *histItem = new QListWidgetItem(historyText);
            histItem->setForeground(QColor(160, 160, 160));
            ui->historyListWidget->addItem(histItem);
        }
    }
    updateListDisplay();
}



void MainWindow::addReminder() {
    QString text = ui->lineEdit->text().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите текст напоминания!");
        return;
    }

    // Соединяем выбранную дату и время в один объект QDateTime
    QTime selectedTime(ui->hoursSpinBox->value(), ui->minutesSpinBox->value());
    QDateTime dt(ui->dateEdit->date(), selectedTime);


    int priority = ui->priorityComboBox->currentIndex();
    QString category = ui->categoryComboBox->currentText();
    int periodType = ui->periodComboBox->currentIndex();

    QSqlQuery query;
    query.prepare("INSERT INTO reminders (text, datetime, triggered, priority, category, period_type) "
                  "VALUES (:text, :dt, 0, :priority, :category, :period)");
    query.bindValue(":text", text);
    query.bindValue(":dt", dt.toString(Qt::ISODate));
    query.bindValue(":priority", priority);
    query.bindValue(":category", category);
    query.bindValue(":period", periodType);

    if (query.exec()) {
        Reminder r;
        r.id = query.lastInsertId().toInt();
        r.text = text;
        r.dateTime = dt;
        r.triggered = false;
        r.priority = priority;
        r.category = category;
        r.periodType = periodType;

        reminders.append(r);
        ui->lineEdit->clear();
        updateListDisplay();
    } else {
        QMessageBox::critical(this, "Ошибка сохранения", query.lastError().text());
    }
}


void MainWindow::checkReminders() {
    QDateTime now = QDateTime::currentDateTime();
    bool changed = false;

    for (int i = 0; i < reminders.size(); ++i) {
        if (!reminders[i].triggered && now >= reminders[i].dateTime) {

            // --- 1. УВЕДОМЛЕНИЕ ГОЛОСОМ + ЗВУК + СОХРАНЕНИЕ ПУША В ПАНЕЛИ WINDOWS ---
            // --- УВЕДОМЛЕНИЕ ГОЛОСОМ + ЗВУК + ЖЕЛЕЗОБЕТОННОЕ СОХРАНЕНИЕ В ЦЕНТРЕ УВЕДОМЛЕНИЙ ---
            if (!isSilentMode) {
                QString speechText = "Внимание! Напоминание: " + reminders[i].text;

                // Собираем продвинутый скрипт PowerShell, использующий современный Windows Toast Notification XML
                QString psScript = QString(
                                       "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; "
                                       "[System.Media.SystemSounds]::Asterisk.Play(); "

                                       // Загружаем необходимые системные типы Windows 11
                                       "[void][Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType=WindowsRuntime]; "
                                       "[void][Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom, ContentType=WindowsRuntime]; "

                                       // Формируем XML-структуру системной карточки уведомления
                                       "$xml = [Windows.Data.Xml.Dom.XmlDocument]::new(); "
                                       "$toastXml = '<toast duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>Напоминание!</text><text>%1</text></binding></visual></toast>'; "
                                       "$xml.LoadXml($toastXml); "

                                       // Создаем пуш-уведомление
                                       "$toast = [Windows.UI.Notifications.ToastNotification]::new($xml); "

                                       // Привязываем уведомление к легальному системному родителю (например, к Edge или PowerShell),
                                       // чтобы Windows 11 была обязана сохранить его в боковую панель истории!
                                       "[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('Microsoft.WindowsTerminal_8wekyb3d8bbwe!App').Show($toast); "

                                       "Start-Sleep -Seconds 1; "
                                       "Add-Type -AssemblyName System.Speech; "
                                       "$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
                                       "$synth.Speak(\"%2\")"
                                       ).arg(reminders[i].text).arg(speechText);

                QStringList arguments;
                arguments << "-NoProfile" << "-NonInteractive" << "-Command" << psScript;
                QProcess::startDetached("powershell.exe", arguments);
            }
            // --- 2. ОТПРАВКА В ТЕЛЕГРАМ ---
            sendTelegramMessage("Напоминание: " + reminders[i].text);

            trayIcon->showMessage("Напоминание!", reminders[i].text, QSystemTrayIcon::Information, 10000);

            // --- НОВЫЙ БЛОК: ДУБЛИРОВАНИЕ В ГЛАВНОМ ОКНЕ Приложения ---
            // 1. Формируем красивую строку для списка истории
            QString historyText = QString("[%1] %2")
                                      .arg(reminders[i].dateTime.toString("HH:mm"))
                                      .arg(reminders[i].text);

            QListWidgetItem *historyItem = new QListWidgetItem(historyText);
            historyItem->setForeground(QColor(160, 160, 160)); // Сделаем шрифт в истории аккуратным серым

            // 2. Добавляем элемент в НОВЫЙ listWidget истории
            ui->historyListWidget->addItem(historyItem);

            // 3. ПРИНУДИТЕЛЬНО РАЗВОРАЧИВАЕМ ОКНО ПОВЕРХ ВСЕХ ПРОГРАММ
            this->show();           // Показываем окно, если оно было скрыто в трее
            this->showNormal();     // Восстанавливаем из свернутого состояния
            this->raise();          // Поднимаем над другими окнами Windows
            this->activateWindow(); // Переводим фокус на нашу программу

            // 4. Автоматически переключаем Tab Widget на вторую вкладку (История)
            // Индекс 0 — это первая вкладка, индекс 1 — это вторая вкладка истории
            ui->tabWidget->setCurrentIndex(1);
            // ---------------------------------------------------------

            // --- 3. ОБРАБОТКА ПЕРИОДИЧНОСТИ ЗАДАЧ В БД ---
            QSqlQuery query;
            if (reminders[i].periodType == 0) { // Однократно
                reminders[i].triggered = true;
                query.prepare("UPDATE reminders SET triggered = 1 WHERE id = :id");
                query.bindValue(":id", reminders[i].id);
                query.exec();
            }
            else if (reminders[i].periodType == 1) { // Ежедневно
                reminders[i].dateTime = reminders[i].dateTime.addDays(1);
                query.prepare("UPDATE reminders SET datetime = :dt WHERE id = :id");
                query.bindValue(":dt", reminders[i].dateTime.toString(Qt::ISODate));
                query.bindValue(":id", reminders[i].id);
                query.exec();
            }
            else if (reminders[i].periodType == 2) { // Каждую неделю
                reminders[i].dateTime = reminders[i].dateTime.addDays(7);
                query.prepare("UPDATE reminders SET datetime = :dt WHERE id = :id");
                query.bindValue(":dt", reminders[i].dateTime.toString(Qt::ISODate));
                query.bindValue(":id", reminders[i].id);
                query.exec();
            }

            changed = true;
        }
    }

    if (changed) {
        updateListDisplay();
    }
}


void MainWindow::updateListDisplay() {
    ui->listWidget->clear();

    // Переменная для подсчета невыполненных задач
    int activeCount = 0;

    // Сортируем вектор: сначала идут невыполненные задачи, более близкие по времени
    std::sort(reminders.begin(), reminders.end(), [](const Reminder &a, const Reminder &b) {
        if (a.triggered != b.triggered) return !a.triggered;
        return a.dateTime < b.dateTime;
    });

    for (const auto &r : reminders) {
        QString status = r.triggered ? "[Сработало] " : "[Ждет] ";
        QString pText = (r.priority == 2) ? "!!! " : (r.priority == 1 ? "! " : "");

        QString itemText = QString("%1 %2[%3] %4 - %5")
                               .arg(status)
                               .arg(pText)
                               .arg(r.category)
                               .arg(r.dateTime.toString("dd.MM.yyyy HH:mm"))
                               .arg(r.text);

        QListWidgetItem *item = new QListWidgetItem(itemText);

        // Адаптивное цветовое кодирование под темную тему
        if (r.triggered) {
            item->setForeground(QColor(128, 128, 128)); // Выполненные — приглушенно-серые
        } else {
            if (r.priority == 2) {
                item->setBackground(QColor(120, 40, 40));   // Высокий — темно-красный пастельный
                item->setForeground(Qt::white);
            } else if (r.priority == 1) {
                item->setBackground(QColor(110, 90, 40));   // Средний — темно-желтый/горчичный
                item->setForeground(Qt::white);
            }
        }

        if (!r.triggered) {
            activeCount++; // Считаем только те задачи, которые еще ждут своего времени
        }

        ui->listWidget->addItem(item);
    }
    // --- ОБНОВЛЯЕМ ПОДСКАЗКУ В ТРЕЕ ДИНАМИЧЕСКИ ---
    QString tooltipText = QString("Планировщик напоминаний\nАктивных задач: %1").arg(activeCount);
    trayIcon->setToolTip(tooltipText);
    // ----------------------------------------------
}


void MainWindow::deleteReminder() {
    // Получаем индекс выделенной строки в списке
    int currentRow = ui->listWidget->currentRow();

    // Если ничего не выделено, currentRow будет равен -1
    if (currentRow < 0) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, выберите напоминание из списка для удаления!");
        return;
    }

    // Находим ID напоминания в нашем векторе данных
    int reminderId = reminders[currentRow].id;

    // Удаляем запись из базы данных SQLite
    QSqlQuery query;
    query.prepare("DELETE FROM reminders WHERE id = :id");
    query.bindValue(":id", reminderId);

    if (query.exec()) {
        // Если из БД удалено успешно, удаляем из оперативной памяти (вектора)
        reminders.removeAt(currentRow);

        // Обновляем визуальный список на экране
        updateListDisplay();
    } else {
        QMessageBox::critical(this, "Ошибка удаления", query.lastError().text());
    }
}

// Вызывается автоматически, когда пользователь кликает по иконке в трее
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        toggleWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();

        // Меняем текст в контекстном меню на "Развернуть", так как окно скрылось
        toggleAction->setText("Развернуть");

        trayIcon->showMessage("Планировщик", "Приложение свернуто в трей.", QSystemTrayIcon::Information, 2000);
    }
}


void MainWindow::toggleWindow() {
    if (isVisible()) {
        hide();
        toggleAction->setText("Развернуть"); // <- Добавить
    } else {
        show();
        raise();
        activateWindow();
        toggleAction->setText("Свернуть");   // <- Добавить
    }
}

void MainWindow::postponeReminder() {
    // 1. Проверяем, выбрано ли напоминание в списке
    int currentRow = ui->listWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, выберите напоминание из списка для изменения!");
        return;
    }

    // 2. Считываем все измененные параметры из виджетов интерфейса
    QString newText = ui->lineEdit->text().trimmed();
    if (newText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Текст напоминания не может быть пустым!");
        return;
    }

    // Соединяем новые измененные дату и время
    QTime selectedTime(ui->hoursSpinBox->value(), ui->minutesSpinBox->value());
    QDateTime newDateTime(ui->dateEdit->date(), selectedTime);


    int newPriority = ui->priorityComboBox->currentIndex();
    QString newCategory = ui->categoryComboBox->currentText();
    int newPeriodType = ui->periodComboBox->currentIndex();

    // Получаем ID текущего изменяемого напоминания
    int reminderId = reminders[currentRow].id;

    // 3. Обновляем ВСЕ параметры в базе данных SQLite
    QSqlQuery query;
    // Сбрасываем статус triggered на 0, чтобы отредактированная задача снова могла сработать
    query.prepare("UPDATE reminders SET text = :text, datetime = :dt, triggered = 0, "
                  "priority = :priority, category = :category, period_type = :period WHERE id = :id");
    query.bindValue(":text", newText);
    query.bindValue(":dt", newDateTime.toString(Qt::ISODate));
    query.bindValue(":priority", newPriority);
    query.bindValue(":category", newCategory);
    query.bindValue(":period", newPeriodType);
    query.bindValue(":id", reminderId);

    if (query.exec()) {
        // 4. Обновляем данные в оперативной памяти (векторе)
        reminders[currentRow].text = newText;
        reminders[currentRow].dateTime = newDateTime;
        reminders[currentRow].priority = newPriority;
        reminders[currentRow].category = newCategory;
        reminders[currentRow].periodType = newPeriodType;
        reminders[currentRow].triggered = false; // Возвращаем в режим ожидания

        // 5. Перерисовываем список на экране с учетом новых цветов и сортировки
        updateListDisplay();

        QMessageBox::information(this, "Успех", "Параметры напоминания успешно изменены!");
    } else {
        QMessageBox::critical(this, "Ошибка БД", query.lastError().text());
    }
}


void MainWindow::toggleSilentMode(bool checked) {
    isSilentMode = checked;
    if (isSilentMode) {
        trayIcon->showMessage("Планировщик", "Режим тишины включен. Уведомления придут без звука и окон.", QSystemTrayIcon::Information, 2000);
    }
}

// 1. Очистить все выполненные напоминания одной кнопкой
void MainWindow::clearTriggered() {
    QSqlQuery query;
    if (query.exec("DELETE FROM reminders WHERE triggered = 1")) {
        // Удаляем из вектора в памяти
        for (int i = reminders.size() - 1; i >= 0; --i) {
            if (reminders[i].triggered) {
                reminders.removeAt(i); // Зависит от версии Qt: reminders.removeAt(i);
            }
        }
        // Перезагружаем из БД для надежности
        loadReminders();
    }
}

// 2. Фильтрация списка (Живой поиск)
void MainWindow::filterList(const QString &text) {
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        // Если текст строки содержит поисковый запрос (без учета регистра) — показываем, иначе скрываем
        bool matches = item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!matches);
    }
}

// 3. Автозаполнение полей при клике (обновим, чтобы подхватывать комбобоксы)
void MainWindow::onRowChanged(int index) {
    if (index < 0 || index >= reminders.size()) return;

    // Блокируем сигналы, чтобы изменение текста не вызывало ложных срабатываний
    ui->lineEdit->blockSignals(true);
    ui->dateEdit->blockSignals(true);
    ui->hoursSpinBox->blockSignals(true);   // Блокируем новые спинбоксы
    ui->minutesSpinBox->blockSignals(true);

    // Заполняем поля данными из выбранного напоминания
    ui->lineEdit->setText(reminders[index].text);
    ui->dateEdit->setDate(reminders[index].dateTime.date());
    ui->hoursSpinBox->setValue(reminders[index].dateTime.time().hour());
    ui->minutesSpinBox->setValue(reminders[index].dateTime.time().minute());

    ui->priorityComboBox->setCurrentIndex(reminders[index].priority);

    int catIdx = ui->categoryComboBox->findText(reminders[index].category);
    if (catIdx >= 0) ui->categoryComboBox->setCurrentIndex(catIdx);

    ui->periodComboBox->setCurrentIndex(reminders[index].periodType);

    // Разблокируем сигналы обратно
    ui->lineEdit->blockSignals(false);
    ui->dateEdit->blockSignals(false);
    ui->hoursSpinBox->blockSignals(false);
    ui->minutesSpinBox->blockSignals(false);
}


bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    Q_UNUSED(eventType);
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    // Переводим указатель на сообщение в структуру Windows MSG
    MSG *msg = static_cast<MSG*>(message);

    // WM_HOTKEY — это системное сообщение Windows о нажатии глобального хоткея
    if (msg->message == WM_HOTKEY) {
        // Проверяем, что сработал именно наш хоткей с ID = 100
        if (msg->wParam == 100) {

            // Если окно скрыто в трее — показываем его и активируем
            if (!isVisible()) {
                show();
                raise();
                activateWindow();

                // Опционально: автоматически переводим фокус клавиатуры в поле ввода текста
                ui->lineEdit->setFocus();
            } else {
                // Если окно уже было открыто на экране — хоткей его спрячет обратно в трей
                hide();
                toggleAction->setText("Развернуть");
            }
            return true; // Возвращаем true, показывая Qt, что сообщение успешно обработано
        }
    }
#endif

    return false; // Для всех остальных системных сообщений передаем управление стандартным механизмам Qt
}

void MainWindow::applyDarkTheme() {
    QString darkStyle =
        "QMainWindow { background-color: #2b2b2b; }"
        "QWidget { color: #ffffff; font-family: 'Segoe UI', sans-serif; font-size: 13px; }"

        // Общий стиль полей ввода. Спинбоксы теперь наследуют нативные стрелочки!
        "QLineEdit, QDateEdit, QSpinBox, QComboBox, QListWidget {"
        "   background-color: #3c3f41;"
        "   border: 1px solid #555555;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "   color: #ffffff;"
        "}"

        // --- ДОБАВЬТЕ ЭТОТ БЛОК: ОТОДВИГАЕМ ТЕКСТ ОТ СТРЕЛОЧЕК ДЛЯ 100% КЛИКАБЕЛЬНОСТИ ---
        "QSpinBox, QDateEdit {"
        "   padding-right: 20px;" // Текст больше никогда не перекроет верхнюю кнопку!
        "}"
        // --------------------------------------------------------------------------------

        "QSplitter::handle { background-color: #2b2b2b; border: none; }"

        // Стили выпадающих списков (ComboBox)
        "QComboBox { padding-right: 20px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 20px; border: none; background-color: #4e5254; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QComboBox::arrow { image: none; border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid #ffffff; }"

        // --- РАБОЧИЙ БЛОК: УБИРАЕМ CSS-КОНФЛИКТЫ ДЛЯ СТРЕЛОЧЕК QSpinBox ---
        // Мы удалили ручную прорисовку up-button и down-arrow.
        // Теперь движок Fusion сам выведет аккуратные стандартные стрелочки, которые будут отлично нажиматься.
        // ---------------------------------------------------------------------

        // Стили вкладок (Tab Widget) без белых полос
        "QTabWidget::pane { border: 1px solid #555555; background-color: #2b2b2b; top: -1px; }"
        "QTabBar::tab { background-color: #3c3f41; border: 1px solid #555555; border-bottom-color: transparent; border-top-left-radius: 4px; border-top-right-radius: 4px; padding: 6px 12px; color: #aaaaaa; }"
        "QTabBar::tab:selected { background-color: #2b2b2b; color: #ffffff; border-bottom-color: #2b2b2b; }"

        "QLineEdit:focus, QDateEdit:focus, QSpinBox:focus, QComboBox:focus, QListWidget:focus { border: 1px solid #4a90e2; }"
        "QPushButton { background-color: #4e5254; border: 1px solid #5e6264; border-radius: 4px; padding: 6px 12px; color: #ffffff; font-weight: bold; }"
        "QPushButton:hover { background-color: #5c6062; }"
        "QPushButton:pressed { background-color: #3c3f41; }"
        "QCalendarWidget QWidget { background-color: #3c3f41; color: #ffffff; }"
        "QCalendarWidget QAbstractItemView:enabled { background-color: #3c3f41; color: #ffffff; selection-background-color: #4a90e2; selection-color: #ffffff; }"
        "QMessageBox { background-color: #2b2b2b; }"
        "QMessageBox QLabel { color: #ffffff; }"
        "QMessageBox QPushButton { background-color: #4e5254; color: #ffffff; }";

    this->setStyleSheet(darkStyle);
}




void MainWindow::sendTelegramMessage(const QString &message) {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    // Безопасная сборка адреса API без опечаток
    QString basePart = "https://api.te" "legram.org/";
    QString botPart  = "bot8779603023:AAFyC3ZxAj5c_dLQluRbJN27Hwy8k1wUNJY/";
    QString methodPart = "sendMessage";

    QString urlStr = basePart + botPart + methodPart;
    QUrl url(urlStr);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    // Ваш проверенный Chat ID
    params.addQueryItem("chat_id", "330151324");
    params.addQueryItem("text", message);

    QNetworkReply *reply = manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, manager, &QNetworkAccessManager::deleteLater);
}
