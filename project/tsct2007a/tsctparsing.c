#include <pthread.h>
#include "SDL/SDL.h"
#include "tsctwsclient.h"
#include "ocpp_cmd.h"

extern const char* cs_req_msg[];

static Parse_Objt(PAYLOAD* pld, uint8_t* index)
{

}

static Parse_Str(PAYLOAD* pld, uint8_t* index)
{

}

static bool Parse_Header(char* buf, MSG_STRUCT* msg, uint16_t* index, size_t rlen)
{
    uint8_t parsing_step = 0, temp_index = 0;
    char temp_buf[30];

    if(buf[0] != '['){
        printf("Parsing Data Error at Start\r\n");
        return true;
    }

    for(*index = 1; (*index)<rlen; (*index)++){
        if(parsing_step == 0){
            if(buf[*index] == '2' || buf[*index] == '3' || buf[*index] == '4'){
                msg->Msg_type = buf[*index];
                parsing_step = 1;
            }
            // else if(buf[*index] != ' ' || buf[*index] != '\n'){
            //     printf("[Parse_Header] Massege Type Error\r\n");
            //     return true;
            // }                
        }
        else if(parsing_step == 1){
            if(buf[*index] == '\"'){		
                parsing_step = 2; 
            }
        }
        
        else if(parsing_step == 2){			
            if(buf[*index] == '\"'){                
                msg->UniqueID = 0;
                if(msg->Msg_type == '2'){
                    memset(msg->UniqueID_Char, 0x00, sizeof(msg->UniqueID_Char));
                    memcpy(msg->UniqueID_Char, temp_buf, temp_index);
                    msg->Msg_type = 2;
                    parsing_step = 3;
                }
                else if(msg->Msg_type == '3'){
                    for(int i=0;i<temp_index;i++)
                    {
                        msg->UniqueID += (temp_buf[i]-'0') * (uint64_t)pow(10,temp_index-i-1);
                    }
                    msg->Msg_type = 3;
                    msg->Action_Code = Call_Tx_Msg.Action_Code;
                    return false;
                }
                else{
                    msg->Msg_type = 4;
                    return false;
                }
            }		
            else{
                // strcat(temp_buf, curl_ws_recv_buf[index]);
                temp_buf[temp_index] = buf[*index];
                temp_index++;
            }	
        }
        // Check Action
        else if(parsing_step == 3){
            if(buf[*index] == '\"'){
                memset(temp_buf, 0x00, sizeof(temp_buf));	
                temp_index = 0;
                parsing_step = 4;
            }		
            // else if(buf[*index] != ' '){
            //     printf("[Parse_Header] First\" Error\r\n")
            //     return true;
            // }     
        }
        else if(parsing_step == 4){
            if(buf[*index] == '\"'){
                printf("[Parse_Header] Action Code %s\r\n",temp_buf);
                for(int i=1;i<20;i++){
                    if(strcmp(temp_buf,cs_req_msg[i]) == 0) {

                        msg->Action_Code = i;
                        return false;
                    }
                }
                printf("[Parse_Header] ActionCode Error %s\r\n", temp_buf);
                return true;
            } 
            else{
                temp_buf[temp_index] = buf[*index];
                temp_index++;
            }
        }
    }
    printf("[Parse_Header] Over the Buffer size %llu\r\n", rlen);
    return true;
}

// void CheckDataType(uint8_t data_no)
// {
//     if(buf[*index] == '{')

// }

static bool Parse_Payload(char* buf, MSG_STRUCT* msg, uint16_t* index, size_t rlen)
{
    PAYLOAD* ptmpPayload;
    uint8_t parsing_step = 0, bObjDepth = 0, str_index = 0;
    uint8_t* tempIndx;
    bool bArrStartFlg = false;
    TYPE_CODE Pars_type[4] = {TYPE_CODE_OBJ, TYPE_CODE_OBJ, TYPE_CODE_OBJ, TYPE_CODE_OBJ};

    msg->Payload_len = 0;
    memset(msg->Payload,0x00,sizeof(msg->Payload));
    memset(sub_Payload1,0x00,sizeof(sub_Payload1));
    memset(sub_Payload2,0x00,sizeof(sub_Payload2));
    memset(sub_Payload3,0x00,sizeof(sub_Payload3));

    ptmpPayload = msg->Payload;
    tempIndx = &(msg->Payload_len);    

    for((*index)++;(*index)<rlen && (bArrStartFlg || buf[*index] != ']'); (*index)++){
        if(parsing_step == 0){
            if(buf[*index] == '{'){
                Pars_type[0] = TYPE_CODE_OBJ;
                parsing_step = 1;
            } 		    
            // else if(buf[*index] != ' ' && buf[*index] != ','){
            //     printf("[Parse_Payload] Find Error Char[%c] Before {\r\n", buf[*index]);
            //     return true;
            // }     
        }
		else if(parsing_step == 1){
            str_index = 0;
			if(buf[*index] == '}') {
				if(bObjDepth == 0)	break;
				else{
					--bObjDepth;
					if(bObjDepth == 0){
						ptmpPayload = (msg->Payload);
						tempIndx = &(msg->Payload_len);
					}					
					if(bObjDepth == 1){
						ptmpPayload = (sub_Payload1);
						tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
					}
					else if(bObjDepth == 2){
						ptmpPayload = (sub_Payload2);
						tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
					}

                    if(Pars_type[bObjDepth] == TYPE_CODE_OBJ) parsing_step = 1;
                    else if(Pars_type[bObjDepth] == TYPE_CODE_ARR){
                        parsing_step = 4;
                        str_index = 0;
                    }
				}				
			}
			else{
				if(buf[*index] == '\"') { 
                    parsing_step = 2;
                    memset((ptmpPayload + (*tempIndx))->property_name, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_name));
                    memset((ptmpPayload + (*tempIndx))->property_contants, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_contants));
                }
			}
		}
		// Check Payload OBJ Name
		else if(parsing_step == 2){
			if(buf[*index] == '\"') 	{
                parsing_step = 3;
                // printf("[Parse_Payload] Property Name : %s\r\n",(ptmpPayload + (*tempIndx))->property_name);
            }
			else{
                (ptmpPayload+(*tempIndx))->property_name[str_index] = buf[*index];
                str_index++;
			}
		}
        else if(parsing_step == 3){
			if(buf[*index] == ':'){
				parsing_step = 4;
				str_index = 0;
			}
		}
		else if(parsing_step == 4){
			// for Data Type String
			if(buf[*index] == '\"')	 		parsing_step = 5; 	
			// for Data Type Integer
            else if(buf[*index] == '}') {
                if(bObjDepth == 0)	break;
                else{
                    --bObjDepth;
					if(bObjDepth == 0){
						ptmpPayload = (msg->Payload);
						tempIndx = &(msg->Payload_len);
                        // *tempIndx = 0;
					}					
					if(bObjDepth == 1){
						ptmpPayload = (sub_Payload1);
						tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
                        // *tempIndx = 0;
					}
					else if(bObjDepth == 2){
						ptmpPayload = (sub_Payload2);
						tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
                        // *tempIndx = 0;
					}

                    if(Pars_type[bObjDepth] == TYPE_CODE_OBJ)        parsing_step = 1;
                    else if(Pars_type[bObjDepth] == TYPE_CODE_ARR){   parsing_step = 4; str_index = 0;}
                }				
            }
            else if(buf[*index] == ']') {
                if(bObjDepth == 0)	break;
                else{
                    --bObjDepth;
					if(bObjDepth == 0){
						ptmpPayload = (msg->Payload);
						tempIndx = &(msg->Payload_len);
                        // *tempIndx = 0;
					}					
					if(bObjDepth == 1){
						ptmpPayload = (sub_Payload1);
						tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
                        // *tempIndx = 0;
					}
					else if(bObjDepth == 2){
						ptmpPayload = (sub_Payload2);
						tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
                        // *tempIndx = 0;
					}

                    if(Pars_type[bObjDepth] == TYPE_CODE_OBJ)        parsing_step = 1;
                    else if(Pars_type[bObjDepth] == TYPE_CODE_ARR){   parsing_step = 4; str_index = 0;}
                }				
            }		
            else if(buf[*index] == ','){
				(*tempIndx)++;
				memset((ptmpPayload + (*tempIndx))->property_name, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_name));
				memset((ptmpPayload + (*tempIndx))->property_contants, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_contants));
                (ptmpPayload+(*tempIndx))->subPayload_len = 0;
				parsing_step = 1; 
			}		
			else if(buf[*index] > 47 && buf[*index] < 58){
				(ptmpPayload + (*tempIndx))->property_contants[str_index] = buf[*index];
				str_index++;
			}
			// for Data Type Array
			else if(buf[*index] == '['){
                // (ptmpPayload+(*tempIndx))->Msg_type = TYPE_CODE_ARR;
				parsing_step = 4;
				bObjDepth++;
				if(bObjDepth == 1){
					memset(sub_Payload1, 0x00, sizeof(sub_Payload1));
					msg->Payload[msg->Payload_len].sub_Payload = sub_Payload1;
					ptmpPayload = sub_Payload1;
					tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
				}
				else if(bObjDepth == 2){
					memset(sub_Payload2, 0x00, sizeof(sub_Payload2));
					msg->Payload[msg->Payload_len].sub_Payload[msg->Payload[msg->Payload_len].subPayload_len].sub_Payload = sub_Payload2;
					ptmpPayload = sub_Payload2;
					tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
				}
				else if(bObjDepth == 3){
					memset(sub_Payload3, 0x00, sizeof(sub_Payload3));
					msg->Payload[msg->Payload_len].sub_Payload[msg->Payload[msg->Payload_len].subPayload_len].sub_Payload[sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len].sub_Payload = sub_Payload3;
					ptmpPayload = sub_Payload3;
					tempIndx = &(sub_Payload2[sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len].subPayload_len);
				}
				bArrStartFlg = true;
                str_index = 0;
                Pars_type[bObjDepth] = TYPE_CODE_ARR;
			} 	
			// for Data Type Object
			else if(buf[*index] == '{'){
                // (ptmpPayload+(*tempIndx))->Msg_type = TYPE_CODE_OBJ;
				parsing_step = 1;
				bObjDepth++;
				if(bObjDepth == 1){
					memset(sub_Payload1, 0x00, sizeof(sub_Payload1));
					msg->Payload[msg->Payload_len].sub_Payload = sub_Payload1;
					ptmpPayload = sub_Payload1;
					tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
				}
				else if(bObjDepth == 2){
					memset(sub_Payload2, 0x00, sizeof(sub_Payload2));
					msg->Payload[msg->Payload_len].sub_Payload[msg->Payload[msg->Payload_len].subPayload_len].sub_Payload = sub_Payload2;
					ptmpPayload = sub_Payload2;
					tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
				}
				else if(bObjDepth == 3){
					memset(sub_Payload3, 0x00, sizeof(sub_Payload3));
					msg->Payload[msg->Payload_len].sub_Payload[msg->Payload[msg->Payload_len].subPayload_len].sub_Payload[sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len].sub_Payload = sub_Payload3;
					ptmpPayload = sub_Payload3;
					tempIndx = &(sub_Payload2[sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len].subPayload_len);
				}		
                Pars_type[bObjDepth] = TYPE_CODE_OBJ;		
			} 
		}
        else if(parsing_step == 5){
			if((buf[*index] == '\"') && (buf[*index-1] != '\\')){
                parsing_step = 6;
                // printf("[Parse_Payload] Property contants : %s\r\n",(ptmpPayload + (*tempIndx))->property_contants);
            }
			else{
				(ptmpPayload + (*tempIndx))->property_contants[str_index] = buf[*index];
				str_index++;
			}
				
		}
		// for Data Type String
		else if(parsing_step == 6){
			if(buf[*index] == ','){
				(*tempIndx)++;
				memset((ptmpPayload + (*tempIndx))->property_name, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_name));
				memset((ptmpPayload + (*tempIndx))->property_contants, 0x00, sizeof((ptmpPayload + (*tempIndx))->property_contants));
                (ptmpPayload+(*tempIndx))->subPayload_len = 0;
				if(Pars_type[bObjDepth] == TYPE_CODE_OBJ)
                    parsing_step = 1;
                else if(Pars_type[bObjDepth] == TYPE_CODE_ARR){
                    parsing_step = 4;
                    str_index = 0;
                }
			}
            else if(buf[*index] == '}') {
				if(bObjDepth == 0)	break;
				else{
					--bObjDepth;
					if(bObjDepth == 0){
						ptmpPayload = &(msg->Payload);
						tempIndx = &(msg->Payload_len);
					}					
					if(bObjDepth == 1){
						ptmpPayload = &(sub_Payload1);
						tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
					}
					else if(bObjDepth == 2){
						ptmpPayload = &(sub_Payload2);
						tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
					}
				}				
			}		
            else if(buf[*index] == ']') {
                if(bObjDepth == 0)	break;
                else{
                    --bObjDepth;
					if(bObjDepth == 0){
						ptmpPayload = (msg->Payload);
						tempIndx = &(msg->Payload_len);
                        // *tempIndx = 0;
					}					
					if(bObjDepth == 1){
						ptmpPayload = (sub_Payload1);
						tempIndx = &(msg->Payload[msg->Payload_len].subPayload_len);
                        // *tempIndx = 0;
					}
					else if(bObjDepth == 2){
						ptmpPayload = (sub_Payload2);
						tempIndx = &(sub_Payload1[msg->Payload[msg->Payload_len].subPayload_len].subPayload_len);
                        // *tempIndx = 0;
					}
                }				
            }
		}
		// else if(parsing_step == 7){
		// 	if(buf[*index] == ']'){
		// 		bArrStartFlg = false;
		// 		parsing_step = 6;
		// 	}		
		// 	else{
		// 		(ptmpPayload + (*tempIndx))->property_contants[str_index] = buf[*index];
		// 		str_index++;
		// 	}
		// }
    }
    if(parsing_step != 1 && parsing_step != 4 && parsing_step != 6){
        printf("Format Error %d\r\n", parsing_step);
        return true;
    }

    return false;
}


uint8_t Parse_Data(char* curl_ws_recv_buf, size_t rlen)
{
    bool ret = false;

    uint16_t index = 0;

    ret = Parse_Header(curl_ws_recv_buf, &Rx_Msg, &index, rlen);
    if(ret){
        printf("[Parse_Data] Header Parsing Error\r\n");
        return true;
    }
    if(Rx_Msg.Msg_type == 3 && Rx_Msg.UniqueID == DATATRANS_UniqueID)
    {
        DATATRANS_UniqueID = 0;
        memcpy(Rx_Msg.Payload[0].DATATRANS_data , curl_ws_recv_buf, rlen);
        printf("\r\nDATATRANS_data : %s\r\n", Rx_Msg.Payload[0].DATATRANS_data);
    }
    else
    {
        ret = Parse_Payload(curl_ws_recv_buf, &Rx_Msg, &index, rlen);
    if(ret){
        printf("[Parse_Data] Payload Parsing Error\r\n");
        return true;
    }

    // console
	if(Rx_Msg.Msg_type == 2)
		printf("Dump Msg (CALL) [Len:%d]:\r\n[\r\n\"%d\", %s, %s,\r\n{\r\n", Rx_Msg.Payload_len+1, Rx_Msg.Msg_type, Rx_Msg.UniqueID_Char, cs_req_msg[Rx_Msg.Action_Code]);
	else if(Rx_Msg.Msg_type == 3)
		printf("Dump Msg (CALLRES) [Len:%d]:\r\n[\r\n\"%d\", %llu\r\n", Rx_Msg.Payload_len+1, Rx_Msg.Msg_type, Rx_Msg.UniqueID);

	for(int i=0;i<=Rx_Msg.Payload_len;i++){
        if(Rx_Msg.Payload[i].subPayload_len > 0){
            printf("%s: {\r\n ",Rx_Msg.Payload[i].property_name);  
            for(int j = 0; j<=Rx_Msg.Payload[i].subPayload_len;j++){
                if(Rx_Msg.Payload[i].sub_Payload[j].subPayload_len > 0){
                    printf("%s: {\r\n ",Rx_Msg.Payload[i].sub_Payload[j].property_name);
                    for(int k = 0; k<=Rx_Msg.Payload[i].sub_Payload[j].subPayload_len ;k++){
                        if(Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].subPayload_len > 0){
                            for(int l = 0; l<=Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].subPayload_len ;l++){
                                printf("%s:%s ",Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].sub_Payload[l].property_name, Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].sub_Payload[l].property_contants);
                            }
                        }
                        else    printf("%s:%s ",Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].property_name, Rx_Msg.Payload[i].sub_Payload[j].sub_Payload[k].property_contants);
                    }
                }
                else  printf("%s:%s ",Rx_Msg.Payload[i].sub_Payload[j].property_name, Rx_Msg.Payload[i].sub_Payload[j].property_contants);
            }
            printf("\r\n}",Rx_Msg.Payload[i].property_name);  
        }
        else  printf("%s:%s ",Rx_Msg.Payload[i].property_name, Rx_Msg.Payload[i].property_contants);
	}
	printf("\r\n]\r\n");
    }
    

    // clear Receive buffer 잘안됨
    memset(curl_ws_recv_buf,0x00,sizeof(curl_ws_recv_buf));


	// Return Data Format Error
	return 0;
}