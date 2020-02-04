#include "include/mainwindow.h"
#include "include/binary_utils.h"
#include "include/gba_music_utils.h"
#include "include/pret_utils.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <limits.h>

static QString getHexInputDialog(QWidget *parent, const QString &title, const QString &label,
                      int value = 0, int min = INT_MIN, int max = INT_MAX, int step = 1,
                      bool *ok = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(400, 460);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_File_clicked()
{
    bool unkownROM;
    QString fPath = QFileDialog::getOpenFileName(this, "Select GBA ROM File:",
                                                 QDir::homePath());

    if (InitROMFile(fPath))
    {
        romReady = false;
        if (!IsROMFile())
        {
            QMessageBox::critical(this,
                               "Error",
                               "Selected File is not a ROM");
        }
        else
            if (!CheckRomVersion())
            {
                QMessageBox::StandardButton reply = QMessageBox::question(this,
                                   "Error",
                                   "Unkown ROM.\nWanna Manually Insert SongTable Offset?",
                                    QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                {
                    QString offset;
                    bool ok;

                    offset = getHexInputDialog(this, "SongTable Offset", "label", 0, 0, 0x1FFFFFF, 4, &ok);

                    if (ok)
                    {
                        quint32 of;

                        romSongTableOffset = offset.toUInt(&ok, 16);
                        of = romSongTableOffset;
                        romReady = true;
                        unkownROM = true;
                        QMessageBox::about(this,
                                           "ROM Loaded",
                                           "Unkown ROM Ready.");
                    }
                }
            }
            else
            {
                romReady = true;
                unkownROM = false;
                QMessageBox::about(this,
                                   "ROM Loaded",
                                   "ROM: " +
                                   ROM_CODES[romType] + ": " +
                                   ROM_NAMES[romType]);
            }
    }

    if (romReady)
    {
        InitROMData(unkownROM);
        UpdateRomLabels(unkownROM);
        EnableExtract();
    }
}

void MainWindow::on_pushButton_Folder_clicked()
{
    pretPath = QFileDialog::getExistingDirectory(this, "Select Your Pret Folder:",
                                                 QDir::homePath());
    pretReady = InitPretRepoData();
    UpdatePretLabels();
    EnableExtract();
}

void MainWindow::on_pushButton_Extract_clicked()
{    
    OUTPUT_DIRECTORY = QFileDialog::getExistingDirectory(this,
                                                         "Choose a Folder to Extract the Data:",
                                                         QDir::homePath()) + "/music_data";

    this->ui->pushButton_Extract->setEnabled(false);
    this->ui->progressBar->setEnabled(true);
    this->ui->progressBar->setValue(0);

    ExtractROMSongData(this->ui->spinBox_FirstSong->value(),
                       this->ui->spinBox_LastSong->value(),
                       this);

    QMessageBox::about(this,
                       "Extraction Completed",
                       "All music data has been successfully extracted\n"
                       "at \"" + OUTPUT_DIRECTORY + "\"");

    this->ui->pushButton_Extract->setEnabled(true);
}

void MainWindow::UpdateRomLabels(bool unkown){

    this->ui->label_rfile->setText(romFile.fileName());
    this->ui->label_rsongTable->setText("0x"+IntToHexQString(romSongTableOffset));
    this->ui->label_rsongNum->setText(IntToDecimalQString(romSongTableSize));

    if (!unkown)
        this->ui->label_rversion->setText(ROM_CODES[romType]
                                          + " - " + ROM_NAMES[romType]);
    else
        this->ui->label_rversion->setText("UNKR - Unkown ROM");
}

void MainWindow::UpdatePretLabels()
{
    this->ui->label_pversion->setText(pretVersion);
    this->ui->label_pfolder->setText(pretPath);
    this->ui->label_LastSong->setText(IntToDecimalQString(pretSongTableSize));
}

void MainWindow::on_radioButton_AName_toggled(bool checked)
{
    automaticSongNames = checked;
}

void MainWindow::on_checkBox_Override_stateChanged(int arg1)
{
    overridePret = arg1;
}

void MainWindow::EnableExtract()
{
    if (romReady && pretReady)
    {
        this->ui->groupBox_Config->setEnabled(true);
        this->ui->radioButton_MName->setEnabled(false);
        this->ui->checkBox_Override->setEnabled(false);
        this->ui->pushButton_Extract->setEnabled(true);

        this->ui->spinBox_FirstSong->setMinimum(1);
        this->ui->spinBox_FirstSong->setMaximum(999);
        this->ui->spinBox_LastSong->setMinimum(1);
        this->ui->spinBox_LastSong->setMaximum(999);

        this->ui->spinBox_FirstSong->setValue(1);
        this->ui->spinBox_LastSong->setValue(romSongTableSize);
    }
    else
    {
        this->ui->groupBox_Config->setEnabled(false);
        this->ui->groupBox_Config->setEnabled(false);
    }
}

void MainWindow::SetPercentage(quint8 percentage)
{
    this->ui->progressBar->setValue(percentage);
}

static QString getHexInputDialog(QWidget *parent, const QString &title,
                                 const QString &label, int value,
                                 int min, int max , int step,
                                 bool *ok, Qt::WindowFlags flags){
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setIntRange(min, max);
    dialog.setIntValue(value);
    dialog.setIntStep(step);
    QSpinBox *spinbox = dialog.findChild<QSpinBox*>();
    spinbox->setDisplayIntegerBase(16);

    bool ret = dialog.exec() == QDialog::Accepted;
    if (ok)
        *ok = ret;
    return spinbox->text();
}
