/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "task_function.h"
#include "platform.h"
#include "r_flash_rx_if.h"
#include "MotorolaStype.h"


#include "Global.h"
extern GLOBAL_INFO_TABLE			g_tGlobalInfo;



typedef struct
{
	flash_bank_t 					eBankInfo;							// 起動バンク情報

} FWUPDATE_GLOBAL_INFO_TABLE;

FWUPDATE_GLOBAL_INFO_TABLE			g_tFwupdate;

// コードフラッシュのアクセスウィンドウ情報
flash_access_window_config_t 	g_AccessWindowInfo[2] =
{	//    Start Address           , End Address
		{ FLASH_CF_LO_BANK_LO_ADDR, FLASH_CF_LO_BANK_HI_ADDR },			// [FLASH_BANK0_FFE00000]:0xFFE00000 - 0xFFEFFFFF
		{ FLASH_CF_HI_BANK_LO_ADDR, FLASH_CF_HI_BANK_HI_ADDR }			// [FLASH_BANK0_FFF00000]:0xFFF00000 - 0xFFFFFFFF
};

// コードフラッシュのブロック情報
typedef struct
{
	flash_block_address_t		StartAddress;			// ブロック開始アドレス
	uint32_t					Size;					// ブロックサイズ
} BLOCK_INFO_TABLE;

const BLOCK_INFO_TABLE			g_BlockInfo[][2] =
{ //      0xFFE00000[FLASH_BANK0_FFE00000]  ,   0xFFF00000[FLASH_BANK0_FFF00000]
		{ { FLASH_CF_BLOCK_75, (32 * 1024) }, { FLASH_CF_BLOCK_37, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_74, (32 * 1024) }, { FLASH_CF_BLOCK_36, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_73, (32 * 1024) }, { FLASH_CF_BLOCK_35, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_72, (32 * 1024) }, { FLASH_CF_BLOCK_34, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_71, (32 * 1024) }, { FLASH_CF_BLOCK_33, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_70, (32 * 1024) }, { FLASH_CF_BLOCK_32, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_69, (32 * 1024) }, { FLASH_CF_BLOCK_31, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_68, (32 * 1024) }, { FLASH_CF_BLOCK_30, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_67, (32 * 1024) }, { FLASH_CF_BLOCK_29, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_66, (32 * 1024) }, { FLASH_CF_BLOCK_28, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_65, (32 * 1024) }, { FLASH_CF_BLOCK_27, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_64, (32 * 1024) }, { FLASH_CF_BLOCK_26, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_63, (32 * 1024) }, { FLASH_CF_BLOCK_25, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_62, (32 * 1024) }, { FLASH_CF_BLOCK_24, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_61, (32 * 1024) }, { FLASH_CF_BLOCK_23, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_60, (32 * 1024) }, { FLASH_CF_BLOCK_22, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_59, (32 * 1024) }, { FLASH_CF_BLOCK_21, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_58, (32 * 1024) }, { FLASH_CF_BLOCK_20, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_57, (32 * 1024) }, { FLASH_CF_BLOCK_19, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_56, (32 * 1024) }, { FLASH_CF_BLOCK_18, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_55, (32 * 1024) }, { FLASH_CF_BLOCK_17, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_54, (32 * 1024) }, { FLASH_CF_BLOCK_16, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_53, (32 * 1024) }, { FLASH_CF_BLOCK_15, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_52, (32 * 1024) }, { FLASH_CF_BLOCK_14, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_51, (32 * 1024) }, { FLASH_CF_BLOCK_13, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_50, (32 * 1024) }, { FLASH_CF_BLOCK_12, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_49, (32 * 1024) }, { FLASH_CF_BLOCK_11, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_48, (32 * 1024) }, { FLASH_CF_BLOCK_10, (32 * 1024) } },
		{ { FLASH_CF_BLOCK_47, (32 * 1024) }, { FLASH_CF_BLOCK_9,  (32 * 1024) } },
		{ { FLASH_CF_BLOCK_46, (32 * 1024) }, { FLASH_CF_BLOCK_8,  (32 * 1024) } },
		{ { FLASH_CF_BLOCK_45, (8 * 1024)  }, { FLASH_CF_BLOCK_7,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_44, (8 * 1024)  }, { FLASH_CF_BLOCK_6,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_43, (8 * 1024)  }, { FLASH_CF_BLOCK_5,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_42, (8 * 1024)  }, { FLASH_CF_BLOCK_4,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_41, (8 * 1024)  }, { FLASH_CF_BLOCK_3,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_40, (8 * 1024)  }, { FLASH_CF_BLOCK_2,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_39, (8 * 1024)  }, { FLASH_CF_BLOCK_1,  (8 * 1024)  } },
		{ { FLASH_CF_BLOCK_38, (8 * 1024)  }, { FLASH_CF_BLOCK_0,  (8 * 1024)  } }
};



void Task_Fwupdate(void * pvParameters)
{
	MOTOROLA_STYPE_RESULT_ENUM 		eResult = MOTOROLA_STYPE_RESULT_SUCCESS;
	FRESULT							eFileResult = FR_OK;
	FIL								file;
	flash_err_t						eFlashResult = FLASH_SUCCESS;											//
	uint8_t							ItemNum = sizeof(g_BlockInfo) / (sizeof(BLOCK_INFO_TABLE) * 2);			//  コードフラッシュのブロック情報の項目数
	uint32_t						CfWriteOffsetAddress = 0x00000000;


	// SW
	PORTA.PDR.BIT.B2 = 0;
	PORTA.PCR.BIT.B2 = 1;


	while(1)
	{
		if (g_tGlobalInfo.eUsbKind == USB_KIND_CONNECT)
		{
			vTaskDelay(500);
			break;
		}

		vTaskDelay(500);
	}

	g_tGlobalInfo.eLedKind = LED_KIND_PROCESS;

	// ファイルオープン
	eFileResult = f_open(&file, "update.mot", (FA_OPEN_EXISTING | FA_READ));
	if (eFileResult != FR_OK)
	{
		printf("f_open Error. [eFileResult:%d]\n",eFileResult);
		g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
		goto Task_Fwupdate_EndProc_Label;
	}

	// フラッシュモジュールオープン
	eFlashResult = R_FLASH_Open();
	if (eFlashResult != FLASH_SUCCESS)
	{
		printf("R_FLASH_Open Error. [eFlashResult:%d]\n",eFlashResult);
		g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
		goto Task_Fwupdate_EndProc_Label;
	}

	// 起動バンク取得
	eFlashResult = R_FLASH_Control(FLASH_CMD_BANK_GET,&g_tFwupdate.eBankInfo);
	if (eFlashResult != FLASH_SUCCESS)
	{
		printf("R_FLASH_Control(FLASH_CMD_BANK_GET) Error. [eFlashResult:%d]\n",eFlashResult);
		g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
		goto Task_Fwupdate_EndProc_Label;
	}

	// アクセスウィンドウの範囲を設定
	eFlashResult = R_FLASH_Control(FLASH_CMD_ACCESSWINDOW_SET, (void *)&g_AccessWindowInfo[g_tFwupdate.eBankInfo]);
	if (eFlashResult != FLASH_SUCCESS)
	{
		printf("R_FLASH_Control(FLASH_CMD_ACCESSWINDOW_SET) Error. [eFlashResult:%d]\n",eFlashResult);
		g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
		goto Task_Fwupdate_EndProc_Label;
	}

	// ブロック消去
	for (uint8_t i = 0 ; i < ItemNum ; i++)
	{
		flash_block_address_t 	BlockAddress = g_BlockInfo[i][g_tFwupdate.eBankInfo].StartAddress;
		eFlashResult = R_FLASH_Erase(BlockAddress,1);
		if (eFlashResult != FLASH_SUCCESS)
		{
			printf("R_FLASH_Erase Error. [Address：%p, eFlashResult:%d]\n",BlockAddress,eFlashResult);
			g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
			goto Task_Fwupdate_EndProc_Label;
		}
	}

	// S-Type解析
	CfWriteOffsetAddress = g_AccessWindowInfo[g_tFwupdate.eBankInfo].start_addr - FLASH_CF_LO_BANK_LO_ADDR;
	eResult = MotorolaStypeAnalyze(&file, 1, CfWriteOffsetAddress);
	if (eResult == MOTOROLA_STYPE_RESULT_SUCCESS)
	{
#if 0
		// 起動バンク変更
		eFlashResult = R_FLASH_Control(FLASH_CMD_BANK_TOGGLE, NULL);
		if (eFlashResult != FLASH_SUCCESS)
		{
			printf("R_FLASH_Control(FLASH_CMD_BANK_TOGGLE) Error. [eFlashResult:%d]\n",eFlashResult);
			g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
			goto Task_Fwupdate_EndProc_Label;
		}
#endif
		// @@@ 書き換え終了 @@@
		printf("Update Success!!\n");
		g_tGlobalInfo.eLedKind = LED_KIND_ON;
	}
	else
	{
		g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
		goto Task_Fwupdate_EndProc_Label;
	}

Task_Fwupdate_EndProc_Label:

	while(1)
	{
		vTaskDelay(100);
		if (PORTA.PIDR.BIT.B2 == 0)
		{
			// 起動バンク変更
			eFlashResult = R_FLASH_Control(FLASH_CMD_BANK_TOGGLE, NULL);
			if (eFlashResult != FLASH_SUCCESS)
			{
				printf("R_FLASH_Control(FLASH_CMD_BANK_TOGGLE) Error. [eFlashResult:%d]\n",eFlashResult);
				g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
				goto Task_Fwupdate_EndProc_Label;
			}

			while(1)
			{
				vTaskDelay(1000);
			}
		}
	}
}
