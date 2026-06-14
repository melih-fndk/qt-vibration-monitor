# Qt Vibration Monitor

STM32 tabanlı titreşim izleme sistemleri için geliştirilmiş gerçek zamanlı masaüstü izleme ve analiz uygulaması.

Bu proje, UART üzerinden STM32F407 mikrodenetleyicisinden alınan titreşim verilerini işleyerek kullanıcıya canlı grafikler, FFT spektrumu ve durum izleme (Condition Monitoring) bilgileri sunmaktadır.

---

## Proje Hakkında

Bu uygulama, STM32 tabanlı gömülü titreşim analiz sisteminin masaüstü tarafını oluşturmaktadır.

STM32 tarafından toplanan ve işlenen veriler UART üzerinden bilgisayara aktarılır. Qt uygulaması bu verileri ayrıştırır, analiz eder ve gerçek zamanlı olarak görselleştirir.

Sistem özellikle:

* CNC makineleri
* Spindle sistemleri
* Elektrik motorları
* Döner makineler

üzerindeki titreşim davranışlarının izlenmesi amacıyla geliştirilmiştir.

---

## Özellikler

### Gerçek Zamanlı Veri İzleme

* UART üzerinden canlı veri alma
* Sürekli veri akışı görüntüleme
* Düşük gecikmeli veri işleme

### Grafik Görselleştirme

* Gerçek zamanlı titreşim grafiği
* Frekans spektrumu görüntüleme
* Canlı veri güncelleme

### FFT Analizi

* STM32 tarafından hesaplanan FFT sonuçlarını görüntüleme
* Spektrum analizi
* Baskın frekansların izlenmesi

### Durum İzleme

* RMS değeri görüntüleme
* Peak Frequency gösterimi
* Peak Amplitude gösterimi
* Arıza frekanslarının gösterimi

### Rulman Arıza Analizi

Aşağıdaki karakteristik frekanslar sistem tarafından görüntülenebilmektedir:

* BPFO (Outer Race Fault)
* BPFI (Inner Race Fault)
* BSF (Ball Spin Fault)
* FTF (Fundamental Train Frequency)

---

## Kullanılan Teknolojiler

* Qt Framework
* Qt Creator
* C++
* UART / Serial Communication
* CMake

---

## Sistem Mimarisi

```text
LIS3DSH Accelerometer
          │
          ▼
     STM32F407
          │
          ▼
   CMSIS-DSP FFT
          │
          ▼
 UART Communication
          │
          ▼
 Qt Vibration Monitor
          │
          ▼
 Visualization & Analysis
```

---

## Proje Yapısı

```text
CMakeLists.txt
main.cpp
mainwindow.cpp
mainwindow.h
mainwindow.ui
```

---

## Derleme

Qt Creator ile açın ve projeyi derleyin.

Alternatif olarak:

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

---

## Planlanan Özellikler

* FFT spektrum grafiği geliştirmeleri
* Fault Score gösterimi
* Alarm sistemi
* Veri kayıt sistemi
* CSV dışa aktarma
* Otomatik raporlama
* Çoklu sensör desteği

---

## İlgili Depolar

Bu proje iki ayrı depodan oluşan bir sistemin masaüstü uygulama tarafıdır.

### STM32 Firmware

https://github.com/melih-fndk/stm32-cnc-vibration-monitor

### Qt Desktop Application

https://github.com/melih-fndk/qt-vibration-monitor

---

## Geliştirme Durumu

Proje aktif olarak geliştirilmektedir.

Mevcut sürüm:

* UART haberleşmesi
* Gerçek zamanlı veri alma
* Temel kullanıcı arayüzü
* Veri görselleştirme altyapısı

özelliklerini içermektedir.

---

## Lisans

Bu proje eğitim, araştırma ve durum izleme uygulamaları amacıyla geliştirilmektedir.

---

**Geliştirici:** Melih Fındık

**Platform:** Qt / C++

**Uygulama Alanı:** Gerçek zamanlı titreşim izleme ve rulman arıza analizi
