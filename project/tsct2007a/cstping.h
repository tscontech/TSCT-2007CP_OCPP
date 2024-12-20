#ifndef __PING_H__ 
#define __PING_H__ 
 
 
/** 
5  * PING_USE_SOCKETS: Set to 1 to use sockets, otherwise the raw api is used 
6  */ 
#ifndef PING_USE_SOCKETS 
#define PING_USE_SOCKETS    LWIP_SOCKET 
#endif 
 
 
 
 
 void ping_init(void); 
 
 void ping_set_target(const char* ip);
#if !PING_USE_SOCKETS 
void ping_send_now(void); 
#endif /* !PING_USE_SOCKETS */ 
 
 
#endif /* __PING_H__ */ 
