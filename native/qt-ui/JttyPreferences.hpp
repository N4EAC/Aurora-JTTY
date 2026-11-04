#pragma once
#include <QSettings>
#include <QVariant>
// Use a root handle: upstream settings readers may temporarily enter groups.
inline QVariant jttyPreference(QSettings const* settings, QString const& key, QVariant fallback={}) {
  QSettings root(settings->fileName(),settings->format());
  return root.value("MainWindow/"+key,fallback);
}
inline void jttySetPreference(QSettings const* settings, QString const& key, QVariant value) {
  QSettings root(settings->fileName(),settings->format());
  root.setValue("MainWindow/"+key,value); root.sync();
}
inline int jttyMessagePoints(QSettings const* settings) {
  return qBound(8,jttyPreference(settings,"JttyMessageFontSize",14).toInt(),17);
}
