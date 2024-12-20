#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "libxml/xpathInternals.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include "onvif/onvif.h"

//=============================================================================
//                              Function Declaration
//=============================================================================

static int getXmlValue(xmlDocPtr doc, xmlChar *xpath, char *buf, int buf_length) 
{
    xmlChar *keyword = NULL;
    xmlXPathContextPtr context = NULL;
    xmlXPathObjectPtr result = NULL;
    
    // new a xpath context
    context = xmlXPathNewContext(doc);    
    if (context == NULL) {
        return -1;
    }
    
    // register namespace to xpath
    xmlXPathRegisterNs(context, BAD_CAST "s", BAD_CAST "http://www.w3.org/2003/05/soap-envelope");
    xmlXPathRegisterNs(context, BAD_CAST "trt", BAD_CAST "http://www.onvif.org/ver10/media/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "tt", BAD_CAST "http://www.onvif.org/ver10/schema");
    xmlXPathRegisterNs(context, BAD_CAST "tds", BAD_CAST "http://www.onvif.org/ver10/device/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "ter", BAD_CAST "http://www.onvif.org/ver10/error");
    xmlXPathRegisterNs(context, BAD_CAST "timg", BAD_CAST "http://www.onvif.org/ver20/imaging/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "wsa5", BAD_CAST "http://www.w3.org/2005/08/addressing");
    xmlXPathRegisterNs(context, BAD_CAST "wsnt", BAD_CAST "http://docs.oasis-open.org/wsn/b-2");
    xmlXPathRegisterNs(context, BAD_CAST "d", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlXPathRegisterNs(context, BAD_CAST "a", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing");

    // use xpath to find node
    result = xmlXPathEvalExpression(xpath, context);

    // free xpath context
    xmlXPathFreeContext(context);
    
    if (result == NULL) {
        // element no found
        DEBUG_PRINT("[%s] Not found Xpath(%s)\n", __FUNCTION__, xpath);
        return -2;
    }

    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        if ((strcmp((char*) xpath, "//s:Body//s:Fault//s:Code//s:Subcode//s:Value") != 0) && (strcmp((char*) xpath, "//s:Body//s:Fault//s:Reason//s:Text") != 0)) {
            DEBUG_PRINT("[%s] Node should be exist\n", __FUNCTION__);
        }
        DEBUG_PRINT("[%s] ERROR: xmlXPathNodeSetIsEmpty\n", __FUNCTION__);
        return -3;
    }

    if (result) {
        // get node's value
        keyword = xmlNodeListGetString(doc, result->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
        if (keyword != NULL) {
            if (strlen((char*) keyword) > buf_length-1) {
                DEBUG_PRINT("[%s] ERROR: Overflow risk\n", __FUNCTION__);
                xmlXPathFreeObject(result);
                free(keyword);
                return -4;
            }
            else {
                strcpy(buf, (char*) keyword);
            }
        }
    }

    xmlXPathFreeObject(result);
    if (keyword != NULL)
        free(keyword);
    
    return 0;
}

static void getScopeField(char *scope, char *field_name, char *value) 
{
    char *field = NULL, *mark = NULL, *result = NULL;
    char field_contents[1024] = {0};
    int length = 0;    

    // ex: scope="onvif://www.onvif.org/location/country/Taiwan onvif://www.onvif.org/name/ITE"
    field = strstr(scope, field_name);
    if (field != NULL) {
        field = field + strlen(field_name);

        // ex: field="onvif://www.onvif.org/name/ITE onvif://www.onvif.org/hardware/9862"
        mark = strstr(field, " ");
        if (mark != NULL) {
            length = mark - field;
            strncpy(field_contents, field, length);
        }
        else {
            strcpy(field_contents, field);
        }

        length = strlen(field_contents);
        int offset = 0;
        int j;
        for (int i = 0; i < length; i++) {
            j = i - offset;
            if (field_contents[i] == '%') {
                // ex: field_contents=%87
                char middle[3] = {0};
                i++; 
                offset++;
                middle[0] = field_contents[i];
                i++; 
                offset++;
                middle[1] = field_contents[i];
                char *ptr;
                int result = strtol(middle, &ptr, 16);
                value[j] = result;
            }
            else {
                // ex: field_contents=ITE
                value[j] = field_contents[i];
            }
        }
        value[length] = '\0';
    }
}


static void getCameraName(int index, onvifConn *onvif_session, onvifData *onvif_data) 
{
    xmlDocPtr xml_input = xmlParseMemory(onvif_session->buf[index], onvif_session->buf_len[index]);
    char scopes[8192] = {0};
    char temp_mf[1024] = {0};
    char temp_hd[1024] = {0};

    memset(scopes, 0, 8192);    
    getXmlValue(xml_input, "//s:Body//d:ProbeMatches//d:ProbeMatch//d:Scopes", scopes, 8192);

    getScopeField(scopes, "onvif://www.onvif.org/name/", temp_mf);
    getScopeField(scopes, "onvif://www.onvif.org/hardware/", temp_hd);

    // found manufacturer
    if (strlen(temp_mf) > 0) {
        strcat(onvif_data->camera_name, temp_mf);
    }

    // found IPcam module number
    if (strlen(temp_hd) > 0) {
        if (strstr(temp_mf, temp_hd) == NULL) {
            strcat(onvif_data->camera_name, " ");
            strcat(onvif_data->camera_name, temp_hd);
        }
    }

    // Not found Camera name
    if (strlen(onvif_data->camera_name)  == 0)
        strcpy(onvif_data->camera_name, "UNKNOWN CAMERA");

    xmlFreeDoc(xml_input);
}

static void getXAddrs(int index, onvifConn *onvif_session, onvifData *onvif_data) 
{
    xmlDocPtr xml_input = xmlParseMemory(onvif_session->buf[index], onvif_session->buf_len[index]);
    
    if (getXmlValue(xml_input, BAD_CAST "//s:Body//d:ProbeMatches//d:ProbeMatch//d:XAddrs", onvif_data->xaddrs, 1024) == 0) {
        char *sub = strstr(onvif_data->xaddrs, " ");
        if (sub != NULL) {
            int mark = sub - onvif_data->xaddrs;
            onvif_data->xaddrs[mark] = '\0';
        }
    }
    
    xmlFreeDoc(xml_input);
}


static void GetUuid(char *uuid)
{
    srand(time(NULL));
    strcpy(uuid, "urn:uuid:");
    
    for (int i = 0; i < 16; i++) {
        char buf[3];
        sprintf(buf, "%02x", (unsigned char) rand());
        strcat(uuid, buf);
        
        if (i == 3 || i == 5 || i == 7 || i == 9)
            strcat(uuid, "-");
    }
}

static void GetDiscoveryXmlv1(char *buffer, int buf_size, char *uuid)
{
    // makeup WS-Discovery
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "Envelope", NULL);
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, BAD_CAST "xmlns:SOAP-ENV", BAD_CAST "http://www.w3.org/2003/05/soap-envelope");
    xmlNewProp(root, BAD_CAST "xmlns:a", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing");
    xmlNsPtr ns_soap = xmlNewNs(root, NULL, BAD_CAST "SOAP-ENV");
    xmlNsPtr ns_a = xmlNewNs(root, NULL, BAD_CAST "a");
    xmlSetNs(root, ns_soap);

    // makupu SOAP Header
    xmlNodePtr header = xmlNewTextChild(root, ns_soap, BAD_CAST "Header", NULL);
    xmlNodePtr action = xmlNewTextChild(header, ns_a, BAD_CAST "Action", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe");
    xmlNewProp(action, BAD_CAST "SOAP-ENV:mustUnderstand", BAD_CAST "1");
    xmlNodePtr messageid = xmlNewTextChild(header, ns_a, BAD_CAST "MessageID", BAD_CAST uuid);
    xmlNodePtr replyto = xmlNewTextChild(header, ns_a, BAD_CAST "ReplyTo", NULL);
    xmlNodePtr address = xmlNewTextChild(replyto, ns_a, BAD_CAST "Address", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous");
    xmlNodePtr to = xmlNewTextChild(header, ns_a, BAD_CAST "To", BAD_CAST "urn:schemas-xmlsoap-org:ws:2005:04:discovery");
    xmlNewProp(to, BAD_CAST "SOAP-ENV:mustUnderstand", BAD_CAST "1");

    // makeup SOAP Boby and add Probe message
    xmlNodePtr body = xmlNewTextChild(root, ns_soap, BAD_CAST "Body", NULL);
    xmlNodePtr probe = xmlNewTextChild(body, NULL, BAD_CAST "Probe", NULL);
    xmlNewProp(probe, BAD_CAST "xmlns:p", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlNsPtr ns_p = xmlNewNs(probe, NULL, BAD_CAST "p");
    xmlSetNs(probe, ns_p);
    xmlNodePtr types = xmlNewTextChild(probe, NULL, BAD_CAST "Types", BAD_CAST "dp0:NetworkVideoTransmitter");
    xmlNewProp(types, BAD_CAST "xmlns:d", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlNewProp(types, BAD_CAST "xmlns:dp0", BAD_CAST "http://www.onvif.org/ver10/network/wsdl");
    xmlNsPtr ns_d = xmlNewNs(types, NULL, BAD_CAST "d");
    xmlSetNs(types, ns_d);

    // xml format transfer to buffer
    xmlOutputBufferPtr outputbuffer = xmlAllocOutputBuffer(NULL);
    xmlNodeDumpOutput(outputbuffer, doc, root, 0, 0, NULL);
    int size = xmlOutputBufferGetSize(outputbuffer);
    strcpy(buffer, (char*)xmlOutputBufferGetContent(outputbuffer));
    
    xmlOutputBufferFlush(outputbuffer);
    xmlOutputBufferClose(outputbuffer);
    xmlFreeDoc(doc);
}

static void GetDiscoveryXmlv2(char *buffer)
{
    char *xml_string = "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\"><s:Header><a:Action s:mustUnderstand=\"1\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action><a:MessageID>uuid:6bbdae2d-f229-42c8-a27b-93880fb80826</a:MessageID><a:ReplyTo><a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address></a:ReplyTo><a:To s:mustUnderstand=\"1\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To></s:Header><s:Body><Probe xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"><d:Types xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:dp0=\"http://www.onvif.org/ver10/device/wsdl\">dp0:Device</d:Types></Probe></s:Body></s:Envelope>";
    strcpy(buffer, xml_string);
}

void OnviInit(onvifConn *conn) 
{
    // init Onvif conn
    memset(conn, 0, sizeof(onvifConn));    

    // init xml parser
    xmlInitParser();
}

void OnvifDeinit(void)
{
    // stop xml parser
    xmlCleanupParser();
}

int OnvifDiscovery(onvifConn *conn) 
{
    int ret = 0;
    struct sockaddr_in onvif_address;
    int onvif_socket, loopback = 0;
    char onvif_msg[1024] = {0};
    int msg_size = 0;
    unsigned int address_size = 0;
    struct timeval tv;
    int keep_receive = 1;
    int cam_count = 0;

    // set discovery_type and Uuid
    conn->discovery_type = 1; //use default type=1
    GetUuid(conn->uuid);

    // get Onvif discovery payload
    memset(onvif_msg, 0, sizeof(char)*1024);
    if(conn->discovery_type == 1) {
        // Standard Onvif Format
        GetDiscoveryXmlv1(onvif_msg, 1024, conn->uuid);
        msg_size = strlen(onvif_msg);
    }
    else if(conn->discovery_type == 2) {
        // non-Standard Onvif Format
        GetDiscoveryXmlv2(onvif_msg);
        msg_size = strlen(onvif_msg);
    }
    else {
        DEBUG_PRINT("[%s] No support Discovery-type\n", __FUNCTION__);
        return -1;
    }

    // open multicast socket
    onvif_socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if(onvif_socket < 0) {
        DEBUG_PRINT("[%s] Onvif broadcast socket create failure\n", __FUNCTION__);
        return -1;
    }

    // set receive's timeout
    tv.tv_sec = 500;  //FIXME: tv_sec = 500 is 500 ms
    tv.tv_usec = 0;
    ret = lwip_setsockopt(onvif_socket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: setsockopt(SO_RCVTIMEO)\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // disable the loopback of outgoing multicast pkg
    ret = lwip_setsockopt(onvif_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: setsockopt(IP_MULTICAST_LOOP)\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // start to send onvif-discovery pkg
    memset(&onvif_address, 0, sizeof(struct sockaddr_in));
    onvif_address.sin_family = AF_INET;
    onvif_address.sin_port = htons(3702);
    onvif_address.sin_addr.s_addr = inet_addr("239.255.255.250");
    address_size = sizeof(onvif_address);
    ret = lwip_sendto(onvif_socket, onvif_msg, msg_size, 0, (struct sockaddr*)&onvif_address, address_size);
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: sendto failure\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // receive IPCam's response
    while(keep_receive) {
        conn->buf_len[cam_count] = lwip_recvfrom(onvif_socket, conn->buf[cam_count], sizeof(conn->buf[cam_count]), 0, (struct sockaddr*) &onvif_address, &address_size);
        DEBUG_PRINT("[%s] (%d)recvifrom ret=%d\n", __FUNCTION__, cam_count, conn->buf_len[cam_count]);
    
        if(conn->buf_len[cam_count] > 0) {
            // Get one IPCam's response
            cam_count++;
        }
        else {
            // Timeout or Error
            keep_receive = 0;
            if(conn->buf_len[cam_count] < 0) {
                //DEBUG_PRINT("[%s] ERROR: recvfrom failure(timeout)\n", __FUNCTION__);
                //lwip_close(broadcast_socket);
                //return -1;
            }
        }
    }

    // stop conn
    lwip_close(onvif_socket);

    return cam_count;
}

void OnvifDataParser(int camIdx, onvifConn *conn, onvifData *data)
{
    // reset onvifData
    memset(data, 0, sizeof(onvifData));

    // get IPCam name
    getCameraName(camIdx, conn, data);

    // get xAddrs
    getXAddrs(camIdx, conn, data);
}

int OnvifGetxAddrsNum(char *xAddrs)
{
    int ret = 0;
    int i = 0, xAddrsLen = 0;

    if(xAddrs == NULL) {
        DEBUG_PRINT("[%s] xAddrs is invaild\n", __FUNCTION__);
        return 0;
    }

    xAddrsLen = strlen(xAddrs);
    for(i = 0; i < xAddrsLen; i++) {
        if(xAddrs[i] == ' ') {
            // There are the another IP address in xAddrs list.
            ret++;
        }
    }

    return ret + 1;
}

void OnvifTransferAddr(int idx, char *ip, char *xAddrs)
{
    int count = 0, length = 0;
    char dupstr[1024] = {0};
    char *space = " ";
    char *substr = NULL, *saveptr = NULL;
    char *mark = NULL;
    int i;

    if(xAddrs == NULL) {
        DEBUG_PRINT("[%s] xAddrs is invaild\n", __FUNCTION__);
        return;
    }

    strcpy(dupstr, xAddrs);

    if(strstr(dupstr, space) != NULL) {
        // More than one IP address exist in xAddrs list
        
        substr = strtok_r(dupstr, space, &saveptr);
        do {
            // Find the idx IP address
            if(count == idx)
                break;
            
            substr = strtok_r(NULL, space, &saveptr);
            count++;
        } while(substr);

        DEBUG_PRINT("[%s] count=%d substr=%s\n", __FUNCTION__, count, substr);

        // parse "http://"
        substr = substr + 7;

        // ex: substr=192.168.1.1/device/service mark=/device/service
        mark = strstr(substr, "/");
        if(mark != NULL) {
            length = mark - substr;
            strncpy(ip, substr, length);
        }
        else {
            strcpy(ip, substr);
        }      
    }
    else {
        // The only one IP address exist in xAddrs list
        
        length = strlen(dupstr);

        // parse "http://"
        substr = strstr(dupstr, "//");
        substr = substr + 2;

        for(i = 0; i < length - (substr - dupstr); i++) {
            if(substr[i] == '/') {
                ip[i] = '\0';
                break;
            }
            else {
                ip[i] = substr[i];
            }
        }
    }

}


