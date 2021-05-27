/*
   SYN sender by icingfire
   ------------------------
   g++ -s synsender.cpp -o synsender
   g++ -s --static synsender.cpp -o synsender

   ./binary  source_ips  target_ips  ports 
   ./binary  10.10.10.20,10.10.10.80-82  192.168.0.2-100,192.168.1.110,192.168.3-200  22,443,8080,10000-10010
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <vector>
#include <string>
#include <set>

using namespace std;


#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;



/*************
 * IP common *
 *************/

 /* Protocol versions: */

#define IP_VER4           0x04
#define IP_VER6           0x06

/* IP-level ECN: */

#define IP_TOS_CE         0x01    /* Congestion encountered          */
#define IP_TOS_ECT        0x02    /* ECN supported                   */

/* Encapsulated protocols we care about: */

#define PROTO_TCP         0x06


/********
 * IPv4 *
 ********/

struct ipv4_hdr {

    u8  ver_hlen;          /* IP version (4), IP hdr len in dwords (4) */
    u8  tos_ecn;           /* ToS field (6), ECN flags (2)             */
    u16 tot_len;           /* Total packet length, in bytes            */
    u16 id;                /* IP ID                                    */
    u16 flags_off;         /* Flags (3), fragment offset (13)          */
    u8  ttl;               /* Time to live                             */
    u8  proto;             /* Next protocol                            */
    u16 cksum;             /* Header checksum                          */
    u8  src[4];            /* Source IP                                */
    u8  dst[4];            /* Destination IP                           */

    /* Dword-aligned options may follow. */

} __attribute__((packed));

/* IP flags: */

#define IP4_MBZ           0x8000  /* "Must be zero"                  */
#define IP4_DF            0x4000  /* Don't fragment (usually PMTUD)  */
#define IP4_MF            0x2000  /* More fragments coming           */


/********
 * IPv6 *
 ********/

struct ipv6_hdr {

    u32 ver_tos;           /* Version (4), ToS (6), ECN (2), flow (20) */
    u16 pay_len;           /* Total payload length, in bytes           */
    u8  proto;             /* Next protocol                            */
    u8  ttl;               /* Time to live                             */
    u8  src[16];           /* Source IP                                */
    u8  dst[16];           /* Destination IP                           */

    /* Dword-aligned options may follow if proto != PROTO_TCP and are
       included in total_length; but we won't be seeing such traffic due
       to BPF rules. */

} __attribute__((packed));



/*******
 * TCP *
 *******/

struct tcp_hdr {

    u16 sport;             /* Source port                              */
    u16 dport;             /* Destination port                         */
    u32 seq;               /* Sequence number                          */
    u32 ack;               /* Acknowledgment number                    */
    u8  doff_rsvd;         /* Data off dwords (4), rsvd (3), ECN (1)   */
    u8  flags;             /* Flags, including ECN                     */
    u16 win;               /* Window size                              */
    u16 cksum;             /* Header and payload checksum              */
    u16 urg;               /* "Urgent" pointer                         */

    /* Dword-aligned options may follow. */

} __attribute__((packed));


/* Normal flags: */

#define TCP_FIN           0x01
#define TCP_SYN           0x02
#define TCP_RST           0x04
#define TCP_PUSH          0x08
#define TCP_ACK           0x10
#define TCP_URG           0x20

/* ECN stuff: */

#define TCP_ECE           0x40    /* ECN supported (SYN) or detected */
#define TCP_CWR           0x80    /* ECE acknowledgment              */
#define TCP_NS_RES        0x01    /* ECE notification via TCP        */

/* Notable options: */

#define TCPOPT_EOL        0       /* End of options (1)              */
#define TCPOPT_NOP        1       /* No-op (1)                       */
#define TCPOPT_MAXSEG     2       /* Maximum segment size (4)        */
#define TCPOPT_WSCALE     3       /* Window scaling (3)              */
#define TCPOPT_SACKOK     4       /* Selective ACK permitted (2)     */
#define TCPOPT_SACK       5       /* Actual selective ACK (10-34)    */
#define TCPOPT_TSTAMP     8       /* Timestamp (10)                  */


/***************
 * Other stuff *
 ***************/

#define MIN_TCP4 (sizeof(struct ipv4_hdr) + sizeof(struct tcp_hdr))
#define MIN_TCP6 (sizeof(struct ipv6_hdr) + sizeof(struct tcp_hdr))



// ----------------------------------- code start -----------------------------

vector<string> g_s_ips;
vector<string> g_d_ips;
vector<int> g_ports;


/* Do a basic IPv4 TCP checksum. */
static void tcp_cksum(u8* src, u8* dst, struct tcp_hdr* t, u8 opt_len) {

    u32 sum, i;
    u8* p;

    if (opt_len % 4) printf("Packet size not aligned to 4.\n");

    t->cksum = 0;

    sum = PROTO_TCP + sizeof(struct tcp_hdr) + opt_len;

    p = (u8*)t;

    for (i = 0; i < sizeof(struct tcp_hdr) + opt_len; i += 2, p += 2)
        sum += (*p << 8) + p[1];

    p = src;

    for (i = 0; i < 4; i += 2, p += 2) sum += (*p << 8) + p[1];

    p = dst;

    for (i = 0; i < 4; i += 2, p += 2) sum += (*p << 8) + p[1];

    t->cksum = htons(~(sum + (sum >> 16)));

}


/* Parse IPv4 address into a buffer. */
static int parse_addr(const char* str, u8* ret) {

    u32 a1, a2, a3, a4;

    if (sscanf(str, "%u.%u.%u.%u", &a1, &a2, &a3, &a4) != 4) {
        printf("Malformed IPv4 address.\n");
        return 1;
    }
        
    if (a1 > 255 || a2 > 255 || a3 > 255 || a4 > 255) {
        printf("Malformed IPv4 address.\n");
        return 1;
    }

    ret[0] = a1;
    ret[1] = a2;
    ret[2] = a3;
    ret[3] = a4;

    return 0;
}


int send_syn(const char* src_ip, const char* dst_ip, int dst_port) {

    static struct sockaddr_in sin;
    char one = 1;
    s32  sock;
    u32  i;

    static u8 work_buf[MIN_TCP4];

    struct ipv4_hdr* ip4 = (struct ipv4_hdr*)work_buf;
    struct tcp_hdr* tcp = (struct tcp_hdr*)(ip4 + 1);

    int ret1 = parse_addr(src_ip, ip4->src);
    int ret2 = parse_addr(dst_ip, ip4->dst);
    if (ret1 + ret2 > 0) {
        printf("malformed ips %s : %s", src_ip, dst_ip);
        return 1;
    }

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    if (sock < 0) printf("Can't open raw socket (you need to be root).");

    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char*)&one, sizeof(char)))
        printf("setsockopt() on raw socket failed.");

    sin.sin_family = PF_INET;

    memcpy(&sin.sin_addr.s_addr, ip4->dst, 4);

    ip4->ver_hlen = 0x45;
    ip4->tot_len = htons(MIN_TCP4);
    ip4->ttl = 192;
    ip4->proto = PROTO_TCP;

    tcp->dport = htons(dst_port);
    tcp->seq = htonl((rand() % 0x12345678) + 0x10000000);
    tcp->doff_rsvd = ((sizeof(struct tcp_hdr)) / 4) << 4;
    tcp->flags = TCP_SYN;
    tcp->win = htons(1024);

    tcp->sport = htons((rand() % 40000) + 20000);

    tcp_cksum(ip4->src, ip4->dst, tcp, 0);

    if (sendto(sock, work_buf, sizeof(work_buf), 0, (struct sockaddr*)&sin,
        sizeof(struct sockaddr_in)) < 0) printf("sendto() fails.");

    //printf("send syn  %s : %d --> %s : %d\n", src_ip, ntohs(tcp->sport), dst_ip, dst_port);

    return 0;
}


void dump(vector<string> * in_vec)
{
    for (int i = 0; i < in_vec->size(); ++i) {
        printf("%s\n", (*in_vec)[i].c_str());
    }
}

void dump_ports()
{
    for (int i = 0; i < g_ports.size(); ++i) {
        printf("%d\n", g_ports[i]);
    }
}


// ip formats
// 1.1.1.1,1.2.2.2,1.3.3.3
// 1.1.1.1-20,2,2,2,2
int parse_ips(char* in_str, int type)
{
    u32 a1, a2, a3, a4, a5;
    vector<string> * t_ips = type ? (&g_d_ips) : (&g_s_ips);

    int start_pos = 0, i=0;
    for (;;++i) {
        if (in_str[i] == 0)
            break;
        if (in_str[i] == ',') {
            in_str[i] = 0;
            t_ips->push_back(string(&in_str[start_pos]));
            start_pos = i + 1;
        }
    }
    if (start_pos < i)
        t_ips->push_back(string(&in_str[start_pos]));

    for (i = 0; i< t_ips->size(); ++i) {
        if (sscanf((*t_ips)[i].c_str(), "%u.%u.%u.%u-%u", &a1, &a2, &a3, &a4, &a5) == 5) {
            //printf("%u.%u.%u.%u-%u\n", a1, a2, a3, a4, a5);
            t_ips->erase(t_ips->begin() + i);
            if (a5 < a4)
                break;
            --i;

            char tmp_ip[32];
            for (u32 j = a4; j <= a5; j++) {
                snprintf(tmp_ip, 32, "%u.%u.%u.%u", a1, a2, a3, j);
                //printf("get ip: %s\n", tmp_ip);
                t_ips->push_back(string(tmp_ip));
            }
        }
    }

    //printf("get ips: \n");
    //dump(t_ips);

    return 0;
}


int parse_ports(char* in_str)
{
    int start_pos = 0, i = 0, t_port = 0;
    u32 a1, a2;
    vector<string> t_str_ports;
    set<int> t_ports;
    for (;; ++i) {
        if (in_str[i] == 0)
            break;
        if (in_str[i] == ',') {
            in_str[i] = 0;
            t_str_ports.push_back(string(&in_str[start_pos]));
            start_pos = i + 1;
        }
    }
    if (start_pos < i)
        t_str_ports.push_back(string(&in_str[start_pos]));

    for (i = 0; i < t_str_ports.size(); ++i) {
        if (sscanf(t_str_ports[i].c_str(), "%u-%u", &a1, &a2) == 2) {
            for (u32 j = a1; j <= a2; j++) {
                t_ports.insert(j);
            }
        }
        else if (t_port = atoi(t_str_ports[i].c_str())) {
            t_ports.insert(t_port);
        }
    }

    std::copy(t_ports.begin(), t_ports.end(), std::back_inserter(g_ports));
    //printf("get ports: \n");
    //dump_ports();
}


int main(int argc, char** argv) {

    if (argc != 4) {
        printf("Usage: %s src_ips dst_ips ports\n", argv[0]);
        exit(1);
    }

    parse_ips(argv[1], 0);
    parse_ips(argv[2], 1);
    parse_ports(argv[3]);

    for (int i = 0; i < g_ports.size(); ++i) {
        for (int j = 0; j < g_s_ips.size(); ++j) {
            for (int k = 0; k < g_d_ips.size(); ++k) {
                send_syn(g_s_ips[j].c_str(), g_d_ips[k].c_str(), g_ports[i]);
            }
        }
    }

    return 0;
}
