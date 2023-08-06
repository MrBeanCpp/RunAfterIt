#ifndef ITEMINFO_H
#define ITEMINFO_H
#include <QDebug>
#include <QString>
struct ItemInfo {
    ItemInfo() = default;
    ItemInfo(bool _isUsed, const QString& _target, const QString& _follow, bool _isEndWith = false, bool _isLoop = false);
    bool isVaild(void) const;
    bool isWifi(void) const;

    bool isUsed = true;
    QString target;
    QString follow;
    bool isEndWith = false;
    bool isLoop = false;

    inline static const QString WIFI = "wifi:"; //内联变量(C++17)
};
QDebug operator<<(QDebug debug, const ItemInfo& info);

#endif // ITEMINFO_H
