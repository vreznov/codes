#ifndef MG_H
#define MG_H


/********************************************
 * 全局变量及函数
 *
 *
 ********************************************/
#include <QObject>
#include <QObject>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <QDebug>
#include <iostream>

using namespace std;
#define COUT qDebug()

#define SETBIT(x,y) x|=(1<<y) //将X的第Y位置1
#define RESETBIT(x,y) x&=!(1<<y) //将X的第Y位清0

#ifndef STATUS_BUFFER_SIZE
    #define STATUS_BUFFER_SIZE 50
#endif

#ifndef OPERA_BUFFER_SIZE
    #define OPERA_BUFFER_SIZE 50
#endif

#ifndef OPERA_BYTES_NUM
    #define OPERA_BYTES_NUM 100
#endif

#ifndef STATUS_BYTES_NUM
    #define STATUS_BYTES_NUM 100
#endif

#define MODBUS_SERVER_ADRESS 2

class mg
{
public:
    mg();

    static bool CheckIP(QString fml_ip);
    static bool CheckNum(QString fml_num);  //检查文本是否是有效的数字,支持小数和正负数检测
    static QStringList Match(QString fml_pattern, const QString& fml_str);  //获取单行文本的数据
    static QStringList Match2(QString fml_pattern, const QString& fml_str);  //获取所有行的数据
    //! 将指定信息写入指定文件
    static bool QuickWriteFile(QString fml_ffn, QString fml_s, QIODevice::OpenMode fml_flag = QIODevice::WriteOnly | QIODevice::Text);

    //! 执行指定windows指令. Qt的cmd执行命令时，不支持带空格的指令
    //! @param fml_cmd: bat文件地址，bat文件包含一系列的命令
    static QString winExecute(QString fml_batFile);

    //! 在执行指定windows命令. Qt的cmd执行命令时，不支持带空格的指令
    static QString winCmd(QString fml_cmd);
};

#endif // MG_H
