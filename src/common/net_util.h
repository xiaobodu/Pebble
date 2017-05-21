/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */


#ifndef _PEBBLE_COMMON_NET_UTIL_H_
#define _PEBBLE_COMMON_NET_UTIL_H_

#include <list>
#include <string>
#include <sys/epoll.h>

namespace pebble {

int GetIpByIf(const char* if_name, std::string* ip);

//////////////////////////////////////////////////////////////////////////////////////

typedef uint64_t NetAddr;
static const NetAddr INVAILD_NETADDR = UINT64_MAX;
class NetIO;

#define NETADDR_IP_PRINT_FMT   "%u.%u.%u.%u:%u"
#define NETADDR_IP_PRINT_CTX(socket_info) \
    (socket_info->_ip & 0xFFU), ((socket_info->_ip >> 8) & 0xFFU), \
    ((socket_info->_ip >> 16) & 0xFFU), ((socket_info->_ip >> 24) & 0xFFU), \
    (((socket_info->_port & 0xFF) << 8) | ((socket_info->_port >> 8) & 0xFF))

uint64_t NetAddressToUIN(const std::string& ip, uint16_t port);

void UINToNetAddress(uint64_t net_uin, std::string* ip, uint16_t* port);

void SetLogOut(FILE* log_fp);

#define NET_LOG_DEBUG_LEVEL 0
#define NET_LOG_INFO_LEVEL 1
#define NET_LOG_ERROR_LEVEL 2
void SetLogLevel(int32_t log_level);

static const uint8_t TCP_PROTOCOL = 0x80;   ///< Protocol: tcpЭ��
static const uint8_t UDP_PROTOCOL = 0x40;   ///< Protocol: udpЭ��
static const uint8_t IN_BLOCKED = 0x10;     ///< ������
static const uint8_t ADDR_TYPE = 0x07;      ///< AddrType: ��ַ����
static const uint8_t CONNECT_ADDR = 0x04;   ///<           ��������
static const uint8_t LISTEN_ADDR = 0x02;    ///<           ����
static const uint8_t ACCEPT_ADDR = 0x01;    ///<           ��������

struct SocketInfo
{
    void Reset();

    uint8_t GetProtocol() const { return (_state & 0xC0); }
    uint8_t GetAddrType() const { return (_state & 0x7); }

    int32_t _socket_fd;
    uint32_t _addr_info;                    ///< _addr_info acceptʱ���ؼ����ĵ�ַ��Ϣ��TCP����������ʱΪ���Զ���������
    uint32_t _ip;
    uint16_t _port;
    uint8_t _state;
    uint8_t _uin;                           ///< ����ʱ�ı��
};

class Epoll
{
    friend class NetIO;
public:
    Epoll();
    ~Epoll();

    int32_t Init(uint32_t max_event);

    /// @brief �ȴ��¼�����
    /// @return >=0 �������¼���
    /// @return -1 ����ԭ���errno
    int32_t Wait(int32_t timeout_ms);

    int32_t AddFd(int32_t fd, uint32_t events, uint64_t data);

    int32_t DelFd(int32_t fd);

    int32_t ModFd(int32_t fd, uint32_t events, uint64_t data);

    /// @brief ��ȡ�¼�
    /// @return 0 ��ȡ�ɹ�
    /// @return -1 ��ȡʧ�ܣ�û���¼���Ϣ
    int32_t GetEvent(uint32_t *events, uint64_t *data);

    const char* GetLastError() const {
        return m_last_error;
    }

private:
    char    m_last_error[256];
    int32_t m_epoll_fd;
    int32_t m_max_event;
    int32_t m_event_num;
    struct epoll_event* m_events;
    NetIO*  m_bind_net_io;
};

/// @brief ����IO�����࣬����socket����װ�������
/// @note ʹ�õ�ַ����socket���ϲ㲻�ÿ�������
class NetIO
{
    friend class Epoll;
public:
    NetIO();
    ~NetIO();

    /// @brief ��ʼ��
    int32_t Init(Epoll* epoll);

    /// @brief �򿪼���
    /// @note ���������������쳣ʱ���Զ����Իָ�
    NetAddr Listen(const std::string& ip, uint16_t port);

    /// @brief ���ܷ�������
    /// @note �����򿪵������������쳣ʱ���Զ��ͷ�
    NetAddr Accept(NetAddr listen_addr);

    /// @brief ���ӵ�ַ
    /// @note ���������������쳣ʱ���Զ����Իָ�
    NetAddr ConnectPeer(const std::string& ip, uint16_t port);

    /// @brief ��������
    /// @note ֻ�����ͣ��������κη��Ͳ�������
    int32_t Send(NetAddr dst_addr, const char* data, uint32_t data_len);

    /// @brief ��������
    /// @note ֻ�����ͣ��������κη��Ͳ�������
    int32_t SendV(NetAddr dst_addr, uint32_t data_num,
                  const char* data[], uint32_t data_len[], uint32_t offset = 0);

    /// @brief ��������
    /// @note only for udp server send rsp
    int32_t SendTo(NetAddr local_addr, NetAddr remote_addr, const char* data, uint32_t data_len);

    /// @brief ��������
    /// @return -1 ��ȡʧ�ܻ����ӹر��ˣ������errno
    /// @return >= 0 ��ȡ�����ݳ��ȣ�����Ϊ��
    /// @note ֻ�����ͣ��������κν��յĲ�������
    int32_t Recv(NetAddr dst_addr, char* buff, uint32_t buff_len);

    /// @brief ��������
    /// @note only for udp listen
    int32_t RecvFrom(NetAddr local_addr, NetAddr* remote_addr, char* buff, uint32_t buff_len);

    /// @brief �ر�����
    /// @return -1 �رպ󷵻ش��󣬴���ԭ���errno
    /// @return 0  ���ӹر���
    /// @note ʧ�ܵ�����ͬclose
    int32_t Close(NetAddr dst_addr);

    /// @brief �ر���������
    void CloseAll();

    /// @brief ��ȡ��ַ��socket�����Ϣ
    /// @return ��NULL����ָ��
    /// @note ��ַ������ʱ���ص�Info������ȫΪ0
    const SocketInfo* GetSocketInfo(NetAddr dst_addr) const;

    /// @brief ��ȡ������ַ��socket�����Ϣ
    /// @return ��NULL����ָ��
    /// @note ��ַ�����ڻ��ַ���Ǳ������ӵĵ�ַʱ�����ص�Info������ȫΪ0
    /// @note ���ڱ������ӵĵ�ַ��ȡ�����ı��صļ�����ַ��Ϣ
    const SocketInfo* GetLocalListenSocketInfo(NetAddr dst_addr) const;

    /// @brief ��ȡ������ַ�ľ����Ϣ
    NetAddr GetLocalListenAddr(NetAddr dst_addr);

    const char* GetLastError() const {
        return m_last_error;
    }

    // ������
    static bool NON_BLOCK;              ///< NON_BLOCK �Ƿ�Ϊ��������д��Ĭ��Ϊtrue
    static bool ADDR_REUSE;             ///< ADDR_REUSE �Ƿ�򿪵�ַ���ã�Ĭ��Ϊtrue
    static bool KEEP_ALIVE;             ///< KEEP_ALIVE �Ƿ�����Ӷ�ʱ���Լ�⣬Ĭ��Ϊtrue
    static bool USE_NAGLE;              ///< USE_NAGLE �Ƿ�ʹ��nagle�㷨�ϲ�С����Ĭ��Ϊfalse
    static bool USE_LINGER;             ///< USE_LINGER �Ƿ�ʹ��linger��ʱ�ر����ӣ�Ĭ��Ϊfalse
    static int32_t LINGER_TIME;         ///< LINGER_TIME ʹ��lingetʱ��ʱʱ�����ã�Ĭ��Ϊ0
    static int32_t LISTEN_BACKLOG;      ///< LISTEN_BACKLOG ������backlog���г��ȣ�Ĭ��Ϊ10240
    static uint32_t MAX_SOCKET_NUM;     ///< MAX_SOCKET_NUM ������������Ĭ��Ϊ1000000
    static uint8_t AUTO_RECONNECT;      ///< AUTO_RECONNECT ��TCP�����������Զ�������Ĭ��ֵΪ3

    // ����
    static const uint32_t MAX_SENDV_DATA_NUM = 32;   ///< SendV�ӿ�����͵����ݶ�����

private:
    NetAddr AllocNetAddr();

    void FreeNetAddr(NetAddr net_addr);

    SocketInfo* RawGetSocketInfo(NetAddr net_addr);

    int32_t InitSocketInfo(const std::string& ip, uint16_t port, SocketInfo* socket_info);

    int32_t OnEvent(NetAddr net_addr, uint32_t events);

    int32_t RawListen(NetAddr net_addr, SocketInfo* socket_info);

    int32_t RawConnect(NetAddr net_addr, SocketInfo* socket_info);

    int32_t RawClose(SocketInfo* socket_info);

    char            m_last_error[256];

    Epoll           *m_epoll;

    NetAddr         m_used_id;
    SocketInfo      *m_sockets;
    std::list<NetAddr>  m_free_sockets;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_NET_UTIL_H_
