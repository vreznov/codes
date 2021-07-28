using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.Sockets;  //使用socket
using System.Net;
using System.IO;

namespace apq2
{
    /// <summary>
    /// @file: KModbusTcpMaster.cs
    /// @brief: 提供Modbus Tcp读写功能，作为主站，向从站读写数据。
    ///     注意：modbus的master = socket的client，是主动发起通信的一方。
    ///     如果因为通信收发影响UI，那么将本类的通信方法在线程内调用，使用状态机。
    ///     
    ///     目前仅仅实现了 0x03 0x10 功能码
    /// @author: kare
    /// @date: 2020.09.10
    /// </summary>
    public class KModbusTcpMaster
    {
        #region 通信操作及相关对象
        private string _ip = "192.168.1.10";
        private uint _port = 502;
        private byte _slaveAdress = 1;  // 要读写的从站地址
        private bool _connected = false;  //连接状态
        private byte[] _receiveBuffer = new byte[512];  //接收数据缓存

        public string Ip
        {
            get
            {
                return _ip;
            }

            set
            {
                _ip = value;
            }
        }

        public uint Port
        {
            get
            {
                return _port;
            }

            set
            {
                _port = value;
            }
        }

        public byte SlaveAdress
        {
            get
            {
                return _slaveAdress;
            }

            set
            {
                _slaveAdress = value;
            }
        }

        public bool Connected
        {
            get
            {
                return _connected;
            }

            set
            {
                _connected = value;
            }
        }

        /// <summary>
        /// 通信socket
        /// </summary>
        protected Socket master;

        public bool connect2device()
        {
            return connect2device(_ip, _port, _slaveAdress);
        }
        public bool connect2device(string fml_ip, uint fml_port, byte fml_slaveAdress = 1)
        {
            if(master != null && master.Connected)  // 判断是否处于连接状态 。 if会先判断第一个条件，第一个满足才判断第二个。
            {
                disconnectFromDevice();
            }

            IPAddress ipa;
            if (!IPAddress.TryParse(fml_ip, out ipa)) return false;

            _ip = fml_ip;
            _port = fml_port;
            _slaveAdress = fml_slaveAdress;

            master = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            try
            {
                master.Connect(ipa, (int)_port);
                _connected = master.Connected;
                return true;
            }
            catch (Exception ex)
            {
                Console.Write("[KModbusTcpMaster.connect2device] error: ");
                Console.WriteLine(ex);
            }

            return false;
        }

        public void disconnectFromDevice()
        {
            if(master != null)
            {
                master.Close();
                _connected = master.Connected;
            }
        }

        /// <summary>
        /// 读写数据
        /// </summary>
        public virtual void req()
        {

        }

        /// <summary>
        /// 生成读取报文
        /// 注意：不检查参数数组的长度，需要自己保证正确
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <param name="fml_cmd">modbus命令</param>
        /// <returns></returns>
        protected byte[] genarateReadCommand(ushort fml_startAdress, ushort fml_len, byte fml_cmd)
        {
            byte[] adress_byt = BitConverter.GetBytes(fml_startAdress);
            byte[] len_byt = BitConverter.GetBytes(fml_len);
            byte[] com_byt = new byte[12];
            com_byt[4] = 0;
            com_byt[5] = 6;
            com_byt[6] = _slaveAdress;
            com_byt[7] = fml_cmd;
            com_byt[8] = adress_byt[1];
            com_byt[9] = adress_byt[0];
            com_byt[10] = len_byt[1];
            com_byt[11] = len_byt[0];

            return com_byt;
        }

        /// <summary>
        /// 生成写入报文，需要自己将数据内容转换为 byte[]
        /// 注意：不检查参数数组的长度，需要自己保证正确
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <param name="fml_cmd"></param>
        /// <param name="fml_val"></param>
        /// <returns></returns>
        protected byte[] genarateWriteCommand(ushort fml_startAdress, ushort fml_len, byte fml_cmd, byte[] fml_val)
        {
            byte[] adress_byt = BitConverter.GetBytes(fml_startAdress);
            byte[] len_byt = BitConverter.GetBytes(fml_len);
            int ret_len;
            if (fml_cmd == 0x10) ret_len = 13 + fml_len * 2;
            else if (fml_cmd == 0x0f) ret_len = 13 + Convert.ToInt32(Math.Ceiling((double)fml_len / 8));
            else return null;
            byte[] com_byt = new byte[ret_len];
            byte[] cmd_tailLen = BitConverter.GetBytes(fml_len * 2 + 7);
            com_byt[4] = cmd_tailLen[1];
            com_byt[5] = cmd_tailLen[0];
            com_byt[6] = _slaveAdress;
            com_byt[7] = fml_cmd;
            com_byt[8] = adress_byt[1];
            com_byt[9] = adress_byt[0];
            com_byt[10] = len_byt[1];
            com_byt[11] = len_byt[0];
            com_byt[12] = (byte)(fml_len * 2);
            Array.Copy(fml_val, 0, com_byt, 13, fml_val.Length);  //拷贝内容区域

            return com_byt;

        }

        /// <summary>
        /// 读取保持寄存器
        /// fc 0x03
        /// 
        /// 示例
        /// 起始地址是0x0000，寄存器数量是 0x0003   00 01 00 00 00 06 01 03 00 00 00 03
        /// 数据长度为0x06，第一个寄存器的数据为0x21，其余为0x00    00 01 00 00 00 09 01 03 06 00 21 00 00 00 00
        /// </summary>
        /// <param name="fml_startAdress">起始地址</param>
        /// <param name="fml_len">数据长度，字的数量</param>
        /// <param name="ret">读取的结果</param>
        /// <returns></returns>
        protected bool read_holdRegister(ushort fml_startAdress, ushort fml_len, ref ushort[] ret)
        {
            byte[] com_byt = genarateReadCommand(fml_startAdress, fml_len, 0x03);
            try
            {
                master.Send(com_byt);
                System.Threading.Thread.Sleep(20);  //等待20ms接收
                int rcv_len = master.Receive(_receiveBuffer);
                int ret_len = (rcv_len - 9) / 2;  //减掉头部数据9字节，除以2获取字数
                for(int i=0; i<ret_len; i++)  
                {
                    byte[] temp = new byte[2];
                    // i * 2 + 9  -->  数据开始的位置
                    temp[1] = _receiveBuffer[i * 2 + 9];
                    temp[0] = _receiveBuffer[i * 2 + 9 + 1];
                    //ret[i] = BitConverter.ToUInt16(_receiveBuffer, i * 2 + 9);
                    ret[i] = BitConverter.ToUInt16(temp, 0);
                }
                return true;
            }
            catch (SocketException ex)
            {
                Console.WriteLine(string.Format("[KModbusTcpMaster.read_holdRegister] {0}", ex.Message));
                throw ex;
            }
            return false;
        }
        /// <summary>
        /// 读取指定寄存器
        /// fc 0x03
        /// </summary>
        /// <param name="fml_adress"></param>
        /// <param name="ret">读取结果</param>
        /// <returns></returns>
        protected bool read_holdRegister(ushort fml_adress, ref ushort ret)
        {
            ushort[] val = new ushort[1];
            bool ok = read_holdRegister(fml_adress, 1, ref val);
            if(ok)
            {
                ret = val[0];
                return true;
            }
            return false;
        }

        /// <summary>
        /// 写多个寄存器
        /// fc 0x10
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <param name="fml_val"></param>
        protected bool write_holdRegister(ushort fml_startAdress, ushort fml_len, ushort[] fml_val)
        {
            byte[] val_byt = array2bytes(fml_val);
            byte[] com_byt = genarateWriteCommand(fml_startAdress, fml_len, 0x10, val_byt);
            try
            {
                master.Send(com_byt);
                int rcv_len = master.Receive(_receiveBuffer, 12, SocketFlags.None);

                if (rcv_len != 12)  //正常写入会收到12个字节的反馈
                {
                    Console.WriteLine("[KModbusTcpMaster.write_holdRegister] receive bytes num not correct");
                }
                return true;
            }
            catch (SocketException ex)
            {
                throw ex;
            }
            return false;
        }

        protected bool write_holdRegister(ushort fml_adress, ushort fml_val)
        {
            return false;
        }

        /// <summary>
        /// 读取线圈状态
        /// fc 0x01
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <returns></returns>
        protected bool[] read_coil(ushort fml_startAdress, ushort fml_len)
        {

            return null;
        }

        /// <summary>
        /// 读线圈状态
        /// fc 0x01
        /// </summary>
        /// <param name="fml_adress"></param>
        /// <returns></returns>
        protected bool read_coil(ushort fml_adress)
        {
            return false;
        }

        /// <summary>
        /// 读取数字输入
        /// fc 0x02
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <returns></returns>
        protected bool[] read_di(ushort fml_startAdress, ushort fml_len)
        {

            return null;
        }

        /// <summary>
        /// 读取数字输入
        /// fc 0x02
        /// </summary>
        /// <param name="fml_adress"></param>
        /// <returns></returns>
        protected bool read_di(ushort fml_adress)
        {
            return false;
        }

        /// <summary>
        /// 写多个线圈
        /// fc 0x0f
        /// </summary>
        /// <param name="fml_startAdress"></param>
        /// <param name="fml_len"></param>
        /// <param name="fml_val"></param>
        private void write_coil(ushort fml_startAdress, ushort fml_len, bool[] fml_val)
        {
            
        }

        /// <summary>
        /// 写单个线圈
        /// 0x05
        /// </summary>
        /// <param name="fml_adress"></param>
        protected void write_coil(ushort fml_adress)
        {

        }
        #endregion

        #region 辅助函数

        /// <summary>
        /// 将其他数组转换为字节数组
        /// </summary>
        /// <param name="fml_src"></param>
        /// <param name="des"></param>
        /// <returns></returns>
        static byte[] array2bytes(ushort[] fml_src)
        {
            byte[] ret = new byte[fml_src.Length * 2];
            for(int i=0; i<fml_src.Length; i++)
            {
                byte[] temp = BitConverter.GetBytes(fml_src[i]);
                Array.Copy(temp, 0, ret, i * 2, 2);
            }

            return ret;
        }
        #endregion
    }
}
