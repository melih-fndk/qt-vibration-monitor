#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectButtonClicked();
    void readSerialData();
    void calculateFaultFrequencies();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QString serialBuffer;

    QLineSeries *seriesX;
    QLineSeries *seriesY;
    QLineSeries *seriesZ;

    QChart *chart;
    QChartView *chartView;
    QValueAxis *axisX;
    QValueAxis *axisY;

    QLineSeries *seriesFFT;
    QChart *chartFFT;
    QChartView *chartViewFFT;
    QValueAxis *axisFreq;
    QValueAxis *axisAmp;

    int sampleIndex = 0;
    const int maxSamples = 100;

    int textCounter = 0;
    int chartCounter = 0;

    double bpfoFreq = 0.0;
    double bpfiFreq = 0.0;
    double bsfFreq = 0.0;
    double ftfFreq = 0.0;

    void updateChart(int x, int y, int z);
};

#endif // MAINWINDOW_H
