// Monotonic beacon scheduling. No radio or audio side effects; readiness is supplied by the UI.
#pragma once
#include <QString>
#include <QRegularExpression>

class JttyBeacon {
public:
  bool enabled() const { return enabled_; }
  int interval() const { return minutes_; }
  bool setInterval(int minutes, qint64 now) {
    if (minutes < 5 || minutes > 60 || minutes % 5) return false;
    minutes_ = minutes;
    if (enabled_) due_ = now + minutes_ * 60000LL;
    return true;
  }
  void enable(bool on, qint64 now) { enabled_ = on; due_ = now + minutes_ * 60000LL; }
  qint64 remaining(qint64 now) const { return enabled_ ? qMax(qint64(0), due_ - now) : 0; }
  bool poll(qint64 now, bool ready) {
    if (!enabled_ || now < due_) return false;
    // Skip busy/unavailable intervals. Never accumulate delayed transmissions.
    due_ = now + minutes_ * 60000LL;
    return ready;
  }
  static QString message(QString call, QString grid) {
    call = call.trimmed().toUpper(); grid = grid.trimmed().toUpper();
    static QRegularExpression const callsign {"^(?=.*[A-Z])(?=.*[0-9])[A-Z0-9/]{3,15}$"};
    static QRegularExpression const locator {"^[A-R]{2}[0-9]{2}(?:[A-X]{2}(?:[0-9]{2})?)?$"};
    if (!callsign.match(call).hasMatch() || !locator.match(grid).hasMatch()) return {};
    return call + " BEACON " + grid;
  }
  static bool smoke() {
    if (message(" n4eac ","fn20ab") != "N4EAC BEACON FN20AB"
        || !message("", "FN20").isEmpty() || !message("N4EAC", "BAD").isEmpty()) return false;
    JttyBeacon b;
    if (b.enabled() || b.setInterval(6, 0) || b.poll(99999999, true)) return false;
    for (int m = 5; m <= 60; m += 5) {
      if (!b.setInterval(m,0)) return false;
      b.enable(true,100);
      if (b.poll(100+m*60000LL-1,true) || !b.poll(100+m*60000LL,true)
          || b.poll(100+m*60000LL,true)) return false;
      if (b.poll(100+2*m*60000LL,false) || b.poll(101+2*m*60000LL,true)) return false;
      b.enable(false,0);
      if (b.poll(99999999,true)) return false;
    }
    b.enable(true,0); b.setInterval(10,1);
    return !b.poll(600000,true) && b.poll(600001,true);
  }
private:
  bool enabled_ {false};
  int minutes_ {5};
  qint64 due_ {0};
};
