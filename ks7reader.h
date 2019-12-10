#ifndef KS7READER_H
#define KS7READER_H

/* ks7reader.h
 *  brief：提供与PLC的通信
 *  author: kare
 *
 *  注意：
 *      PLC端的bool变量是bit，PC端的是BYTE，所以在变量的读入写出时注意转换
 *  读写数据步骤
 *      1.连接到plc
 *      2.使用 DBRead读取数据，使用DBWrite写入数据
 *      3.结束则断开连接
 *
 * */

#include <iostream>
#include "s7_client.h"
#include <QObject>
#include <QString>
#include <QLoggingCategory>
#include <QList>

using namespace std;

typedef qint16 S7_INT;
typedef qint32 S7_DINT;
typedef float S7_REAL;
typedef double S7_LREAL;

#define MAX_AXES 4

Q_DECLARE_LOGGING_CATEGORY(s7reader)

//! PLC轴状态 HmiRead中的mot_fullSta，该字的内容如下，需要自己将对应bit转换为byte
struct PLCStc_AxisStatus{
    bool Activated;
    bool Enable;
    bool HomingDone;
    bool Done;
    bool Error;
    bool Standstill;
    bool PositioningCommand;
    bool VelocityCommand;
    bool HomingCommand;
    bool CommandTableActive;
    bool ConstantVelocity;
    bool Accelerating;
    bool Decelerating;
    bool ControlPanelActive;
    bool DriveReady;
    bool RestartRequired;
    bool SWLimitMinActive;
    bool SWLimitMaxActive;
    bool HWLimitMinActive;
    bool HWLimitMaxActive;

};

//! plc读取数据
struct HmiRead{
    bool mot_homeDone[MAX_AXES] = {};
    bool mot_ready[MAX_AXES] = {};
    bool mot_PL[MAX_AXES] = {};
    bool mot_NL[MAX_AXES] = {};
    bool mot_err[MAX_AXES] = {};
    S7_REAL mot_pos[MAX_AXES] = {};
    S7_REAL mot_vel[MAX_AXES] = {};
    PLCStc_AxisStatus mot_fullSta[MAX_AXES] = {};
    bool KA[5] = {};  //各个继电器输出
    bool ES = false;  //紧急停止
};

//! 向PLC写入的数据
struct HmiWrite{
    bool pow_up[MAX_AXES] = {};
    bool mot_reset[MAX_AXES] = {};
    bool mov_home[MAX_AXES] = {};
    bool mov_abs[MAX_AXES] = {};
    bool mov_rel[MAX_AXES] = {};
    bool halt[MAX_AXES] = {};  //停止运动
    bool jog_forward[MAX_AXES] = {};
    bool jog_backward[MAX_AXES] = {};
    bool set_KA[5] = {};  //继电器输出设置
    S7_INT home_mode[MAX_AXES] = {};  // 0-将当前位置设置为位置0  3-开始主动回零
    S7_REAL jog_vel[MAX_AXES] = {};
    S7_REAL mov_rel_distance[MAX_AXES] = {};
    S7_REAL mov_abs_target[MAX_AXES] = {};
    S7_REAL mov_vel[MAX_AXES] = {};  //相对、绝对运动速度
};


// PLC读取
class KS7Reader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(HmiRead readValues READ readValues)
//    Q_PROPERTY(HmiWrite writeValues MEMBER m_writeValues NOTIFY writeValuesChanged)

public:
    explicit KS7Reader(QObject *parent = nullptr);

    bool isConnected(); //返回连接状态


    int cnnt(); //连接到PLC
    void dis_cnnt(); //断开连接

    //!获取当前连接的plc的cpu信息
    //! \brief GetCpuInfo
    //!
    void GetCpuInfo();

    //! 获取读取的数据
    HmiRead readValues();

    //!获取写入的数据
    //! \brief writeValues
    //! \return HmiWrite的指针
    //!
    HmiWrite* writeValues();
//    void setWriteValues(HmiWrite fml_wValue);
private:
    void KInit();

    //! 从PLC读取数据
    //! @param fml_db: 要读取的 DB编号
    //! @param fml_stIndex :索引起始地址
    //! @param fml_len: 数据字节长度
    void ReqData(int fml_db = 0, int fml_stIndex = 0, int fml_len = 4); //读取计数数据

    //!向PLC写入数据 专用
    //! \brief WriteData
    //!
    void WriteData();

    TSnap7Client clt; //PLC client对象

    QString _ip= "192.168.0.10";  //plc的ip
    short m_pack = 0; //机架0
    short m_slot = 1; //s7-1200 槽号1. s7-300槽号2

    HmiRead m_readValues;  //存储从plc获取的数据
    HmiWrite m_writeValues;  //要写入到plc的数据

public:
    //!用于外部设置ip
    //! \brief setIp
    //! \param fml_ip
    //!
    void setIp(QString fml_ip);


signals:
    //! 控制器已连接
    void sig_Connected();

    //! 控制器断开连接
    void sig_Disconnected();

    void connectedChanged();

    void writeValuesChanged();
public slots:
    void Req(); //读写数据，用于外部调用
};

#endif // S7_READER_H
