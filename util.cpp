#include "util.h"

#include <QElapsedTimer>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

QString Util::getWifiName()
{
    QElapsedTimer t;
    t.start();
    QProcess pro;
    pro.start("netsh", QStringList() << "wlan"
                                     << "show"
                                     << "interfaces");
    pro.waitForFinished(2000);
    QString netStr = QString::fromLocal8Bit(pro.readAllStandardOutput());
    qDebug() << "netsh" << t.elapsed() << "ms";

    static auto resolveInfo = [](const QString& str) -> std::tuple<QString, QString> {
        static QRegularExpression re_address("(?:物理地址|Physical address)\\s*:\\s*(?<address>\\S+)"); //非获取捕获组 + 命名捕获组
        static QRegularExpression re_name("(?:配置文件|Profile)\\s*:\\s*(?<name>\\S+)"); //注意英文系统
        auto match_address = re_address.match(str);
        auto match_name = re_name.match(str);
        QString address, name;
        if (match_address.hasMatch() && match_name.hasMatch()) {
            address = match_address.captured("address");
            name = match_name.captured("name");
        }
        return {address, name};
    };

    auto [address, name] = resolveInfo(netStr); //结构化绑定
    return name;
}

bool Util::startProcess(const QString& localFile)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(localFile));
}

QString Util::getFileName(const QString& path)
{
    return QFileInfo(path).fileName();
}
