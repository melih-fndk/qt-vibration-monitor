#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMessageBox>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QPushButton>
#include <QPainter>
#include <QVBoxLayout>
#include <QtGlobal>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /*
     * ==========================================================
     * TEMEL PENCERE VE ARAYÜZ AYARLARI
     * ==========================================================
     */

    this->resize(1200, 800);

    ui->labelTitle->setStyleSheet("font-size: 18px; font-weight: bold;");
    ui->labelStatus->setStyleSheet("font-weight: bold;");
    ui->textEdit->document()->setMaximumBlockCount(50);

    ui->labelBPFO->setText("BPFO: -");
    ui->labelBPFI->setText("BPFI: -");
    ui->labelBSF->setText("BSF: -");
    ui->labelFTF->setText("FTF: -");
    ui->labelFaultResult_2->setText("Muhtemel Hasar: -");

    // Test / varsayılan rulman bilgileri
    ui->lineEditRPM->setText("60");
    ui->lineEditBallCount->setText("8");
    ui->lineEditBallDiameter->setText("5");
    ui->lineEditPitchDiameter->setText("25");
    ui->lineEditContactAngle->setText("0");

    /*
     * ==========================================================
     * SERİ PORT NESNESİ
     * ==========================================================
     */

    serial = new QSerialPort(this);
    sampleIndex = 0;

    /*
     * ==========================================================
     * 1) X-Y-Z ZAMAN DOMENİ GRAFİĞİ
     * ==========================================================
     */

    seriesX = new QLineSeries();
    seriesY = new QLineSeries();
    seriesZ = new QLineSeries();

    seriesX->setName("X Ekseni");
    seriesY->setName("Y Ekseni");
    seriesZ->setName("Z Ekseni");

    chart = new QChart();
    chart->addSeries(seriesX);
    chart->addSeries(seriesY);
    chart->addSeries(seriesZ);

    chart->setTitle("Canlı X-Y-Z İvme Grafiği");
    chart->legend()->setVisible(true);

    axisX = new QValueAxis();
    axisX->setTitleText("Örnek");
    axisX->setRange(0, maxSamples);

    axisY = new QValueAxis();
    axisY->setTitleText("Sensör Değeri");
    axisY->setRange(-20000, 20000);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    seriesX->attachAxis(axisX);
    seriesX->attachAxis(axisY);

    seriesY->attachAxis(axisX);
    seriesY->attachAxis(axisY);

    seriesZ->attachAxis(axisX);
    seriesZ->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *timeLayout = new QVBoxLayout(ui->timeChartContainer);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->addWidget(chartView);

    /*
     * ==========================================================
     * 2) FFT FREKANS SPEKTRUMU GRAFİĞİ
     * ==========================================================
     */

    seriesFFT = new QLineSeries();
    seriesFFT->setName("FFT - Bileşke Titreşim");

    chartFFT = new QChart();
    chartFFT->addSeries(seriesFFT);
    chartFFT->setTitle("Bileşke Titreşim FFT Frekans Spektrumu");
    chartFFT->legend()->setVisible(true);

    axisFreq = new QValueAxis();
    axisFreq->setTitleText("Frekans (Hz)");
    axisFreq->setRange(0, 50);

    axisAmp = new QValueAxis();
    axisAmp->setTitleText("Genlik");
    axisAmp->setRange(0, 20000);

    chartFFT->addAxis(axisFreq, Qt::AlignBottom);
    chartFFT->addAxis(axisAmp, Qt::AlignLeft);

    seriesFFT->attachAxis(axisFreq);
    seriesFFT->attachAxis(axisAmp);

    chartViewFFT = new QChartView(chartFFT);
    chartViewFFT->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *fftLayout = new QVBoxLayout(ui->fftChartContainer);
    fftLayout->setContentsMargins(0, 0, 0, 0);
    fftLayout->addWidget(chartViewFFT);

    /*
     * ==========================================================
     * SIGNAL-SLOT BAĞLANTILARI
     * ==========================================================
     */

    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);

    connect(serial, &QSerialPort::readyRead,
            this, &MainWindow::readSerialData);

    connect(ui->calculateFaultButton, &QPushButton::clicked,
            this, &MainWindow::calculateFaultFrequencies);
}

MainWindow::~MainWindow()
{
    if (serial && serial->isOpen()) {
        serial->close();
    }

    delete ui;
}

void MainWindow::onConnectButtonClicked()
{
    serial->setPortName("COM5");
    serial->setBaudRate(921600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    // ÖNEMLİ:
    // ReadOnly değil, ReadWrite olacak.
    // Çünkü Qt hem STM32'den veri okuyacak hem de STM32'ye CFG komutu gönderecek.
    if (serial->open(QIODevice::ReadWrite)) {
        ui->textEdit->append("COM5 bağlantısı açıldı.\n");
        ui->connectButton->setEnabled(false);

        ui->labelStatus->setText("Durum: Veri alımı başladı");
        ui->labelStatus->setStyleSheet("color: lightgreen; font-weight: bold;");
    } else {
        QMessageBox::critical(this,
                              "Bağlantı Hatası",
                              "COM5 açılamadı.\nPuTTY kapalı mı kontrol et.");
    }
}

void MainWindow::readSerialData()
{
    QByteArray data = serial->readAll();
    serialBuffer += QString::fromUtf8(data);

    while (serialBuffer.contains('\n')) {
        int endIndex = serialBuffer.indexOf('\n');
        QString line = serialBuffer.left(endIndex).trimmed();
        serialBuffer.remove(0, endIndex + 1);

        if (line.isEmpty())
            continue;

        QStringList values = line.split(',');

        if (values.isEmpty())
            continue;

        QString packetType = values[0].trimmed();

        /*
         * ======================================================
         * RAW,x,y,z
         * STM32'den gelen ham sensör verisi
         * ======================================================
         */
        if (packetType == "RAW" && values.size() == 4) {
            bool okX, okY, okZ;

            int x = values[1].toInt(&okX);
            int y = values[2].toInt(&okY);
            int z = values[3].toInt(&okZ);

            if (okX && okY && okZ) {
                ui->labelX->setText(QString("X: %1").arg(x));
                ui->labelY->setText(QString("Y: %1").arg(y));
                ui->labelZ->setText(QString("Z: %1").arg(z));

                textCounter++;
                chartCounter++;

                // Ham veri kutusunu her 10 veride bir güncelle
                if (textCounter >= 10) {
                    ui->textEdit->append(line);
                    textCounter = 0;
                }

                // Zaman grafiğini her 3 veride bir güncelle
                if (chartCounter >= 3) {
                    updateChart(x, y, z);
                    chartCounter = 0;
                }
            }
        }

        /*
         * ======================================================
         * FAULT_FREQ,BPFO=...,BPFI=...,BSF=...,FTF=...
         * STM32 tarafında hesaplanan rulman hata frekansları
         * ======================================================
         */
        else if (packetType == "FAULT_FREQ" && values.size() == 5) {
            bool okBpfo = false;
            bool okBpfi = false;
            bool okBsf = false;
            bool okFtf = false;

            bpfoFreq = values[1].section('=', 1, 1).toDouble(&okBpfo);
            bpfiFreq = values[2].section('=', 1, 1).toDouble(&okBpfi);
            bsfFreq  = values[3].section('=', 1, 1).toDouble(&okBsf);
            ftfFreq  = values[4].section('=', 1, 1).toDouble(&okFtf);

            if (okBpfo && okBpfi && okBsf && okFtf) {
                ui->labelBPFO->setText(QString("BPFO: %1 Hz | Genlik: -")
                                           .arg(bpfoFreq, 0, 'f', 2));

                ui->labelBPFI->setText(QString("BPFI: %1 Hz | Genlik: -")
                                           .arg(bpfiFreq, 0, 'f', 2));

                ui->labelBSF->setText(QString("BSF: %1 Hz | Genlik: -")
                                          .arg(bsfFreq, 0, 'f', 2));

                ui->labelFTF->setText(QString("FTF: %1 Hz | Genlik: -")
                                          .arg(ftfFreq, 0, 'f', 2));

                ui->textEdit->append(line);
            }
        }

        /*
         * ======================================================
         * FFT,peakFreq,peakAmp,rms,bpfoAmp,bpfiAmp,bsfAmp,ftfAmp
         * STM32 üzerinde CMSIS-DSP ile hesaplanan FFT analiz sonucu
         * ======================================================
         */
        else if (packetType == "FFT" && values.size() == 8) {
            bool okPeakFreq, okPeakAmp, okRms;
            bool okBpfo, okBpfi, okBsf, okFtf;

            double peakFreq = values[1].toDouble(&okPeakFreq);
            double peakAmp  = values[2].toDouble(&okPeakAmp);
            double rms      = values[3].toDouble(&okRms);

            double bpfoAmp  = values[4].toDouble(&okBpfo);
            double bpfiAmp  = values[5].toDouble(&okBpfi);
            double bsfAmp   = values[6].toDouble(&okBsf);
            double ftfAmp   = values[7].toDouble(&okFtf);

            if (okPeakFreq && okPeakAmp && okRms &&
                okBpfo && okBpfi && okBsf && okFtf) {

                /*
                 * Yeni FFT sonucu geldiğinde eski spektrum temizlenir.
                 * Sonrasında gelen SPEC satırları grafiği doldurur.
                 */
                seriesFFT->clear();
                axisFreq->setRange(0, 800);
                axisAmp->setRange(0, 100);

                ui->labelPeakFreq->setText(QString("Baskın Frekans: %1 Hz")
                                               .arg(peakFreq, 0, 'f', 2));

                ui->labelPeakAmp->setText(QString("Tepe Genlik: %1")
                                              .arg(peakAmp, 0, 'f', 1));

                ui->labelRms->setText(QString("RMS: %1")
                                          .arg(rms, 0, 'f', 1));

                ui->labelBPFO->setText(
                    bpfoAmp < 0
                        ? QString("BPFO: %1 Hz | Ölçüm dışı").arg(bpfoFreq, 0, 'f', 2)
                        : QString("BPFO: %1 Hz | Genlik: %2").arg(bpfoFreq, 0, 'f', 2).arg(bpfoAmp, 0, 'f', 1)
                    );

                ui->labelBPFI->setText(
                    bpfiAmp < 0
                        ? QString("BPFI: %1 Hz | Ölçüm dışı").arg(bpfiFreq, 0, 'f', 2)
                        : QString("BPFI: %1 Hz | Genlik: %2").arg(bpfiFreq, 0, 'f', 2).arg(bpfiAmp, 0, 'f', 1)
                    );

                ui->labelBSF->setText(
                    bsfAmp < 0
                        ? QString("BSF: %1 Hz | Ölçüm dışı").arg(bsfFreq, 0, 'f', 2)
                        : QString("BSF: %1 Hz | Genlik: %2").arg(bsfFreq, 0, 'f', 2).arg(bsfAmp, 0, 'f', 1)
                    );

                ui->labelFTF->setText(
                    ftfAmp < 0
                        ? QString("FTF: %1 Hz | Ölçüm dışı").arg(ftfFreq, 0, 'f', 2)
                        : QString("FTF: %1 Hz | Genlik: %2").arg(ftfFreq, 0, 'f', 2).arg(ftfAmp, 0, 'f', 1)
                    );
                /*
                 * BPFO/BPFI/BSF/FTF genliklerini karşılaştırarak
                 * muhtemel hasar bölgesi yorumu yapılır.
                 */
                double maxFaultAmp = -1.0;
                QString faultText = "Muhtemel Hasar: Ölçülebilir aralıkta belirgin artış yok";

                if (bpfoAmp >= 0 && bpfoAmp > maxFaultAmp) {
                    maxFaultAmp = bpfoAmp;
                    faultText = "Muhtemel Hasar: BPFO bölgesi → Dış bilezik hasarı şüphesi";
                }

                if (bpfiAmp >= 0 && bpfiAmp > maxFaultAmp) {
                    maxFaultAmp = bpfiAmp;
                    faultText = "Muhtemel Hasar: BPFI bölgesi → İç bilezik hasarı şüphesi";
                }

                if (bsfAmp >= 0 && bsfAmp > maxFaultAmp) {
                    maxFaultAmp = bsfAmp;
                    faultText = "Muhtemel Hasar: BSF bölgesi → Bilye hasarı şüphesi";
                }

                if (ftfAmp >= 0 && ftfAmp > maxFaultAmp) {
                    maxFaultAmp = ftfAmp;
                    faultText = "Muhtemel Hasar: FTF bölgesi → Kafes hasarı şüphesi";
                }

                if (maxFaultAmp < 100.0) {
                    faultText = "Muhtemel Hasar: BPFO/BPFI/BSF/FTF bölgelerinde belirgin artış yok";
                }
                ui->labelFaultResult_2->setText(faultText);

                if (rms < 500) {
                    ui->labelStatus->setText("Durum: Normal titreşim seviyesi");
                    ui->labelStatus->setStyleSheet("color: lightgreen; font-weight: bold;");
                } else if (rms < 2000) {
                    ui->labelStatus->setText("Durum: Artan titreşim izleniyor");
                    ui->labelStatus->setStyleSheet("color: orange; font-weight: bold;");
                } else {
                    ui->labelStatus->setText("Durum: Yüksek titreşim uyarısı");
                    ui->labelStatus->setStyleSheet("color: red; font-weight: bold;");
                }
            }
        }

        /*
         * ======================================================
         * SPEC,frekans,genlik
         * STM32 tarafından gönderilen FFT spektrum noktası
         * ======================================================
         */
        else if (packetType == "SPEC" && values.size() == 3) {
            bool okFreq, okAmp;

            double freq = values[1].toDouble(&okFreq);
            double amp  = values[2].toDouble(&okAmp);

            if (okFreq && okAmp) {
                seriesFFT->append(freq, amp);

                if (amp > axisAmp->max()) {
                    axisAmp->setRange(0, amp * 1.2);
                }
            }
        }

        /*
         * WHO_AM_I, CFG_OK, ERROR veya tanınmayan satırlar
         */
        else {
            ui->textEdit->append(line);
        }
    }
}

void MainWindow::updateChart(int x, int y, int z)
{
    seriesX->append(sampleIndex, x);
    seriesY->append(sampleIndex, y);
    seriesZ->append(sampleIndex, z);

    sampleIndex++;

    if (seriesX->count() > maxSamples) {
        int removeCount = seriesX->count() - maxSamples;

        seriesX->removePoints(0, removeCount);
        seriesY->removePoints(0, removeCount);
        seriesZ->removePoints(0, removeCount);
    }

    if (sampleIndex > maxSamples) {
        axisX->setRange(sampleIndex - maxSamples, sampleIndex);
    } else {
        axisX->setRange(0, maxSamples);
    }

    int maxVal = qMax(qMax(qAbs(x), qAbs(y)), qAbs(z));

    if (maxVal < 20000) {
        maxVal = 20000;
    }

    axisY->setRange(-maxVal - 1000, maxVal + 1000);
}

void MainWindow::calculateFaultFrequencies()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this,
                             "Bağlantı Yok",
                             "Önce COM port bağlantısını aç.");
        return;
    }

    QString rpm = ui->lineEditRPM->text().trimmed();
    QString ballCount = ui->lineEditBallCount->text().trimmed();
    QString ballDiameter = ui->lineEditBallDiameter->text().trimmed();
    QString pitchDiameter = ui->lineEditPitchDiameter->text().trimmed();
    QString contactAngle = ui->lineEditContactAngle->text().trimmed();

    QString command = QString("CFG,%1,%2,%3,%4,%5\n")
                          .arg(rpm)
                          .arg(ballCount)
                          .arg(ballDiameter)
                          .arg(pitchDiameter)
                          .arg(contactAngle);

    qint64 written = serial->write(command.toUtf8());
    serial->flush();

    if (written == -1) {
        QMessageBox::warning(this,
                             "Gönderme Hatası",
                             "STM32'ye CFG komutu gönderilemedi.");
        return;
    }

    ui->textEdit->append("Gönderilen komut: " + command.trimmed());
    ui->labelFaultResult_2->setText("Muhtemel Hasar: STM32'ye rulman bilgileri gönderildi.");
}
