#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void SetPercentage(quint8 percentage);

private slots:
    void on_pushButton_File_clicked();

    void on_pushButton_Folder_clicked();

    void on_pushButton_Extract_clicked();

    void UpdateRomLabels(bool unkown);

    void UpdatePretLabels();

    void on_radioButton_AName_toggled(bool checked);

    void on_checkBox_Override_stateChanged(int arg1);

    void EnableExtract();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
