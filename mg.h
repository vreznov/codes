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

#include <QDebug>
#include <iostream>

using namespace std;
#define COUT qDebug()


class mg
{
public:
    mg();

    static bool CheckIP(QString fml_ip);
    static bool CheckNum(QString fml_num);  //检查文本是否是有效的数字
    static QStringList Match(QString fml_pattern, const QString& fml_str);  //获取单行文本的数据
    static QStringList Match2(QString fml_pattern, const QString& fml_str);  //获取所有行的数据
};

#endif // MG_H
