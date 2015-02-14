#ifndef STDAFX_HPP
#define STDAFX_HPP

#if defined(__cplusplus) && !defined(__OBJC__)

#if __cplusplus > 201100L
extern "C" {
char *gets(char *str);
}
#endif

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QObject>
#include <QSettings>
#include <QUrl>
#include <QFileInfo>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QTime>
#include <QStringBuilder>
#include <QComboBox>
#include <QButtonGroup>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QApplication>
#include <QDialog>
#include <QPainter>
#include <QLabel>
#include <QSharedPointer>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QMutexLocker>
#include <QTimer>
#include <QLinkedList>
#include <QVector>
#include <QDir>
#include <QJsonDocument>
#include <QtMath>
#include <QDebug>

#include <utility>
#include <array>
#include <tuple>
#include <deque>
#include <cmath>
#include <functional>

#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

#include "global.hpp"

#endif

#endif // STDAFX_HPP
