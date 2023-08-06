#ifndef UTIL_H
#define UTIL_H
#include <QString>

class Util
{
    Util() = delete;
public:
    static QString getWifiName(void);
    static bool startProcess(const QString& localFile);
    static QString getFileName(const QString& path);
};

#endif // UTIL_H
