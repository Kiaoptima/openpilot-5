#include "selfdrive/ui/qt/spinner.h"

#include <cstdio>
#include <iostream>
#include <string>

#include <QApplication>
#include <QGridLayout>
#include <QPainter>
#include <QString>
#include <QTransform>
#include <QTime>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/util.h"

TrackWidget::TrackWidget(QWidget *parent) : QWidget(parent) {
  setAttribute(Qt::WA_OpaquePaintEvent);
  setFixedSize(spinner_size);

  // pre-compute all the track imgs. make this a gif instead?
  QPixmap comma_img = QPixmap("../assets/img_spinner_comma.png").scaled(spinner_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap track_img = QPixmap("../assets/img_spinner_track.png").scaled(spinner_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  QTransform transform(1, 0, 0, 1, width() / 2, height() / 2);
  QPixmap pm(spinner_size);
  QPainter p(&pm);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  for (int i = 0; i < track_imgs.size(); ++i) {
    p.resetTransform();
    p.fillRect(0, 0, spinner_size.width(), spinner_size.height(), Qt::black);
    p.drawPixmap(0, 0, comma_img);
    p.setTransform(transform.rotate(360 / spinner_fps));
    p.drawPixmap(-width() / 2, -height() / 2, track_img);
    track_imgs[i] = pm.copy();
  }

  m_anim.setDuration(1000);
  m_anim.setStartValue(0);
  m_anim.setEndValue(int(track_imgs.size() -1));
  m_anim.setLoopCount(-1);
  m_anim.start();
  connect(&m_anim, SIGNAL(valueChanged(QVariant)), SLOT(update()));
  connect(&m_anim, SIGNAL(valueChanged(QVariant)), SIGNAL(update_track()));
}

void TrackWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.drawPixmap(0, 0, track_imgs[m_anim.currentValue().toInt()]);
}

// Spinner

Spinner::Spinner(QWidget *parent) : QWidget(parent) {
  btElapsed.start();
  QGridLayout *main_layout = new QGridLayout(this);
  main_layout->setSpacing(0);
  main_layout->setMargin(200);

  main_layout->addWidget(new TrackWidget(this), 0, 0, Qt::AlignHCenter | Qt::AlignVCenter);

  text = new QLabel();
  text->setWordWrap(true);
  text->setVisible(false);
  text->setAlignment(Qt::AlignCenter);
  main_layout->addWidget(text, 1, 0, Qt::AlignHCenter);

  progress_bar = new QProgressBar();
  progress_bar->setRange(5, 100);
  progress_bar->setTextVisible(false);
  progress_bar->setVisible(false);
  progress_bar->setFixedHeight(20);
  main_layout->addWidget(progress_bar, 1, 0, Qt::AlignHCenter);

  ip_label = new QLabel();
  const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
  for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
      device_ip = address.toString();
  }
  ip_label->setText(device_ip);
  ip_label->setVisible(false);
  main_layout->addWidget(ip_label, 0, 0, Qt::AlignRight | Qt::AlignTop);

  bt_label = new QLabel();
  bt_label->setText("00:00");
  bt_label->setVisible(false);
  main_layout->addWidget(bt_label, 0, 0, Qt::AlignLeft | Qt::AlignTop);

  setStyleSheet(R"(
    Spinner {
      background-color: black;
    }
    QLabel {
      color: white;
      font-size: 80px;
      background-color: transparent;
    }
    QProgressBar {
      background-color: #373737;
      width: 1000px;
      border solid white;
      border-radius: 10px;
    }
    QProgressBar::chunk {
      border-radius: 10px;
      background-color: white;
    }
  )");

  // notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read);
  // QObject::connect(notifier, &QSocketNotifier::activated, this, &Spinner::update);
  bttimer = new QTimer(this);
  connect(bttimer, &QTimer::timeout, this, &Spinner::update);
  bttimer->start(1000);
};

void Spinner::update(int n) {
  std::string line;
  std::getline(std::cin, line);

  if (line.length()) {
    bool number = std::all_of(line.begin(), line.end(), ::isdigit);
    text->setVisible(!number);
    progress_bar->setVisible(number);
    ip_label->setVisible(true);
    text->setText(QString::fromStdString(line));
    if (number) {
      progress_bar->setValue(std::stoi(line));
    }
    bt_label->setVisible(true);
    QString btoutTime = QTime::fromMSecsSinceStartOfDay(btElapsed.elapsed()).toString("mm:ss");
    bt_label->setText(btoutTime);
  } else {
    QString btoutTime = QTime::fromMSecsSinceStartOfDay(btElapsed.elapsed()).toString("mm:ss");
    bt_label->setText(btoutTime);
  }
}

int main(int argc, char *argv[]) {
  initApp();
  QApplication a(argc, argv);
  Spinner spinner;
  setMainWindow(&spinner);
  return a.exec();
}
