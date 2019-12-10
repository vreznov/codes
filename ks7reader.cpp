#include "ks7reader.h"
Q_LOGGING_CATEGORY(s7reader, "s7reader")

enum class reg_type //寄存器类型
{
    mBit = 0x01,
    mByte = 0x02,
    mWord = 0x04,
    mDouble = 0x06
};

KS7Reader::KS7Reader(QObject *parent) : QObject(parent)
{
    KInit();
}

int KS7Reader::cnnt()
{
    int ret = clt.ConnectTo(_ip.toStdString().c_str(), m_pack, m_slot);

    if(ret == 0)  //成功连接
    {
        emit sig_Connected();
        qCDebug(s7reader()) << tr("connect to plc sucess");
    }else {
        qCDebug(s7reader()) << tr("connect to plc failed");
    }

    emit connectedChanged();

    return ret;
}

void KS7Reader::dis_cnnt()
{
    if(isConnected())
    {
        clt.Disconnect();
    }
    emit sig_Disconnected();
    qCDebug(s7reader()) << tr("disconnected from plc");
    emit connectedChanged();
}



void KS7Reader::KInit()
{

}

void KS7Reader::Req()
{
    //读取数据  从plc的db1，字节0开始，读取61个字节的数据
    ReqData(1, 0, 61);

    //写入数据
    WriteData();

    //自复位变量复位，plc的某些运动指令触发需要上升沿触发，比如变量 M11负责启动运动，监测到M11由false变为true即可，为了
    // 不影响下一次的触发，需要自己对该变量进行复位
    for( int i=0; i<MAX_AXES; i++){
        m_writeValues.mov_abs[i] = false;
        m_writeValues.mov_rel[i] = false;
        m_writeValues.halt[i] = false;
        m_writeValues.mot_reset[i] = false;
        m_writeValues.mov_home[i] = false;
    }
}


void KS7Reader::ReqData(int fml_db, int fml_stIndex, int fml_len)
{
    //接收缓存区，默认256字节
    quint8 temp[256] = {0};
    int ret = clt.DBRead(fml_db, fml_stIndex, fml_len, temp);
    if(ret != 0)
    {
        qCDebug(s7reader)<<"read error"<<endl;
        return;
    }
    else
    {
        //按照格式解析，因为plc端的bool变量是bit，所以不能按照pc端的结构体直接复制，需要自己一个个的重新组合
        for(int i=0;i<MAX_AXES;i++){
            m_readValues.mot_homeDone[i] = temp[0] & (0x01 << i);
            m_readValues.mot_ready[i] = temp[2] & (0x01 << i);
            m_readValues.mot_PL[i] = temp[4] & (0x01 << i);
            m_readValues.mot_NL[i] = temp[6] & (0x01 << i);
            m_readValues.mot_err[i] = temp[8] & (0x01 << i);

            //读取的多字节数据需要进行高低位互换，可能是snap7的原因
            quint8 tmp2[4] = {};  //存放高低互换的字节
            tmp2[0] = temp[13+i*4];
            tmp2[1] = temp[12+i*4];
            tmp2[2] = temp[11+i*4];
            tmp2[3] = temp[10+i*4];
            memcpy(&m_readValues.mot_pos[i], tmp2, 4);

            tmp2[0] = temp[29+i*4];
            tmp2[1] = temp[28+i*4];
            tmp2[2] = temp[27+i*4];
            tmp2[3] = temp[26+i*4];
            memcpy(&m_readValues.mot_vel[i], tmp2, 4);

            //fullstatus未处理

        }
        m_readValues.KA[0] = temp[58] & (0x01 << 0);
        m_readValues.KA[1] = temp[58] & (0x01 << 1);
        m_readValues.KA[2] = temp[58] & (0x01 << 2);
        m_readValues.KA[3] = temp[58] & (0x01 << 3);
        m_readValues.KA[4] = temp[58] & (0x01 << 4);
        m_readValues.ES = temp[60] & 0x01;
    }
}

void KS7Reader::WriteData()
{
    //写入缓存区，默认缓存区256字节
    quint8 tmp[256] = {0};
    for(int i=0;i<MAX_AXES;i++){
        //依次填写每一个bit
        tmp[0] = tmp[0] | static_cast<quint8>(((m_writeValues.pow_up[i] & 0x01)<<i));  //pow_up
        tmp[2] = tmp[2] | static_cast<quint8>(((m_writeValues.mot_reset[i] & 0x01)<<i));  //motor error reset
        tmp[4] = tmp[4] | static_cast<quint8>(((m_writeValues.mov_home[i] & 0x01)<<i));  //move home
        tmp[6] = tmp[6] | static_cast<quint8>(((m_writeValues.mov_abs[i] & 0x01)<<i));  //absolute move
        tmp[8] = tmp[8] | static_cast<quint8>(((m_writeValues.mov_rel[i] & 0x01)<<i));  //relative move
        tmp[10] = tmp[10] | static_cast<quint8>(((m_writeValues.halt[i] & 0x01)<<i));  //motor halt
        tmp[12] = tmp[12] | static_cast<quint8>(((m_writeValues.jog_forward[i] & 0x01)<<i));  //jog forward
        tmp[14] = tmp[14] | static_cast<quint8>(((m_writeValues.jog_backward[i] & 0x01)<<i));  //jog backward

        //回零模式只能等于0或3，所以只需要填写一个字节即可，由于高低字节颠倒，所以填写本地的高字节就等于填写plc端的低字节
        tmp[19+i*2] = static_cast<quint8>(m_writeValues.home_mode[i]);  //模式只需要填写一个字节即可

        //多字节数据需要进行高低位互换
        //jog速度
        tmp[26+i*4] = reinterpret_cast<quint8*>(&m_writeValues.jog_vel[i])[3];
        tmp[27+i*4] = reinterpret_cast<quint8*>(&m_writeValues.jog_vel[i])[2];
        tmp[28+i*4] = reinterpret_cast<quint8*>(&m_writeValues.jog_vel[i])[1];
        tmp[29+i*4] = reinterpret_cast<quint8*>(&m_writeValues.jog_vel[i])[0];

        //相对运动目标值
        tmp[42+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_rel_distance[i])[3];
        tmp[43+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_rel_distance[i])[2];
        tmp[44+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_rel_distance[i])[1];
        tmp[45+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_rel_distance[i])[0];

        //绝对运动目标值
        tmp[58+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_abs_target[i])[3];
        tmp[59+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_abs_target[i])[2];
        tmp[60+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_abs_target[i])[1];
        tmp[61+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_abs_target[i])[0];

        //运动速度
        tmp[74+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_vel[i])[3];
        tmp[75+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_vel[i])[2];
        tmp[76+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_vel[i])[1];
        tmp[77+i*4] = reinterpret_cast<quint8*>(&m_writeValues.mov_vel[i])[0];
    }

    //数字输出
    tmp[16] = tmp[16] | static_cast<quint8>((m_writeValues.set_KA[0] & 0x01));
    tmp[16] = tmp[16] | static_cast<quint8>(((m_writeValues.set_KA[1] & 0x01) << 1));
    tmp[16] = tmp[16] | static_cast<quint8>(((m_writeValues.set_KA[2] & 0x01) << 2));
    tmp[16] = tmp[16] | static_cast<quint8>(((m_writeValues.set_KA[3] & 0x01) << 3));
    tmp[16] = tmp[16] | static_cast<quint8>(((m_writeValues.set_KA[4] & 0x01) << 4));

    //向plc的db2从字节0开始，写入90个字节的数据
    int ret = clt.DBWrite(2, 0, 90, static_cast<void*>(tmp));
    if(ret != 0){
        qCDebug(s7reader()) << "write to plc error";
    }

}


void KS7Reader::GetCpuInfo()
{
    TS7CpuInfo cinfo = {};
    int ret = clt.GetCpuInfo(&cinfo);
    qCDebug(s7reader())<<"readccpu info"<<ret<<endl;
}

HmiRead KS7Reader::readValues()
{
    return m_readValues;
}

HmiWrite *KS7Reader::writeValues()
{
     return  &m_writeValues;
}

//void KS7Reader::setWriteValues(HmiWrite fml_wValue)
//{
//    m_writeValues = fml_wValue;
//}

void KS7Reader::setIp(QString fml_ip)
{
    _ip = fml_ip;
}

bool KS7Reader::isConnected()
{
    return clt.Connected;
}

