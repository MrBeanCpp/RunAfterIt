#define _WIN32_WINNT 0x0601 //Win7

#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QTime>
#include <QTimer>
#include <QMenu>
#include <windowsx.h>
#include <QPainter>
#include <QWindowStateChangeEvent>
#include <QDesktopServices>
#include <QtConcurrent>
#include <winbase.h> //必须在windows.h后
#include <tlhelp32.h> //必须在windows.h后 否则报错（需要一些定义）
Widget::Widget(QWidget* parent) //增加禁用按钮 & 是否持续监测（or 只在启动瞬间）
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint); //否则nativeEvent拉伸窗体不响应
    setWindowTitle("RunAfterIt by MrBeanC");
    ui->label_title->setText("RunAfterIt by MrBeanC");
    lw = ui->listWidget;

    lw->setDragDropMode(QAbstractItemView::InternalMove);
    lw->setDefaultDropAction(Qt::MoveAction);
    lw->setSelectionMode(QAbstractItemView::SingleSelection);
    lw->viewport()->setAcceptDrops(true); //支持拖拽 在设计师界面修改貌似无效

    lw->viewport()->installEventFilter(this);

    setAcceptDrops(true);

    initSysTray();

    connect(ui->btn_min, &QPushButton::clicked, this, &Widget::showMinimized);
    connect(ui->btn_close, &QPushButton::clicked, this, &Widget::close);
    connect(ui->btn_add, &QPushButton::clicked, [=]() {
        addItem();
    });

    QTimer* timer = new QTimer(this);
    timer->callOnTimeout([=]() {
        static PathSet preSet;
        auto pList = enumProcess();
        auto pSet = enumProcessPath(pList);
        qDebug() << "Processes:" << pSet.size() << QTime::currentTime();

        QSet<QString> livePathList; //应当存活的进程
        QSet<QString> startedPathList;
        QSet<QPair<DWORD, QString>> endList; //使用set存储再统一执行 防止启动和终止列表冲突
        for (const auto& info : qAsConst(infoList)) {
            if (!info.isVaild()) { //not exist
                qDebug() << "#Not Valid:" << info;
                continue;
            }

            bool isTargetExist = pSet.contains(info.target);
            bool isTargetExisted = preSet.contains(info.target);
            bool isFollowExist = pSet.contains(info.follow);
            bool isTargetStart = !isTargetExisted && isTargetExist; //开启的瞬间 or 首次检测到存在(preSet.empty())
            bool isTargetEnd = isTargetExisted && !isTargetExist; //结束的瞬间

            if (isTargetExist) //只要target存活 follow就应当存活（处理的是多个follow相同的情况 防止冲突 而被end）
                livePathList << info.follow;

            if (isTargetExist && !isFollowExist) {
                if (info.isLoop || isTargetStart) { //not loop 只在A的开启瞬间启动B 不会重复启动
                    if (!startedPathList.contains(info.follow)) { //防止重复启动 (when列表中有多个相同follow)
                        QString target = getFileName(info.target);
                        QString follow = getFileName(info.follow);

                        QDesktopServices::openUrl(QUrl::fromLocalFile(info.follow));
                        qDebug() << "#Detect:" << target << "then #Run:" << follow;
                        livePathList << info.follow; //当然启动也算应当存活
                        startedPathList << info.follow;
                    }
                }
            } else if (isTargetEnd && isFollowExist && info.isEndWith) {
                for (const auto& P : qAsConst(pList)) {
                    QString path = queryProcessName(P.first);
                    if (path == info.follow) {
                        //                        HANDLE Process = OpenProcess(PROCESS_TERMINATE, FALSE, P.first);
                        //                        bool ret = TerminateProcess(Process, 0);
                        //                        qDebug() << "#Terminate:" << path << ret;
                        endList << qMakePair(P.first, path); //PID + fullPath
                    }
                }
            }
        }

        for (auto P : qAsConst(endList))
            if (!livePathList.contains(P.second)) { //确保与启动列表不冲突
                HANDLE Process = OpenProcess(PROCESS_TERMINATE, FALSE, P.first);
                bool ret = TerminateProcess(Process, 0);
                qDebug() << "#Terminate:" << P.second << ret;
            }

        preSet = pSet;
    });
    timer->start(2000);

    readFile();
    readIni();
    sysTray->showMessage("Message", "RunAfterIt Started");
}

Widget::~Widget()
{
    delete ui;
}

void Widget::addItem(const ItemInfo& info)
{
    QListWidgetItem* widgetItem = new QListWidgetItem(lw);
    widgetItem->setSizeHint(QSize(0, 30));
    Item* item = new Item(widgetItem, lw); //如果需要Item了解row信息，可以传入QListWidgetItem指针
    item->setData(info);
    lw->addItem(widgetItem);
    lw->setItemWidget(widgetItem, item);

    connect(item, &Item::contentChanged, [=]() {
        qDebug() << "#Changed" << QTime::currentTime();
        asyncSave();
    });

    connect(item, &Item::deleteButtonClicked, [=](QListWidgetItem* widgetItem) {
        lw->removeItemWidget(widgetItem);
        int row = lw->row(widgetItem); //获取当前鼠标所选行
        delete lw->takeItem(row); //删除该行
        qDebug() << "#Delete";
        asyncSave();
    });
}

QStringList Widget::parse(const QString& str)
{
    static QRegExp rx("\"(.*)\"");
    rx.setMinimal(true);
    QStringList list;
    int pos = 0;
    while ((pos = rx.indexIn(str, pos)) != -1) {
        list << rx.cap(1);
        pos += rx.matchedLength();
    }
    return list;
}

Item* Widget::getItemWidget(int row)
{
    if (lw == nullptr) return nullptr;
    QListWidgetItem* item = lw->item(row);
    return static_cast<Item*>(lw->itemWidget(item));
}

void Widget::writeFile()
{
    static auto addQuote = [](const QString& str) {
        return "\"" + str + "\"";
    };
    QFile file(filePath);
    if (file.open(QFile::Text | QIODevice::WriteOnly)) {
        QTextStream text(&file);
        text.setCodec("UTF-8");
        int rows = lw->count();
        for (int i = 0; i < rows; i++) {
            Item* itemWidget = getItemWidget(i);
            ItemInfo info = itemWidget->getData();
            text << QString("%1 %2 %3 %4 %5\n")
                        .arg(addQuote(info.isUsed ? "1" : "0"))
                        .arg(addQuote(info.target))
                        .arg(addQuote(info.follow))
                        .arg(addQuote(info.isEndWith ? "1" : "0"))
                        .arg(addQuote(info.isLoop ? "1" : "0"));
        }
        file.close();
        updateList();
    }
    qDebug() << "saved";
}

void Widget::readFile()
{
    QFile file(filePath);
    if (file.open(QFile::Text | QFile::ReadOnly)) {
        QTextStream text(&file);
        text.setCodec("UTF-8");
        while (!text.atEnd()) {
            QString line = text.readLine();
            QStringList list = parse(line);
            if (list.size() == 5)
                addItem(ItemInfo(list[0] == "1", list[1], list[2], list[3] == "1", list[4] == "1"));
            else
                qWarning() << "InfoSize Error";
        }
        file.close();
        updateList();
    }
}

void Widget::asyncSave()
{
    QtConcurrent::run([=]() {
        writeFile();
    });
}

void Widget::writeIni()
{
    qDebug() << "#writeIni";
    QSettings iniSet(iniPath, QSettings::IniFormat);
    iniSet.setValue("Size", size());
}

void Widget::readIni()
{
    qDebug() << "#readIni";
    QSettings iniSet(iniPath, QSettings::IniFormat);
    QVariant var = iniSet.value("Size");
    if (var.isValid())
        resize(var.toSize());
}

Widget::InfoList Widget::getInfoList()
{
    InfoList list;
    int rows = lw->count();
    for (int i = 0; i < rows; i++) {
        Item* itemWidget = getItemWidget(i);
        list << itemWidget->getData();
    }
    return list;
}

void Widget::updateList()
{
    infoList = getInfoList();
    //qDebug() << list;
}

Widget::ProcessList Widget::enumProcess()
{
    ProcessList list;
    HANDLE hProcessSnap;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return list;

    if (Process32First(hProcessSnap, &entry)) {
        do {
            list << qMakePair(entry.th32ProcessID, QString::fromWCharArray(entry.szExeFile));
        } while (Process32Next(hProcessSnap, &entry));
    }

    CloseHandle(hProcessSnap);
    return list;
}

Widget::PathSet Widget::enumProcessPath(const ProcessList& pList)
{
    static QMap<DWORD, QString> map; //缓存优化
    PathSet set;
    QString path;
    for (const auto& P : pList) {
        DWORD PID = P.first;
        if (map.contains(PID) && map[PID].endsWith(P.second)) //校对fileName防止PID被重用
            path = map[PID];
        else
            path = queryProcessName(PID); //比较耗时
        if (!path.isEmpty()) {
            set << path;
            map.insert(PID, path);
        }
    }
    return set;
}

Widget::PathSet Widget::enumProcessPath()
{
    return enumProcessPath(enumProcess());
}

QString Widget::queryProcessName(DWORD PID)
{
    static WCHAR path[512];
    QString sPath;
    if (HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID)) { //不是特别耗时
        DWORD size = _countof(path);
        if (QueryFullProcessImageName(Process, 0, path, &size)) //Windows Vista之后可用 //无法获取服务和系统进程 由于path为static 所以失败后 path表现为不变
            sPath = QString::fromWCharArray(path);
        CloseHandle(Process);
    }
    return sPath;
}

QString Widget::getFileName(const QString& path)
{
    return QFileInfo(path).fileName();
}

void Widget::initSysTray()
{
    if (sysTray) return;
    sysTray = new QSystemTrayIcon(this);
    sysTray->setIcon(QIcon(":/Images/ICON_WC.ico"));
    sysTray->setToolTip("RunAfterIt");
    connect(sysTray, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            showNormal(), activateWindow();
    });
    connect(sysTray, &QSystemTrayIcon::messageClicked, this, &Widget::showNormal);

    QMenu* menu = new QMenu(this);
    menu->setStyleSheet("QMenu{background-color:rgb(15,15,15);color:rgb(220,220,220);}"
                        "QMenu:selected{background-color:rgb(60,60,60);}");
    QAction* act_autoStart = new QAction("AutoStart", menu);
    QAction* act_quit = new QAction("Quit>>", menu);
    act_autoStart->setCheckable(true);
    act_autoStart->setChecked(isAutoRun());
    connect(act_autoStart, &QAction::toggled, [=](bool checked) {
        setAutoRun(checked);
        sysTray->showMessage("Tip", checked ? "已添加启动项" : "已移除启动项");
    });
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
    menu->addAction(act_autoStart);
    menu->addAction(act_quit);

    sysTray->setContextMenu(menu);
    sysTray->show();
}

void Widget::setAutoRun(bool isAuto) //如果想区分是（开机启动|手动启动）可以加上启动参数 用来判别
{
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    if (isAuto)
        reg.setValue(AppName, AppPath);
    else
        reg.remove(AppName);
}

bool Widget::isAutoRun()
{
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    return reg.value(AppName).toString() == AppPath;
}

void Widget::closeEvent(QCloseEvent* event)
{
    hide();
    sysTray->showMessage("Message", "已隐藏到托盘");
    writeIni();
    event->ignore();
}

bool Widget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType);
    MSG* msg = (MSG*)message;
    switch (msg->message) {
    case WM_NCHITTEST:
        int xPos = GET_X_LPARAM(msg->lParam) - this->frameGeometry().x();
        int yPos = GET_Y_LPARAM(msg->lParam) - this->frameGeometry().y();
        const int BW = 4; //boundaryWidth
        if (xPos < BW && yPos < BW) //左上角
            *result = HTTOPLEFT;
        else if (xPos >= width() - BW && yPos < BW) //右上角
            *result = HTTOPRIGHT;
        else if (xPos < BW && yPos >= height() - BW) //左下角
            *result = HTBOTTOMLEFT;
        else if (xPos >= width() - BW && yPos >= height() - BW) //右下角
            *result = HTBOTTOMRIGHT;
        else if (xPos < BW) //左边
            *result = HTLEFT;
        else if (xPos >= width() - BW) //右边
            *result = HTRIGHT;
        else if (yPos < BW) //上边
            *result = HTTOP;
        else if (yPos >= height() - BW) //下边
            *result = HTBOTTOM;
        else if (ui->label_title->geometry().contains(xPos, yPos)) //标题栏(伪)
            *result = HTCAPTION;
        else //其他部分不做处理，返回false，留给其他事件处理器处理
            return false;
        return true;
    }
    return false; //此处返回false，留给其他事件处理器处理
}

void Widget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QRect Rect(this->rect());
    painter.drawRect(Rect.x(), Rect.y(), Rect.width() - 1, Rect.height() - 1); //绘制边框
}

void Widget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        SwitchToThisWindow(HWND(winId()), true);
    } else
        event->ignore();
}

void Widget::dropEvent(QDropEvent* event)
{
    QString path = event->mimeData()->urls().at(0).toLocalFile();
    addItem(ItemInfo(true, QDir::toNativeSeparators(path), ""));
}

bool Widget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == lw->viewport() && event->type() == QEvent::Drop) {
        qDebug() << "drop";
        asyncSave();
    }
    return false;
}
