#pragma once
#include <QFontDatabase>
#include <QString>
inline QString jttyMonospaceFamily() {
  static QString const family=[] {
  auto const families=QFontDatabase().families();
  // Use public font names: macOS's private system fixed-font alias is not
  // consistently resolved by Qt stylesheets.
  for(auto name : {"Menlo", "Consolas", "DejaVu Sans Mono", "Liberation Mono", "Courier New"})
    if(families.contains(QString::fromLatin1(name))) return QString::fromLatin1(name);
  return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
  }();
  return family;
}
