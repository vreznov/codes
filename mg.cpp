#include "mg.h"

mg::mg()
{

}

bool mg::CheckIP(QString fml_ip)
{
    //检查格式
    QRegExp regex("^(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)$");
    regex.indexIn(fml_ip);

    QStringList ls = regex.capturedTexts();
    if(ls.length() != 5) return false;

    for(int i=1; i<5; i++)
    {
        bool ok = false;
        int val = ls.at(i).toInt(&ok);
        if(!ok || val < 0 || val > 255)
            return false;
    }

    return true;
}

bool mg::CheckNum(QString fml_num)
{
    QRegExp regex("^-?\\d+\\.?\\d*$");
    int pos = regex.indexIn(fml_num);
    QStringList ls = regex.capturedTexts();
    if(pos != 0) {
        COUT<<fml_num<<" not a num"<<endl;
        return false;
    }

    return true;
}

QStringList mg::Match(QString fml_pattern, const QString &fml_str)
{
    QStringList sret;
    QRegExp regex(fml_pattern);
    if(!regex.isValid()) return sret;

    regex.indexIn(fml_str);
    sret = regex.capturedTexts();
    return sret;
}

QStringList mg::Match2(QString fml_pattern, const QString &fml_str)
{
    QStringList sret;
    QRegExp regex(fml_pattern);
    if(!regex.isValid()) return sret;
    int pos = 0;
    int count = 0;
    while ((pos = regex.indexIn(fml_str, pos)) != -1) {
        ++count;
        pos += regex.matchedLength();
        QStringList slist = regex.capturedTexts();
        sret.append(slist[0]);  //???下标到底是0还是1
    }

    return sret;
}

bool mg::QuickWriteFile(QString fml_ffn, QString fml_s, QIODevice::OpenMode fml_flag)
{
    QFile file(fml_ffn);
    if(!file.open(fml_flag)) { return false; }
    file.write(fml_s.toUtf8());
    file.close();

    return true;
}

QString mg::winExecute(QString fml_batFile)
{
    QProcess proc;
    proc.execute(fml_batFile);

    QString sresult = QString::fromLocal8Bit(proc.readAllStandardOutput());
    qDebug() << sresult;

    return sresult;
}

QString mg::winCmd(QString fml_cmd)
{
    QProcess proc;
    proc.setReadChannel(QProcess::StandardOutput);

    // ?不起作用
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
        QString sresult = QString::fromLocal8Bit(proc.readAllStandardOutput());
        qDebug() << sresult;
    });
    proc.execute("cmd", QStringList() << "/c" << fml_cmd);
//    QString sresult = QString::fromLocal8Bit(proc.readAllStandardOutput());
//    qDebug() << sresult;

    return "sresult";
}
