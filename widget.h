#ifndef WIDGET_H
#define WIDGET_H

#include "item.h"
#include <QListWidget>
#include <QWidget>
#include <windows.h>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QFileIconProvider>
#include <wlanapi.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
    using InfoList = QList<ItemInfo>;
    using ProcessList = QList<QPair<DWORD, QString>>;
    using PathSet = QSet<QString>;

public:
    Widget(QWidget* parent = nullptr);
    ~Widget();
    void addItem(const ItemInfo& info = ItemInfo());
    QStringList parse(const QString& str);
    Item* getItemWidget(int row);
    void writeFile(void);
    void readFile(void);
    void asyncSave(void);
    void writeIni(void);
    void readIni(void);
    InfoList getInfoList(void);
    void updateList(void);
    static ProcessList enumProcess(void);
    static PathSet enumProcessPath(const ProcessList& pList);
    static PathSet enumProcessPath(void); //重载
    static QString queryProcessName(DWORD PID);
    void initSysTray(void);
    void setAutoRun(bool isAuto);
    bool isAutoRun(void);
    static void WINAPI WLANCallback(PWLAN_NOTIFICATION_DATA wlanData, PVOID context);
    void wlanStateRegister(WLAN_NOTIFICATION_CALLBACK funcCallback);

private:
    Ui::Widget* ui;

    QListWidget* lw = nullptr;
    const QString filePath = QApplication::applicationDirPath() + "/data.txt"; //R"(E:\Qt5.14.2\Projects\RunAfterIt\data.txt)";
    const QString iniPath = QApplication::applicationDirPath() + "/settings.ini";
    const QString Reg_AutoRun = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"; //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    const QString AppName = "RunAfterIt";
    const QString AppPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSystemTrayIcon* sysTray = nullptr;
    static inline QString wifiName;

    InfoList infoList;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent* event) override;

    // QWidget interface
protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void paintEvent(QPaintEvent* event) override;

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // QObject interface
public:
    bool eventFilter(QObject* watched, QEvent* event) override;
};
#endif // WIDGET_H

/*兄弟们 看源码是非常重要滴
 * setWindowState(Qt::WindowMinimized); //showMinimized();会在末尾调用SetVisible(true)导致hide失效
 * 兄弟们 看源码是非常重要滴
*/
