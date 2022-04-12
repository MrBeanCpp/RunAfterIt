#include "iteminfo.h"
#include <QFile>
/*
#ifndef ITEMINFO_H
#define ITEMINFO_H
这个只能处理同一编译单元（cpp） 不能处理不同cpp
*/
QDebug operator<<(QDebug debug, const ItemInfo& info) //放在头文件会重定义 inline函数不会这样 写在类内 自动inline
{
    debug << info.isUsed << info.target << info.follow << info.isEndWith << info.isLoop << '\n';
    return debug;
}

ItemInfo::ItemInfo(bool _isUsed, const QString& _target, const QString& _follow, bool _isEndWith, bool _isLoop)
    : isUsed(_isUsed), target(_target), follow(_follow), isEndWith(_isEndWith), isLoop(_isLoop)
{
}

bool ItemInfo::isVaild() const
{
    return isUsed && QFile::exists(target) && QFile::exists(follow);
}
