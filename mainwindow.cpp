#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "about.h"
#include "extract_worker.h"
#include "rebuild_worker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");

    QString bankDir = settings.value("BankDir").toString();

    if (bankDir.isEmpty())
    {
        bankDir = QCoreApplication::applicationDirPath() + "/bank";
        settings.setValue("BankDir", bankDir);
    }

    ui->bankTextBox->setText(bankDir);

    QString wavDir = settings.value("WavDir").toString();

    if (wavDir.isEmpty())
    {
        wavDir = QCoreApplication::applicationDirPath() + "/wav";
        settings.setValue("WavDir", wavDir);
    }

    ui->wavTextBox->setText(wavDir);

    QString rebuildDir = settings.value("RebuildDir").toString();

    if (rebuildDir.isEmpty())
    {
        rebuildDir = QCoreApplication::applicationDirPath() + "/build";
        settings.setValue("RebuildDir", rebuildDir);
    }

    ui->rebuildTextBox->setText(rebuildDir);
    settings.endGroup();
    settings.beginGroup("Options");

    QString format = settings.value("Format").toString();
    QString quality = settings.value("Quality").toString();
    settings.endGroup();

    if (!format.isEmpty())
    {
        if (format == "vorbis")
            ui->format_comboBox->setCurrentIndex(0);
        else
            ui->format_comboBox->setCurrentIndex(1);
    }

    if (!quality.isEmpty())
        ui->quality_spinBox->setValue(quality.toInt() ? quality.toInt() : 95);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_bankFolderButton_clicked()
{
    //qDebug() << "Bank Button clicked!";
    QString dir = QCoreApplication::applicationDirPath() + "/bank";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Bank Folder", dir, options);

    if(folder.isEmpty())
        ui->bankTextBox->setText(dir);
    else
        ui->bankTextBox->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("BankDir", ui->bankTextBox->text());
    settings.endGroup();
}

void MainWindow::on_wavFolderButton_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/wav";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Wav Folder", dir, options);

    if(folder.isEmpty())
        ui->wavTextBox->setText(dir);
    else
        ui->wavTextBox->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("WavDir", ui->wavTextBox->text());
    settings.endGroup();
}

void MainWindow::on_RebuildFolderButton_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/build";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Build Folder", dir, options);

    if(folder.isEmpty())
        ui->rebuildTextBox->setText(dir);
    else
        ui->rebuildTextBox->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("RebuildDir", ui->rebuildTextBox->text());
    settings.endGroup();
}

// Shared actions for buttons (extract, rebuild, info)
void MainWindow::extract()
{
    ui->consoleTextBox->clear();

    QString password = ui->passwordTextBox->text().trimmed();
    bool overrideEnabled = ui->overrideCheckBox->isChecked();

    ExtractWorker *extractWorker = new ExtractWorker(password, overrideEnabled);

    QThread *thread = new QThread();
    extractWorker->moveToThread(thread); // Move extract_worker to the new thread

    // Connect signals from extract_worker to slots in MainWindow
    connect(extractWorker, &ExtractWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);
    connect(extractWorker, &ExtractWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);
    connect(extractWorker, &ExtractWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Connect thread signals for cleanup
    connect(thread, &QThread::started, extractWorker, &ExtractWorker::extract_fsb);
    connect(extractWorker, &ExtractWorker::taskFinished, thread, &QThread::quit);
    connect(thread, &QThread::finished, extractWorker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start(); // Start the thread
}

void MainWindow::rebuild()
{
    ui->consoleTextBox->clear();

    RebuildWorker *rebuildWorker = new RebuildWorker();
    QThread *thread = new QThread();
    rebuildWorker->moveToThread(thread); // Move extract_worker to the new thread

    // Connect signals from extract_worker to slots in MainWindow
    connect(rebuildWorker, &RebuildWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);
    connect(rebuildWorker, &RebuildWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);
    connect(rebuildWorker, &RebuildWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Connect thread signals for cleanup
    connect(thread, &QThread::started, rebuildWorker, &RebuildWorker::rebuild_bank);
    connect(rebuildWorker, &RebuildWorker::taskFinished, thread, &QThread::quit);
    connect(thread, &QThread::finished, rebuildWorker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start(); // Start the thread
}

void MainWindow::fsbinfo()
{
    //Fmod_FSB_List fsbList;
    //fsbList.setModal(true);
    //fsbList.exec();
}

// Context Menu Buttons
void MainWindow::on_actionExtract_triggered()
{
    extract();
}

void MainWindow::on_actionRebuild_triggered()
{
    rebuild();
}

void MainWindow::on_actionFSB_Info_triggered()
{
    fsbinfo();
}

void MainWindow::on_actionInfo_triggered()
{
    About about;
    about.setModal(true);
    about.exec();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

// Bottom of window UI

void MainWindow::on_extractButton_clicked()
{
    extract();
}


void MainWindow::on_fsbinfoButton_clicked()
{
    rebuild();
}


void MainWindow::on_rebuildButton_clicked()
{
    fsbinfo();
}

void MainWindow::on_format_comboBox_currentIndexChanged(int index)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Format", ui->format_comboBox->itemText(index).toLower());
    settings.endGroup();
}

void MainWindow::on_quality_spinBox_valueChanged(int arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Quality", arg1);
    settings.endGroup();
}

// Update Handlers
void MainWindow::handleProgressUpdate(int value)
{
    // Update a progress bar or text label on the UI
    ui->progressBar->setValue(value);
}

void MainWindow::handleConsoleUpdate(QString result)
{
    ui->consoleTextBox->append(result);
    // Auto-scroll
    ui->consoleTextBox->ensureCursorVisible();
}

void MainWindow::handleWorkFinished(QString result)
{
    // Display the final result on the UI
    ui->consoleTextBox->append(result);
}

void MainWindow::on_passwordTextBox_textChanged(const QString &arg1)
{
    if (arg1.isEmpty()) {
        ui->overrideCheckBox->setEnabled(false);
        ui->overrideCheckBox->setChecked(false);
    }
    else {
        ui->overrideCheckBox->setEnabled(true);
    }
}

