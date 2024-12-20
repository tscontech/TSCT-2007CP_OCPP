# ifdef CFG_NET_LWIP_2

/**
 * @file
 * IP address API (common IPv4 and IPv6)
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_IP_ADDR_H
#define LWIP_HDR_IP_ADDR_H

#include "lwip/opt.h"
#include "lwip/def.h"

#include "lwip/ip4_addr.h"
#include "lwip/ip6_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup ipaddr
 * IP address types for use in ip_addr_t.type member.
 * @see tcp_new_ip_type(), udp_new_ip_type(), raw_new_ip_type().
 */
enum lwip_ip_addr_type {
  /** IPv4 */
  IPADDR_TYPE_V4 =   0U,
  /** IPv6 */
  IPADDR_TYPE_V6 =   6U,
  /** IPv4+IPv6 ("dual-stack") */
  IPADDR_TYPE_ANY = 46U
};

#if LWIP_IPV4 && LWIP_IPV6
/**
 * @ingroup ipaddr
 * A union struct for both IP version's addresses.
 * ATTENTION: watch out for its size when adding IPv6 address scope!
 */
typedef struct ip_addr {
  union {
    ip6_addr_t ip6;
    ip4_addr_t ip4;
  } u_addr;
  /** @ref lwip_ip_addr_type */
  u8_t type;
} ip_addr_t;

extern const ip_addr_t ip_addr_any_type;

/** @ingroup ip4addr */
#define IPADDR4_INIT(u32val)          { { { { u32val, 0ul, 0ul, 0ul } IPADDR6_ZONE_INIT } }, IPADDR_TYPE_V4 }
/** @ingroup ip4addr */
#define IPADDR4_INIT_BYTES(a,b,c,d)   IPADDR4_INIT(PP_HTONL(LWIP_MAKEU32(a,b,c,d)))

/** @ingroup ip6addr */
#define IPADDR6_INIT(a, b, c, d)      { { { { a, b, c, d } IPADDR6_ZONE_INIT } }, IPADDR_TYPE_V6 }
/** @ingroup ip6addr */
#define IPADDR6_INIT_HOST(a, b, c, d) { { { { PP_HTONL(a), PP_HTONL(b), PP_HTONL(c), PP_HTONL(d) } IPADDR6_ZONE_INIT } }, IPADDR_TYPE_V6 }

/** @ingroup ipaddr */
#define IP_IS_ANY_TYPE_VAL(ipaddr)    (IP_GET_TYPE(&ipaddr) == IPADDR_TYPE_ANY)
/** @ingroup ipaddr */
#define IPADDR_ANY_TYPE_INIT          { { { { 0ul, 0ul, 0ul, 0ul } IPADDR6_ZONE_INIT } }, IPADDR_TYPE_ANY }

/** @ingroup ip4addr */
#define IP_IS_V4_VAL(ipaddr)          (IP_GET_TYPE(&ipaddr) == IPADDR_TYPE_V4)
/** @ingroup ip6addr */
#define IP_IS_V6_VAL(ipaddr)          (IP_GET_TYPE(&ipaddr) == IPADDR_TYPE_V6)
/** @ingroup ip4addr */
#define IP_IS_V4(ipaddr)              (((ipaddr) == NULL) || IP_IS_V4_VAL(*(ipaddr)))
/** @ingroup ip6addr */
#define IP_IS_V6(ipaddr)              (((ipaddr) != NULL) && IP_IS_V6_VAL(*(ipaddr)))

#define IP_SET_TYPE_VAL(ipaddr, iptype) do { (ipaddr).type = (iptype); }while(0)
#define IP_SET_TYPE(ipaddr, iptype)     do { if((ipaddr) != NULL) { IP_SET_TYPE_VAL(*(ipaddr), iptype); }}while(0)
#define IP_GET_TYPE(ipaddr)           ((ipaddr)->type)

#define IP_ADDR_RAW_SIZE(ipaddr)      (IP_GET_TYPE(&ipaddr) == IPADDR_TYPE_V4 ? sizeof(ip4_addr_t) : sizeof(ip6_addr_t))

#define IP_ADDR_PCB_VERSION_MATCH_EXACT(pcb, ipaddr) (IP_GET_TYPE(&pcb->local_ip) == IP_GET_TYPE(ipaddr))
#define IP_ADDR_PCB_VERSION_MATCH(pcb, ipaddr) (IP_IS_ANY_TYPE_VAL(pcb->local_ip) || IP_ADDR_PCB_VERSION_MATCH_EXACT(pcb, ipaddr))

/** @ingroup ip6addr
 * Convert generic ip address to specific protocol version
 */
#define ip_2_ip6(ipaddr)   (&((ipaddr)->u_addr.ip6))
/** @ingroup ip4addr
 * Convert generic ip address to specific protocol version
 */
#define ip_2_ip4(ipaddr)   (&((ipaddr)->u_addr.ip4))

/** @ingroup ip4addr */
#define IP_ADDR4(ipaddr,a,b,c,d)      do { IP4_ADDR(ip_2_ip4(ipaddr),a,b,c,d); \
                                           IP_SET_TYPE_VAL(*(ipaddr), IPADDR_TYPE_V4); } while(0)
/** @ingroup ip6addr */
#define IP_ADDR6(ipaddr,i0,i1,i2,i3)  do { IP6_ADDR(ip_2_ip6(ipaddr),i0,i1,i2,i3); \
                                           IP_SET_TYPE_VAL(*(ipaddr), IPADDR_TYPE_V6); } while(0)
/** @ingroup ip6addr */
#define IP_ADDR6_HOST(ipaddr,i0,i1,i2,i3)  IP_ADDR6(ipaddr,PP_HTONL(i0),PP_HTONL(i1),PP_HTONL(i2),PP_HTONL(i3))

#define ip_clear_no4(ipaddr)  do { ip_2_ip6(ipaddr)->addr[1] = \
                                   ip_2_ip6(ipaddr)->addr[2] = \
                                   ip_2_ip6(ipaddr)->addr[3] = 0; \
                                   ip6_addr_clear_zone(ip_2_ip6(ipaddr)); }while(0)

/** @ingroup ipaddr */
#define ip_addr_copy(dest, src)      do{ IP_SET_TYPE_VAL(dest, IP_GET_TYPE(&src)); if(IP_IS_V6_VAL(src)){ \
  ip6_addr_copy(*ip_2_ip6(&(dest)), *ip_2_ip6(&(src))); }else{ \
  ip4_addr_copy(*ip_2_ip4(&(dest)), *ip_2_ip4(&(src))); ip_clear_no4(&dest); }}while(0)
/** @ingroup ip6addr */
#define ip_addr_copy_from_ip6(dest, src)      do{ \
  ip6_addr_copy(*ip_2_ip6(&(dest)), src); IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V6); }while(0)
/** @ingroup ip6addr */
#define ip_addr_copy_from_ip6_packed(dest, src)      do{ \
  ip6_addr_copy_from_packed(*ip_2_ip6(&(dest)), src); IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V6); }while(0)
/** @ingroup ip4addr */
#define ip_addr_copy_from_ip4(dest, src)      do{ \
  ip4_addr_copy(*ip_2_ip4(&(dest)), src); IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V4); ip_clear_no4(&dest); }while(0)
/** @ingroup ip4addr */
#define ip_addr_set_ip4_u32(ipaddr, val)  do{if(ipaddr){ip4_addr_set_u32(ip_2_ip4(ipaddr), val); \
  IP_SET_TYPE(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(ipaddr); }}while(0)
/** @ingroup ip4addr */
#define ip_addr_set_ip4_u32_val(ipaddr, val)  do{ ip4_addr_set_u32(ip_2_ip4(&(ipaddr)), val); \
  IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(&ipaddr); }while(0)
/** @ingroup ip4addr */
#define ip_addr_get_ip4_u32(ipaddr)  (((ipaddr) && IP_IS_V4(ipaddr)) ? \
  ip4_addr_get_u32(ip_2_ip4(ipaddr)) : 0)
/** @ingroup ipaddr */
#define ip_addr_set(dest, src) do{ IP_SET_TYPE(dest, IP_GET_TYPE(src)); if(IP_IS_V6(src)){ \
  ip6_addr_set(ip_2_ip6(dest), ip_2_ip6(src)); }else{ \
  ip4_addr_set(ip_2_ip4(dest), ip_2_ip4(src)); ip_clear_no4(dest); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_set_ipaddr(dest, src) ip_addr_set(dest, src)
/** @ingroup ipaddr */
#define ip_addr_set_zero(ipaddr)     do{ \
  ip6_addr_set_zero(ip_2_ip6(ipaddr)); IP_SET_TYPE(ipaddr, 0); }while(0)
/** @ingroup ip5addr */
#define ip_addr_set_zero_ip4(ipaddr)     do{ \
  ip6_addr_set_zero(ip_2_ip6(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V4); }while(0)
/** @ingroup ip6addr */
#define ip_addr_set_zero_ip6(ipaddr)     do{ \
  ip6_addr_set_zero(ip_2_ip6(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V6); }while(0)
/** @ingroup ipaddr */
#define ip_addr_set_any(is_ipv6, ipaddr)      do{if(is_ipv6){ \
  ip6_addr_set_any(ip_2_ip6(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V6); }else{ \
  ip4_addr_set_any(ip_2_ip4(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(ipaddr); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_set_any_val(is_ipv6, ipaddr)      do{if(is_ipv6){ \
  ip6_addr_set_any(ip_2_ip6(&(ipaddr))); IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V6); }else{ \
  ip4_addr_set_any(ip_2_ip4(&(ipaddr))); IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(&ipaddr); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_set_loopback(is_ipv6, ipaddr) do{if(is_ipv6){ \
  ip6_addr_set_loopback(ip_2_ip6(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V6); }else{ \
  ip4_addr_set_loopback(ip_2_ip4(ipaddr)); IP_SET_TYPE(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(ipaddr); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_set_loopback_val(is_ipv6, ipaddr) do{if(is_ipv6){ \
  ip6_addr_set_loopback(ip_2_ip6(&(ipaddr))); IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V6); }else{ \
  ip4_addr_set_loopback(ip_2_ip4(&(ipaddr))); IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4); ip_clear_no4(&ipaddr); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_set_hton(dest, src)  do{if(IP_IS_V6(src)){ \
  ip6_addr_set_hton(ip_2_ip6(dest), ip_2_ip6(src)); IP_SET_TYPE(dest, IPADDR_TYPE_V6); }else{ \
  ip4_addr_set_hton(ip_2_ip4(dest), ip_2_ip4(src)); IP_SET_TYPE(dest, IPADDR_TYPE_V4); ip_clear_no4(ipaddr); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_get_network(target, host, netmask) do{if(IP_IS_V6(host)){ \
  ip4_addr_set_zero(ip_2_ip4(target)); IP_SET_TYPE(target, IPADDR_TYPE_V6); } else { \
  ip4_addr_get_network(ip_2_ip4(target), ip_2_ip4(host), ip_2_ip4(netmask)); IP_SET_TYPE(target, IPADDR_TYPE_V4); }}while(0)
/** @ingroup ipaddr */
#define ip_addr_netcmp(addr1, addr2, mask) ((IP_IS_V6(addr1) && IP_IS_V6(addr2)) ? \
  0 : \
  ip4_addr_netcmp(ip_2_ip4(addr1), ip_2_ip4(addr2), mask))
/** @ingroup ipaddr */
#define ip_addr_cmp(addr1, addr2)    ((IP_GET_TYPE(addr1) != IP_GET_TYPE(addr2)) ? 0 : (IP_IS_V6_VAL(*(addr1)) ? \
  ip6_addr_cmp(ip_2_ip6(addr1), ip_2_ip6(addr2)) : \
  ip4_addr_cmp(ip_2_ip4(addr1), ip_2_ip4(addr2))))
/** @ingroup ipaddr */
#define ip_addr_cmp_zoneless(addr1, addr2)    ((IP_GET_TYPE(addr1) != IP_GET_TYPE(addr2)) ? 0 : (IP_IS_V6_VAL(*(addr1)) ? \
  ip6_addr_cmp_zoneless(ip_2_ip6(addr1), ip_2_ip6(addr2)) : \
  ip4_addr_cmp(ip_2_ip4(addr1), ip_2_ip4(addr2))))
/** @ingroup ipaddr */
#define ip_addr_isany(ipaddr)        (((ipaddr) == NULL) ? 1 : ((IP_IS_V6(ipaddr)) ? \
  ip6_addr_isany(ip_2_ip6(ipaddr)) : \
  ip4_addr_isany(ip_2_ip4(ipaddr))))
/** @ingroup ipaddr */
#define ip_addr_isany_val(ipaddr)        ((IP_IS_V6_VAL(ipaddr)) ? \
  ip6_addr_isany_val(*ip_2_ip6(&(ipaddr))) : \
  ip4_addr_isany_val(*ip_2_ip4(&(ipaddr))))
/** @ingroup ipaddr */
#define ip_addr_isbroadcast(ipaddr, netif) ((IP_IS_V6(ipaddr)) ? \
  0 : \
  ip4_addr_isbroadcast(ip_2_ip4(ipaddr), netif))
/** @ingroup ipaddr */
#define ip_addr_ismulticast(ipaddr)  ((IP_IS_V6(ipaddr)) ? \
  ip6_addr_ismulticast(ip_2_ip6(ipaddr)) : \
  ip4_addr_ismulticast(ip_2_ip4(ipaddr)))
/** @ingroup ipaddr */
#define ip_addr_isloopback(ipaddr)  ((IP_IS_V6(ipaddr)) ? \
  ip6_addr_isloopback(ip_2_ip6(ipaddr)) : \
  ip4_addr_isloopback(ip_2_ip4(ipaddr)))
/** @ingroup ipaddr */
#define ip_addr_islinklocal(ipaddr)  ((IP_IS_V6(ipaddr)) ? \
  ip6_addr_islinklocal(ip_2_ip6(ipaddr)) : \
  ip4_addr_islinklocal(ip_2_ip4(ipaddr)))
#define ip_addr_debug_print(debug, ipaddr) do { if(IP_IS_V6(ipaddr)) { \
  ip6_addr_debug_print(debug, ip_2_ip6(ipaddr)); } else { \
  ip4_addr_debug_print(debug, ip_2_ip4(ipaddr)); }}while(0)
#define ip_addr_debug_print_val(debug, ipaddr) do { if(IP_IS_V6_VAL(ipaddr)) { \
  ip6_addr_debug_print_val(debug, *ip_2_ip6(&(ipaddr))); } else { \
  ip4_addr_debug_print_val(debug, *ip_2_ip4(&(ipaddr))); }}while(0)
char *ipaddr_ntoa(const ip_addr_t *addr);
char *ipaddr_ntoa_r(const ip_addr_t *addr, char *buf, int buflen);
int ipaddr_aton(const char *cp, ip_addr_t *addr);

/** @ingroup ipaddr */
#define IPADDR_STRLEN_MAX   IP6ADDR_STRLEN_MAX

/** @ingroup ipaddr */
#define ip4_2_ipv4_mapped_ipv6(ip6addr, ip4addr) do { \
  (ip6addr)->addr[3] = (ip4addr)->addr; \
  (ip6addr)->addr[2] = PP_HTONL(0x0000FFFFUL); \
  (ip6addr)->addr[1] = 0; \
  (ip6addr)->addr[0] = 0; \
  ip6_addr_clear_zone(ip6addr); } while(0);

/** @ingroup ipaddr */
#define unmap_ipv4_mapped_ipv6(ip4addr, ip6addr) \
  (ip4addr)->addr = (ip6addr)->addr[3];

#define IP46_ADDR_ANY(type) (((type) == IPADDR_TYPE_V6)? IP6_ADDR_ANY : IP4_ADDR_ANY)

#else /* LWIP_IPV4 && LWIP_IPV6 */

#define IP_ADDR_PCB_VERSION_MATCH(addr, pcb)          1
#define IP_ADDR_PCB_VERSION_MATCH_EXACT(pcb, ipaddr)  1

#define ip_addr_set_any_val(is_ipv6, ipaddr)          ip_addr_set_any(is_ipv6, &(ipaddr))
#define ip_addr_set_loopback_val(is_ipv6, ipaddr)     ip_addr_set_loopback(is_ipv6, &(ipaddr))

#if LWIP_IPV4

typedef ip4_addr_t ip_addr_t;
#define IPADDR4_INIT(u32val)                    { u32val }
#define IPADDR4_INIT_BYTES(a,b,c,d)             IPADDR4_INIT(PP_HTONL(LWIP_MAKEU32(a,b,c,d)))
#define IP_IS_V4_VAL(ipaddr)                    1
#define IP_IS_V6_VAL(ipaddr)                    0
#define IP_IS_V4(ipaddr)                        1
#define IP_IS_V6(ipaddr)                        0
#define IP_IS_ANY_TYPE_VAL(ipaddr)              0
#define IP_SET_TYPE_VAL(ipaddr, iptype)
#define IP_SET_TYPE(ipaddr, iptype)
#define IP_GET_TYPE(ipaddr)                     IPADDR_TYPE_V4
#define IP_ADDR_RAW_SIZE(ipaddr)                sizeof(ip4_addr_t)
#define ip_2_ip4(ipaddr)                        (ipaddr)
#define IP_ADDR4(ipaddr,a,b,c,d)                IP4_ADDR(ipaddr,a,b,c,d)

#define ip_addr_copy(dest, src)                 ip4_addr_copy(dest, src)
#define ip_addr_copy_from_ip4(dest, src)        ip4_addr_copy(dest, src)
#define ip_addr_set_ip4_u32(ipaddr, val)        ip4_addr_set_u32(ip_2_ip4(ipaddr), val)
#define ip_addr_set_ip4_u32_val(ipaddr, val)    ip_addr_set_ip4_u32(&(ipaddr), val)
#define ip_addr_get_ip4_u32(ipaddr)             ip4_addr_get_u32(ip_2_ip4(ipaddr))
#define ip_addr_set(dest, src)                  ip4_addr_set(dest, src)
#define ip_addr_set_ipaddr(dest, src)           ip4_addr_set(dest, src)
#define ip_addr_set_zero(ipaddr)                ip4_addr_set_zero(ipaddr)
#define ip_addr_set_zero_ip4(ipaddr)            ip4_addr_set_zero(ipaddr)
#define ip_addr_set_any(is_ipv6, ipaddr)        ip4_addr_set_any(ipaddr)
#define ip_addr_set_loopback(is_ipv6, ipaddr)   ip4_addr_set_loopback(ipaddr)
#define ip_addr_set_hton(dest, src)             ip4_addr_set_hton(dest, src)
#define ip_addr_get_network(target, host, mask) ip4_addr_get_network(target, host, mask)
#define ip_addr_netcmp(addr1, addr2, mask)      ip4_addr_netcmp(addr1, addr2, mask)
#define ip_addr_cmp(addr1, addr2)               ip4_addr_cmp(addr1, addr2)
#define ip_addr_isany(ipaddr)                   ip4_addr_isany(ipaddr)
#define ip_addr_isany_val(ipaddr)               ip4_addr_isany_val(ipaddr)
#define ip_addr_isloopback(ipaddr)              ip4_addr_isloopback(ipaddr)
#define ip_addr_islinklocal(ipaddr)             ip4_addr_islinklocal(ipaddr)
#define ip_addr_isbroadcast(addr, netif)        ip4_addr_isbroadcast(addr, netif)
#define ip_addr_ismulticast(ipaddr)             ip4_addr_ismulticast(ipaddr)
#define ip_addr_debug_print(debug, ipaddr)      ip4_addr_debug_print(debug, ipaddr)
#define ip_addr_debug_print_val(debug, ipaddr)  ip4_addr_debug_print_val(debug, ipaddr)
#define ipaddr_ntoa(ipaddr)                     ip4addr_ntoa(ipaddr)
#define ipaddr_ntoa_r(ipaddr, buf, buflen)      ip4addr_ntoa_r(ipaddr, buf, buflen)
#define ipaddr_aton(cp, addr)                   ip4addr_aton(cp, addr)

#define IPADDR_STRLEN_MAX   IP4ADDR_STRLEN_MAX

#define IP46_ADDR_ANY(type) (IP4_ADDR_ANY)

#else /* LWIP_IPV4 */

typedef ip6_addr_t ip_addr_t;
#define IPADDR6_INIT(a, b, c, d)                { { a, b, c, d } IPADDR6_ZONE_INIT }
#define IPADDR6_INIT_HOST(a, b, c, d)           { { PP_HTONL(a), PP_HTONL(b), PP_HTONL(c), PP_HTONL(d) } IPADDR6_ZONE_INIT }
#define IP_IS_V4_VAL(ipaddr)                    0
#define IP_IS_V6_VAL(ipaddr)                    1
#define IP_IS_V4(ipaddr)                        0
#define IP_IS_V6(ipaddr)                        1
#define IP_IS_ANY_TYPE_VAL(ipaddr)              0
#define IP_SET_TYPE_VAL(ipaddr, iptype)
#define IP_SET_TYPE(ipaddr, iptype)
#define IP_GET_TYPE(ipaddr)                     IPADDR_TYPE_V6
#define IP_ADDR_RAW_SIZE(ipaddr)                sizeof(ip6_addr_t)
#define ip_2_ip6(ipaddr)                        (ipaddr)
#define IP_ADDR6(ipaddr,i0,i1,i2,i3)            IP6_ADDR(ipaddr,i0,i1,i2,i3)
#define IP_ADDR6_HOST(ipaddr,i0,i1,i2,i3)       IP_ADDR6(ipaddr,PP_HTONL(i0),PP_HTONL(i1),PP_HTONL(i2),PP_HTONL(i3))

#define ip_addr_copy(dest, src)                 ip6_addr_copy(dest, src)
#define ip_addr_copy_from_ip6(dest, src)        ip6_addr_copy(dest, src)
#define ip_addr_copy_from_ip6_packed(dest, src) ip6_addr_copy_from_packed(dest, src)
#define ip_addr_set(dest, src)                  ip6_addr_set(dest, src)
#define ip_addr_set_ipaddr(dest, src)           ip6_addr_set(dest, src)
#define ip_addr_set_zero(ipaddr)                ip6_addr_set_zero(ipaddr)
#define ip_addr_set_zero_ip6(ipaddr)            ip6_addr_set_zero(ipaddr)
#define ip_addr_set_any(is_ipv6, ipaddr)        ip6_addr_set_any(ipaddr)
#define ip_addr_set_loopback(is_ipv6, ipaddr)   ip6_addr_set_loopback(ipaddr)
#define ip_addr_set_hton(dest, src)             ip6_addr_set_hton(dest, src)
#define ip_addr_get_network(target, host, mask) ip6_addr_set_zero(target)
#define ip_addr_netcmp(addr1, addr2, mask)      0
#define ip_addr_cmp(addr1, addr2)               ip6_addr_cmp(addr1, addr2)
#define ip_addr_cmp_zoneless(addr1, addr2)      ip6_addr_cmp_zoneless(addr1, addr2)
#define ip_addr_isany(ipaddr)                   ip6_addr_isany(ipaddr)
#define ip_addr_isany_val(ipaddr)               ip6_addr_isany_val(ipaddr)
#define ip_addr_isloopback(ipaddr)              ip6_addr_isloopback(ipaddr)
#define ip_addr_islinklocal(ipaddr)             ip6_addr_islinklocal(ipaddr)
#define ip_addr_isbroadcast(addr, netif)        0
#define ip_addr_ismulticast(ipaddr)             ip6_addr_ismulticast(ipaddr)
#define ip_addr_debug_print(debug, ipaddr)      ip6_addr_debug_print(debug, ipaddr)
#define ip_addr_debug_print_val(debug, ipaddr)  ip6_addr_debug_print_val(debug, ipaddr)
#define ipaddr_ntoa(ipaddr)                     ip6addr_ntoa(ipaddr)
#define ipaddr_ntoa_r(ipaddr, buf, buflen)      ip6addr_ntoa_r(ipaddr, buf, buflen)
#define ipaddr_aton(cp, addr)                   ip6addr_aton(cp, addr)

#define IPADDR_STRLEN_MAX   IP6ADDR_STRLEN_MAX

#define IP46_ADDR_ANY(type) (IP6_ADDR_ANY)

#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_IPV4

extern const ip_addr_t ip_addr_any;
extern const ip_addr_t ip_addr_broadcast;

/**
 * @ingroup ip4addr
 * Can be used as a fixed/const ip_addr_t
 * for the IP wildcard.
 * Defined to @ref IP4_ADDR_ANY when IPv4 is enabled.
 * Defined to @ref IP6_ADDR_ANY in IPv6 only systems.
 * Use this if you can handle IPv4 _AND_ IPv6 addresses.
 * Use @ref IP4_ADDR_ANY or @ref IP6_ADDR_ANY when the IP
 * type matters.
 */
#define IP_ADDR_ANY         IP4_ADDR_ANY
/**
 * @ingroup ip4addr
 * Can be used as a fixed/const ip_addr_t
 * for the IPv4 wildcard and the broadcast address
 */
#define IP4_ADDR_ANY        (&ip_addr_any)
/**
 * @ingroup ip4addr
 * Can be used as a fixed/const ip4_addr_t
 * for the wildcard and the broadcast address
 */
#define IP4_ADDR_ANY4       (ip_2_ip4(&ip_addr_any))

/** @ingroup ip4addr */
#define IP_ADDR_BROADCAST   (&ip_addr_broadcast)
/** @ingroup ip4addr */
#define IP4_ADDR_BROADCAST  (ip_2_ip4(&ip_addr_broadcast))

#endif /* LWIP_IPV4*/

#if LWIP_IPV6

extern const ip_addr_t ip6_addr_any;

/** 
 * @ingroup ip6addr
 * IP6_ADDR_ANY can be used as a fixed ip_addr_t
 * for the IPv6 wildcard address
 */
#define IP6_ADDR_ANY   (&ip6_addr_any)
/**
 * @ingroup ip6addr
 * IP6_ADDR_ANY6 can be used as a fixed ip6_addr_t
 * for the IPv6 wildcard address
 */
#define IP6_ADDR_ANY6  (ip_2_ip6(&ip6_addr_any))

#if !LWIP_IPV4
/** IPv6-only configurations */
#define IP_ADDR_ANY IP6_ADDR_ANY
#endif /* !LWIP_IPV4 */

#endif

#if LWIP_IPV4 && LWIP_IPV6
/** @ingroup ipaddr */
#define IP_ANY_TYPE    (&ip_addr_any_type)
#else
#define IP_ANY_TYPE    IP_ADDR_ANY
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_IP_ADDR_H */

# else

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __LWIP_IP_ADDR_H__
#define __LWIP_IP_ADDR_H__

#include "lwip/opt.h"
#include "lwip/def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the aligned version of ip_addr_t,
   used as local variable, on the stack, etc. */
#if LWIP_PACKET
struct ip_addr {
  u32_t addr;
	  u32_t addr1;
	  u32_t addr2;
	  u32_t addr3;
	  u16_t proto;
};

	struct ip_addr_r {
	  u32_t addr;
	};
#else	
	struct ip_addr {
	  u32_t addr;
	};
#endif

/* This is the packed version of ip_addr_t,
   used in network headers that are itself packed */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct ip_addr_packed {
  PACK_STRUCT_FIELD(u32_t addr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** ip_addr_t uses a struct for convenience only, so that the same defines can
 * operate both on ip_addr_t as well as on ip_addr_p_t. */
typedef struct ip_addr ip_addr_t;
typedef struct ip_addr_packed ip_addr_p_t;
#if LWIP_PACKET
typedef struct ip_addr_r ip_addr_r_t;
#endif

/*
 * struct ipaddr2 is used in the definition of the ARP packet format in
 * order to support compilers that don't have structure packing.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct ip_addr2 {
  PACK_STRUCT_FIELD(u16_t addrw[2]);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* Forward declaration to not include netif.h */
struct netif;

extern const ip_addr_t ip_addr_any;
extern const ip_addr_t ip_addr_broadcast;

/** IP_ADDR_ can be used as a fixed IP address
 *  for the wildcard and the broadcast address
 */
#define IP_ADDR_ANY         ((ip_addr_t *)&ip_addr_any)
#define IP_ADDR_BROADCAST   ((ip_addr_t *)&ip_addr_broadcast)

/** 255.255.255.255 */
#define IPADDR_NONE         ((u32_t)0xffffffffUL)
/** 127.0.0.1 */
#define IPADDR_LOOPBACK     ((u32_t)0x7f000001UL)
/** 0.0.0.0 */
#define IPADDR_ANY          ((u32_t)0x00000000UL)
/** 255.255.255.255 */
#define IPADDR_BROADCAST    ((u32_t)0xffffffffUL)

/* Definitions of the bits in an Internet address integer.

   On subnets, host and network parts are found according to
   the subnet mask, not these masks.  */
#define IP_CLASSA(a)        ((((u32_t)(a)) & 0x80000000UL) == 0)
#define IP_CLASSA_NET       0xff000000
#define IP_CLASSA_NSHIFT    24
#define IP_CLASSA_HOST      (0xffffffff & ~IP_CLASSA_NET)
#define IP_CLASSA_MAX       128

#define IP_CLASSB(a)        ((((u32_t)(a)) & 0xc0000000UL) == 0x80000000UL)
#define IP_CLASSB_NET       0xffff0000
#define IP_CLASSB_NSHIFT    16
#define IP_CLASSB_HOST      (0xffffffff & ~IP_CLASSB_NET)
#define IP_CLASSB_MAX       65536

#define IP_CLASSC(a)        ((((u32_t)(a)) & 0xe0000000UL) == 0xc0000000UL)
#define IP_CLASSC_NET       0xffffff00
#define IP_CLASSC_NSHIFT    8
#define IP_CLASSC_HOST      (0xffffffff & ~IP_CLASSC_NET)

#define IP_CLASSD(a)        (((u32_t)(a) & 0xf0000000UL) == 0xe0000000UL)
#define IP_CLASSD_NET       0xf0000000          /* These ones aren't really */
#define IP_CLASSD_NSHIFT    28                  /*   net and host fields, but */
#define IP_CLASSD_HOST      0x0fffffff          /*   routing needn't know. */
#define IP_MULTICAST(a)     IP_CLASSD(a)

//#define IP_EXPERIMENTAL(a)  (((u32_t)(a) & 0xf0000000UL) == 0xf0000000UL)
#define IP_EXPERIMENTAL(a)  (((u32_t)(a) & 0x000000f0UL) == 0x000000f0UL) //Due to assist ip_canforward()
#define IP_BADCLASS(a)      (((u32_t)(a) & 0xf0000000UL) == 0xf0000000UL)

#define IP_LOOPBACKNET      127                 /* official! */

#if LWIP_PACKET
/* Conversion Macros to create fake ip_addr containing all the info
 * of sockaddr_ll structures (but sll_protocol).
 * This simplify the implementation as all the internal structures
 * can be reused */

///#include <netpacket/packet.h>
///#include "lwip/packet.h"
#if 1
#define SALL2IPADDR(S,I) ({ \
	(I).addr = (u32_t) ((S).sll_ifindex);\
  (I).addr1 = (u32_t) (((S).sll_hatype << 16) + ((S).sll_pkttype << 8) + \
												 (S).sll_halen);\
	(I).addr2 = (u32_t) (((S).sll_addr[0] << 24) + ((S).sll_addr[1] << 16) + \
												 ((S).sll_addr[2] << 8) + (S).sll_addr[3]); \
	(I).addr3 = (u32_t) (((S).sll_addr[4] << 24) + ((S).sll_addr[5] << 16) + \
												 ((S).sll_addr[6] << 8) + (S).sll_addr[7]); \
})

#define IPADDR2SALL(I,S) ({ \
		register u32_t tmp; \
		(S).sll_ifindex = (int) ((I).addr);\
		(S).sll_hatype = ((I).addr1 >> 16); \
		(S).sll_pkttype = ((I).addr1 >> 8 & 0xff); \
		(S).sll_halen = ((I).addr1 & 0xff); \
		tmp = (I).addr2; \
		(S).sll_addr[3] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[2] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[1] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[0] = tmp & 0xff; \
		tmp = (I).addr3; \
		(S).sll_addr[7] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[6] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[5] = tmp & 0xff; tmp >>= 8; \
		(S).sll_addr[4] = tmp & 0xff; \
})

#define IPSADDR_IFINDEX(I) ((I).addr)
#else
#define IPADDR2SALL(I,S)
#define SALL2IPADDR(S,I)
#define IPSADDR_IFINDEX(I) ((I).addr)
#endif
#endif /* LWIP_PACKET */



#if BYTE_ORDER == BIG_ENDIAN
/** Set an IP address given by the four byte-parts */
#define IP4_ADDR(ipaddr, a,b,c,d) \
        (ipaddr)->addr = ((u32_t)((a) & 0xff) << 24) | \
                         ((u32_t)((b) & 0xff) << 16) | \
                         ((u32_t)((c) & 0xff) << 8)  | \
                          (u32_t)((d) & 0xff)
#else
/** Set an IP address given by the four byte-parts.
    Little-endian version that prevents the use of htonl. */
#define IP4_ADDR(ipaddr, a,b,c,d) \
        (ipaddr)->addr = ((u32_t)((d) & 0xff) << 24) | \
                         ((u32_t)((c) & 0xff) << 16) | \
                         ((u32_t)((b) & 0xff) << 8)  | \
                          (u32_t)((a) & 0xff)
#endif

/** MEMCPY-like copying of IP addresses where addresses are known to be
 * 16-bit-aligned if the port is correctly configured (so a port could define
 * this to copying 2 u16_t's) - no NULL-pointer-checking needed. */
#ifndef IPADDR2_COPY
#define IPADDR2_COPY(dest, src) SMEMCPY(dest, src, sizeof(ip_addr_t))
#define IPADDR2_COPY_2(dest, src) SMEMCPY(dest, src, 4) //for WIFI
#endif

/** Copy IP address - faster than ip_addr_set: no NULL check */
#define ip_addr_copy(dest, src) ((dest).addr = (src).addr)
/** Safely copy one IP address to another (src may be NULL) */
#define ip_addr_set(dest, src) ((dest)->addr = \
                                    ((src) == NULL ? 0 : \
                                    (src)->addr))
/** Set complete address to zero */
#define ip_addr_set_zero(ipaddr)      ((ipaddr)->addr = 0)
/** Set address to IPADDR_ANY (no need for htonl()) */
#define ip_addr_set_any(ipaddr)       ((ipaddr)->addr = IPADDR_ANY)
/** Set address to loopback address */
#define ip_addr_set_loopback(ipaddr)  ((ipaddr)->addr = PP_HTONL(IPADDR_LOOPBACK))
/** Safely copy one IP address to another and change byte order
 * from host- to network-order. */
#define ip_addr_set_hton(dest, src) ((dest)->addr = \
                               ((src) == NULL ? 0:\
                               htonl((src)->addr)))
/** IPv4 only: set the IP address given as an u32_t */
#define ip4_addr_set_u32(dest_ipaddr, src_u32) ((dest_ipaddr)->addr = (src_u32))
/** IPv4 only: get the IP address as an u32_t */
#define ip4_addr_get_u32(src_ipaddr) ((src_ipaddr)->addr)

/** Get the network address by combining host address with netmask */
#define ip_addr_get_network(target, host, netmask) ((target)->addr = ((host)->addr) & ((netmask)->addr))

/**
 * Determine if two address are on the same network.
 *
 * @arg addr1 IP address 1
 * @arg addr2 IP address 2
 * @arg mask network identifier mask
 * @return !0 if the network identifiers of both address match
 */
#define ip_addr_netcmp(addr1, addr2, mask) (((addr1)->addr & \
                                              (mask)->addr) == \
                                             ((addr2)->addr & \
                                              (mask)->addr))
#define ip_addr_cmp(addr1, addr2) ((addr1)->addr == (addr2)->addr)

#define ip_addr_isany(addr1) ((addr1) == NULL || (addr1)->addr == IPADDR_ANY)

#define ip_addr_isbroadcast(ipaddr, netif) ip4_addr_isbroadcast((ipaddr)->addr, (netif))
u8_t ip4_addr_isbroadcast(u32_t addr, const struct netif *netif);

#define ip_addr_netmask_valid(netmask) ip4_addr_netmask_valid((netmask)->addr)
u8_t ip4_addr_netmask_valid(u32_t netmask);

#define ip_addr_ismulticast(addr1) (((addr1)->addr & PP_HTONL(0xf0000000UL)) == PP_HTONL(0xe0000000UL))

#define ip_addr_islinklocal(addr1) (((addr1)->addr & PP_HTONL(0xffff0000UL)) == PP_HTONL(0xa9fe0000UL))

#define ip_addr_debug_print(debug, ipaddr) \
  LWIP_DEBUGF(debug, ("%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F,             \
                      ipaddr != NULL ? ip4_addr1_16(ipaddr) : 0,       \
                      ipaddr != NULL ? ip4_addr2_16(ipaddr) : 0,       \
                      ipaddr != NULL ? ip4_addr3_16(ipaddr) : 0,       \
                      ipaddr != NULL ? ip4_addr4_16(ipaddr) : 0))

/* Get one byte from the 4-byte address */
#define ip4_addr1(ipaddr) (((u8_t*)(ipaddr))[0])
#define ip4_addr2(ipaddr) (((u8_t*)(ipaddr))[1])
#define ip4_addr3(ipaddr) (((u8_t*)(ipaddr))[2])
#define ip4_addr4(ipaddr) (((u8_t*)(ipaddr))[3])
/* These are cast to u16_t, with the intent that they are often arguments
 * to printf using the U16_F format from cc.h. */
#define ip4_addr1_16(ipaddr) ((u16_t)ip4_addr1(ipaddr))
#define ip4_addr2_16(ipaddr) ((u16_t)ip4_addr2(ipaddr))
#define ip4_addr3_16(ipaddr) ((u16_t)ip4_addr3(ipaddr))
#define ip4_addr4_16(ipaddr) ((u16_t)ip4_addr4(ipaddr))

/** For backwards compatibility */
#define ip_ntoa(ipaddr)  ipaddr_ntoa(ipaddr)

u32_t ipaddr_addr(const char *cp);
int ipaddr_aton(const char *cp, ip_addr_t *addr);
/** returns ptr to static buffer; not reentrant! */
char *ipaddr_ntoa(const ip_addr_t *addr);
char *ipaddr_ntoa_r(const ip_addr_t *addr, char *buf, int buflen);

#ifdef __cplusplus
}
#endif

#endif /* __LWIP_IP_ADDR_H__ */

# endif