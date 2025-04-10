/**
*       @file
*               cststring.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2019.03.05 <br>
*               author: ktlee <br>
*               description: <br>
*/
#ifndef __CSTSTRING_H__
#define __CSTSTRING_H__

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

#define STR_TITLE_ERROR					"오류"
#define STR_TITLE_ERRORCODE				"오류코드"
#define STR_TITLE_RFREADER				"RF READER"
#define STR_TITLE_AMI					"AMI 전력량계"
#define STR_TITLE_AMI2					"AMI 2 전력량계"

#define STR_TITLE_COVER_OPEN			"커버 열림"
#define STR_TITLE_GROUND_FAULT			"과전류 발생"

#define STR_DIALOG_CANNOT_PROCEED		"진행이 되지 않습니다."
#define STR_DIALOG_CLOSE_COVER			"커버를 닫아주세요."
#define STR_DIALOG_CALL_MANAGER			"관리자에게 문의하세요."

#define STR_CLOSE_COVER_AFTER_USE		"사용이 끝나셨으면 반드시 커버를 닫아주세요."
#define STR_TIMEOUT_CLOSE_COVER			"시간이 초과되었습니다. 커버를 닫아주세요."
#define STR_CLOSE_COVER_TO_CHARGE		"충전을 시작하기 위해 커버를 닫아주세요."
#define STR_UNPLUGGED_CLOSE_COVER		"플러그가 해제되었습니다. 커버를 닫아주세요."
#define STR_OPEN_COVER_AND_CONNECT		"충전기 커버를 열고 차량과 연결해 주세요."
#define STR_OPEN_COVER_AND_CONNECT2		"충전기 케이블을 차량과 연결해 주세요."
#define STR_CHAGE_STARTS_AFTER_CLOSE	"커버를 닫아야 충전이 시작됩니다."
#define STR_DISCONNECT_AND_CLOSE		"플러그를 분리하고 충전기 커버를 닫아주세요."
#define STR_DISCONNECT_AND_CLOSE2		"충전 케이블을 분리해주세요."

#define STR_EMERGENCY_DIALOG_1			"비상버튼을 돌려"
#define STR_EMERGENCY_DIALOG_2			"정상복귀 해주세요."

#define STR_EMERGENCY_DIALOG_11			"비상버튼을 오른쪽으로 돌려"
#define STR_EMERGENCY_DIALOG_12			"해제하고 커버를 닫아주세요."
#define STR_BLANK_STRING				""

#define STR_EMERGENCY_DIALOG_21			"비상버튼을 오른쪽으로 돌려"
#define STR_EMERGENCY_DIALOG_22			"해제하고 커버를 닫아주세요."

#define STR_EMERGENCY_DIALOG_31			"비상버튼을 오른쪽으로 돌려"
#define STR_EMERGENCY_DIALOG_32			"해제하고 플러그를 제거 후"
#define STR_EMERGENCY_DIALOG_33			"커버를 닫아주세요."

#define STR_EMERGENCY_DIALOG_41			"비상버튼을 오른쪽으로 돌려"
#define STR_EMERGENCY_DIALOG_42			"해제하신 후 충전내역을 확인하세요."

#define STR_OPEN_COVER_AND_CONNECTC		"충전을 시작하기 위해 차량과 연결해 주세요."
#define STR_PRICE_WON					"원"
#define STR_CHARGING_SOC				"%"
#define	STR_TIME_MIN					"분"

#define STR_PASS_AUTH_WAIT				"비밀번호 인증 대기중 입니다."
#define STR_PASS_AUTH_SUCC				"비밀번호 인증 성공"
#define STR_PASS_AUTH_SUCC_1			"충전을 진행합니다"
#define STR_PASS_AUTH_FAIL				"비밀번호 인증 실패"
#define STR_PASS_AUTH_FAIL_1			"다시 시도해주세요"

#define STR_CARD_AUTH_SUCC				"회원 카드 인증 성공"
#define STR_CARD_AUTH_SUCC_1			"충전을 진행합니다"
#define STR_CARD_AUTH_FAIL				"회원 카드 인증 실패"
#define STR_CARD_AUTH_FAIL_1			"다시 시도해주세요"
#define STR_CARD_AUTH_FAIL_2			"선결제 금액으로 결제 되었습니다."
#define STR_CARD_AUTH_FAIL_3			"선결제 + 실충전금액 결제로 결제 되었습니다."
#define STR_CARD_AUTH_FAIL_4			"선결제가 취소되지 않았습니다."

#define STR_CAR_AUTH_SUCC				"차량번호 조회 성공"
#define STR_CAR_AUTH_SUCC_1				"해당 차량을 선택해주세요."
#define STR_CAR_AUTH_FAIL				"조회 결과가 없습니다."
#define STR_CAR_AUTH_FAIL_1				"다시 시도해 주십시오."
#define STR_AUTH_WAIT					"카드 인증 대기중 입니다."
#define STR_WAIT_MSG					"잠시만 기다려 주십시오."
#define STR_CAR_AUTH_WAIT				"차량번호 조회중 입니다."

#define STR_FTPFWUPDATE_WAIT_CON_01		"FTP 서버 연결중입니다."
#define STR_FTPFWUPDATE_DOWNLOAD_01		"펌웨어 다운로드 중입니다."
#define STR_FTPFWUPDATE_WRITE_01		"펌웨어 저장 중입니다."
#define STR_FTPFWUPDATE_VERIFY_01		"펌웨어 확인 중입니다."
#define STR_SYSTEM_RESET_01				"충전기를 다시 시작 합니다."

#define STR_SYSTEM_ERROR_01				"에러 발생!"
#define STR_K1_UPDATE					"펌웨어 업데이트"
#define STR_K1_UPDATE_01				"펌웨어 업데이트 진행 중입니다."
#define STR_K1_UPDATE_02				"시스템이 재부팅됩니다."

#define STR_INPUT_AMOUNT1 				"예상 충전전력량"
#define STR_INPUT_AMOUNT1_01 			"예상 충전금액"
#define STR_INPUT_AMOUNT2 				"kWh"
#define STR_INPUT_AMOUNT2_01 			"원"
#define STR_INPUT_AMOUNT3 				"충전금액을 입력하세요."
#define STR_INPUT_AMOUNT3_01 			"충전할 kWh를 입력하세요."

#define STR_CREDIT_WAIT					"신용카드 승인요청 중 입니다."
#define STR_CREDIT_AUTH_SUCC			"신용카드 승인 성공"
#define STR_CREDIT_AUTH_FAIL			"신용카드 승인 실패"
#define STR_CREDIT_CARD_REMOVE			"카드를 제거하여 주십시오."
#define STR_CREDIT_CARD_REALEND			"선결제 취소가 진행됩니다."
#define STR_CREDIT_TEXT_01				"선결제"
#define STR_CREDIT_TEXT_02				"실충전금액 결제"
#define STR_CREDIT_TEXT_03				"선결제 취소"

#define STR_INPUT_AMOUNT_ERR			"입력금액 오류"
#define STR_INPUT_AMOUNT_ERR_01			"100원 이상 " 
#define STR_INPUT_AMOUNT_ERR_02			"입력해주세요" 

#define STR_INSERT_TIMEOUT_REAL			"선결제 금액으로"
#define STR_INSERT_TIMEOUT_REAL_01		"결제 되었습니다."
#define STR_INSERT_TIMEOUT_CANCEL		"선결제 + 실충전금액 결제로"
#define STR_INSERT_TIMEOUT_CANCEL_01	"결제 되었습니다."
#define STR_INSERT_TIMEOUT_CANCEL_02	"선결제가"
#define STR_INSERT_TIMEOUT_CANCEL_03	"취소되지 않았습니다."

#endif
