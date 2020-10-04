#include "MotorolaStype.h"
#include "platform.h"
#include "r_flash_rx_if.h"


#include "Global.h"
extern GLOBAL_INFO_TABLE			g_tGlobalInfo;

//#define DEBUG_PRINTF_DUMP

#define CF_CLEAR_NUM				( 0x00 )


// プロトタイプ宣言
//void Abort(void);
static uint8_t atohex(const char ch);
static uint8_t StrHex2Num(const char *pszHex);
//static uint8_t StrNum2Num(const char* pszNum);
static uint8_t CheckSum(const char* pszData, uint8_t Size, uint8_t CheckSum);
static uint8_t Analyze_S0(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord);
static uint8_t Analyze_S3(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord, FLASH_INFO_TABLE* ptFlashInfo);
static uint8_t Analyze_S7(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord);
#ifdef DEBUG_PRINTF_DUMP
static void DebugDump(CF_WRITE_INFO_TABLE* ptCfWriteInfo, uint32_t CfWriteOffsetAddress);
#endif // DEBUG_PRINTF_DUMP
static MOTOROLA_STYPE_RESULT_ENUM CF_Write(CF_WRITE_INFO_TABLE* ptCfWriteInfo, uint32_t CfWriteOffsetAddress);


typedef struct
{
	S_TYPE_RECORD_TABLE 			tStypeRecord;
	FLASH_INFO_TABLE 				tFlashInfo;
	CF_WRITE_INFO_TABLE				tCfWriteInfo;
	uint32_t						EntryPointAddress;
} MOTOROLA_GLOBAL_INFO_TABLE;

MOTOROLA_GLOBAL_INFO_TABLE			g_MotorolaInfo;


//-----------------------------------------------------------------------------
// モトローラ S-Typeファイルの解析
//-----------------------------------------------------------------------------
MOTOROLA_STYPE_RESULT_ENUM MotorolaStypeAnalyze(FIL *pFile, uint8_t bWriteFlag, uint32_t CfWriteOffsetAddress )
{
	MOTOROLA_STYPE_RESULT_ENUM		eResult = MOTOROLA_STYPE_RESULT_SUCCESS;
	FRESULT							eFileResult = FR_OK;
	UINT							ReadNum = 0;
	uint8_t							Ret = 0;
	uint16_t						Pos = 0;

	//
	while (1)
	{
		// 1文字読み込んで、レコードタイプを調べる
		eFileResult = f_read(pFile, g_MotorolaInfo.tStypeRecord.szRecordName,
						sizeof(g_MotorolaInfo.tStypeRecord.szRecordName), &ReadNum);
		if (eFileResult != FR_OK)
		{
			printf("f_read Error. [eFileResult:%d]\n", eFileResult);
			eResult = MOTOROLA_STYPE_RESULT_ERROR_FILE;
			goto MotorolaStypeAnalyze_EndProc_Label;
		}
		if (ReadNum == 0)
		{
			break;
		}
		else if (ReadNum != sizeof(g_MotorolaInfo.tStypeRecord.szRecordName))
		{
			printf("f_read size Error. [ReadNum:%d, Size:%d]\n", ReadNum, sizeof(g_MotorolaInfo.tStypeRecord.szRecordName));
			eResult = MOTOROLA_STYPE_RESULT_ERROR_ANALYZE;
			goto MotorolaStypeAnalyze_EndProc_Label;
		}

		// S-Typeレコード種別チェック
		if (g_MotorolaInfo.tStypeRecord.szRecordName[0] != 'S')
		{
			printf("S-Type Record Check Error. [szRecordName[0]:%d]\n",g_MotorolaInfo.tStypeRecord.szRecordName[0]);
			eResult = MOTOROLA_STYPE_RESULT_ERROR_ANALYZE;
			goto MotorolaStypeAnalyze_EndProc_Label;
		}

		switch (g_MotorolaInfo.tStypeRecord.szRecordName[1]) {
		case '0':
			g_MotorolaInfo.tStypeRecord.eRecordKind = RECORD_KIND_S0;
			Ret = Analyze_S0(pFile, &g_MotorolaInfo.tStypeRecord);
			break;
		case '3':
			g_MotorolaInfo.tStypeRecord.eRecordKind = RECORD_KIND_S3;
			Ret = Analyze_S3(pFile, &g_MotorolaInfo.tStypeRecord, &g_MotorolaInfo.tFlashInfo);
			break;
		case '7':
			g_MotorolaInfo.tStypeRecord.eRecordKind = RECORD_KIND_S7;
			Ret = Analyze_S7(pFile, &g_MotorolaInfo.tStypeRecord);
			break;
		case '1':
		case '2':
		case '8':
		case '9':
		default:
			printf("S-Type Record Check Error. [szRecordName[1]:%d]\n",g_MotorolaInfo.tStypeRecord.szRecordName[1]);
			eResult = MOTOROLA_STYPE_RESULT_ERROR_UNKNOWN_RECORD_KIND;
			goto MotorolaStypeAnalyze_EndProc_Label;
			break;
		}

		// レコードタイプの解析結果がNGの場合
		if (Ret == 1)
		{
			printf("S-Type Record Analyze Error. [RecordKind:%d]\n", g_MotorolaInfo.tStypeRecord.eRecordKind);
			eResult = MOTOROLA_STYPE_RESULT_ERROR_ANALYZE;
			goto MotorolaStypeAnalyze_EndProc_Label;
		}

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// 書込みが有効の場合
		if (bWriteFlag == 1)
		{
			if (g_MotorolaInfo.tStypeRecord.eRecordKind == RECORD_KIND_S3)
			{
				if ((g_MotorolaInfo.tCfWriteInfo.StartAddress == 0) && (g_MotorolaInfo.tCfWriteInfo.EndAddress == 0))
				{
					// CF書込み範囲設定
					g_MotorolaInfo.tCfWriteInfo.StartAddress = g_MotorolaInfo.tFlashInfo.Address;
					g_MotorolaInfo.tCfWriteInfo.EndAddress = g_MotorolaInfo.tCfWriteInfo.StartAddress + (CF_WRITE_MIN_SIZE - 1);
					memset(g_MotorolaInfo.tCfWriteInfo.Data, CF_CLEAR_NUM, sizeof(g_MotorolaInfo.tCfWriteInfo.Data));
				}

				// CF書込み範囲確認
				if (g_MotorolaInfo.tCfWriteInfo.StartAddress > g_MotorolaInfo.tFlashInfo.Address)
				{
					// 仕様上、あり得ないエラー
					printf("*** System Error. *** [StartAddress:%08X, Address:%08X]\n", g_MotorolaInfo.tCfWriteInfo.StartAddress, g_MotorolaInfo.tFlashInfo.Address);
					eResult = MOTOROLA_STYPE_RESULT_ERROR_SYSTEM;
					goto MotorolaStypeAnalyze_EndProc_Label;
				}

				// 1Byteずつ、CF書込み範囲を超えていないかチェックする
				for (uint32_t i = 0; i < g_MotorolaInfo.tFlashInfo.DataSize; i++)
				{
					if (g_MotorolaInfo.tCfWriteInfo.EndAddress >= (g_MotorolaInfo.tFlashInfo.Address + i))
					{
						Pos = (g_MotorolaInfo.tFlashInfo.Address + i) - g_MotorolaInfo.tCfWriteInfo.StartAddress;
						g_MotorolaInfo.tCfWriteInfo.Data[Pos] = g_MotorolaInfo.tFlashInfo.Data[i];
					}
					else
					{
						// 書込み処理
						eResult = CF_Write(&g_MotorolaInfo.tCfWriteInfo,CfWriteOffsetAddress);
						if (eResult != MOTOROLA_STYPE_RESULT_SUCCESS)
						{
							printf("CF_Write Error. [eResult:%d]\n",eResult);
							goto MotorolaStypeAnalyze_EndProc_Label;
						}

						// 前回、書き込みデータがピッタリだった場合
						if (i == 0)
						{
							g_MotorolaInfo.tCfWriteInfo.StartAddress = g_MotorolaInfo.tFlashInfo.Address;
							g_MotorolaInfo.tCfWriteInfo.EndAddress = g_MotorolaInfo.tCfWriteInfo.StartAddress + (CF_WRITE_MIN_SIZE - 1);
							memset(g_MotorolaInfo.tCfWriteInfo.Data, CF_CLEAR_NUM, sizeof(g_MotorolaInfo.tCfWriteInfo.Data));
							Pos = (g_MotorolaInfo.tFlashInfo.Address + i) - g_MotorolaInfo.tCfWriteInfo.StartAddress;
							g_MotorolaInfo.tCfWriteInfo.Data[Pos] = g_MotorolaInfo.tFlashInfo.Data[i];

						}
						// 書込みデータが余っている場合
						else
						{
							g_MotorolaInfo.tCfWriteInfo.StartAddress = g_MotorolaInfo.tCfWriteInfo.EndAddress + 1;
							g_MotorolaInfo.tCfWriteInfo.EndAddress = g_MotorolaInfo.tCfWriteInfo.StartAddress + (CF_WRITE_MIN_SIZE - 1);
							memset(g_MotorolaInfo.tCfWriteInfo.Data, CF_CLEAR_NUM, sizeof(g_MotorolaInfo.tCfWriteInfo.Data));
							Pos = (g_MotorolaInfo.tFlashInfo.Address + i) - g_MotorolaInfo.tCfWriteInfo.StartAddress;
							g_MotorolaInfo.tCfWriteInfo.Data[Pos] = g_MotorolaInfo.tFlashInfo.Data[i];
						}

					}
				}
			}
		}
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	}

	// 書込みデータが残っている場合
	if ((g_MotorolaInfo.tCfWriteInfo.StartAddress != 0) && (g_MotorolaInfo.tCfWriteInfo.EndAddress != 0))
	{
		// 書込み処理
		eResult = CF_Write(&g_MotorolaInfo.tCfWriteInfo,CfWriteOffsetAddress);
		if (eResult != MOTOROLA_STYPE_RESULT_SUCCESS)
		{
			printf("CF_Write Error. [eResult:%d]\n",eResult);
			goto MotorolaStypeAnalyze_EndProc_Label;
		}

	}

MotorolaStypeAnalyze_EndProc_Label:

	return eResult;
}












#if 0
//-----------------------------------------------------------------------------
// エラー
//-----------------------------------------------------------------------------
static void Abort(void)
{
	g_tGlobalInfo.eLedKind = LED_KIND_ERROR;
	while(1)
	{
		vTaskDelay(1000);
	}
}
#endif

//-----------------------------------------------------------------------------
// 16進数文字から数値に変換
//-----------------------------------------------------------------------------
static uint8_t atohex(const char ch)
{
	uint8_t				Hex = 0x00;


	switch (ch) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		Hex = ch - '0';
		break;
	case 'A':
	case 'a':
		Hex = 0x0A;
		break;
	case 'B':
	case 'b':
		Hex = 0x0B;
		break;
	case 'C':
	case 'c':
		Hex = 0x0C;
		break;
	case 'D':
	case 'd':
		Hex = 0x0D;
		break;
	case 'E':
	case 'e':
		Hex = 0x0E;
		break;
	case 'F':
	case 'f':
		Hex = 0x0f;
		break;
	default:
		Hex = 0x00;
		break;
	}

	return Hex;
}

//-----------------------------------------------------------------------------
// 2バイトのHEX文字を数値に変換（例："3D" → 61(0x3D)）
//-----------------------------------------------------------------------------
static uint8_t StrHex2Num(const char *pszHex)
{
	uint8_t				Hex = 0x00;



	Hex = atohex(pszHex[0]) << 4;
	Hex += atohex(pszHex[1]);

	return Hex;
}

#if 0
//-----------------------------------------------------------------------------
// 2バイトの10進数文字を数値に変換（例："91" → 91(0x5B)）
//-----------------------------------------------------------------------------
static uint8_t StrNum2Num(const char* pszNum)
{
	uint8_t				Num = 0x00;

	if ((pszNum[0] >= '0') && (pszNum[0] <= '9'))
	{
		Num += pszNum[0] - '0';
	}
	Num = Num * 10;

	if ((pszNum[1] >= '0') && (pszNum[1] <= '9'))
	{
		Num += pszNum[1] - '0';
	}

	return Num;
}
#endif


//-----------------------------------------------------------------------------
// チェックサム
//    戻り値　：　　 1:NG , 0:OK
//-----------------------------------------------------------------------------
static uint8_t CheckSum(const char* pszData, uint8_t Size, uint8_t CheckSum)
{
	uint8_t 			Sum = 0x00;
	uint8_t 			i = 0;


	// Sum & CheckSumを算出する
	for (i = 0; i < Size ; i+=2 )
	{
		Sum += StrHex2Num(&pszData[i]);
	}
	Sum = ~Sum;

	return ((Sum == CheckSum) ? 0 : 1);
}


//-----------------------------------------------------------------------------
// S0レコード解析
//    戻り値　：　　 1:NG , 0:OK
//-----------------------------------------------------------------------------
static uint8_t Analyze_S0(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord)
{
	uint8_t							Result = 0;
	FRESULT							eFileResult = FR_OK;
	UINT							ReadNum = 0;

	// S0のレコードサイズは固定なので、S0サイズ分読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS0.sz0E,sizeof(S0_RECORD_TABLE), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != sizeof(S0_RECORD_TABLE))
	{
		return 1;
	}

	// "E0"チェック
	if ((ptStypeRecord->tS0.sz0E[0] != '0') || (ptStypeRecord->tS0.sz0E[1] != 'E'))
	{
		return 1;
	}

	// "0000"チェック
	for (int i = 0; i < 4; i++)
	{
		if (ptStypeRecord->tS0.sz0000[i] != '0')
		{
			return 1;
		}
	}

	// ニュー・ラインチェック
	if ((ptStypeRecord->tS0.szNewLine[0] != 0x0D) || (ptStypeRecord->tS0.szNewLine[1] != 0x0A))
	{
		return 1;
	}

	// チェクサム
	Result = CheckSum((const char*)&ptStypeRecord->tS0,
		(sizeof(S0_RECORD_TABLE) - (CHECK_SUM_SIZE + NEW_LINE_SIZE)),
		StrHex2Num((const char*)&ptStypeRecord->tS0.szChecksum));
	if (Result == 1)
	{
		return 1;
	}

	return 0;
}


//-----------------------------------------------------------------------------
// S3レコード解析
//    戻り値　：　　 1:NG , 0:OK
//-----------------------------------------------------------------------------
static uint8_t Analyze_S3(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord, FLASH_INFO_TABLE* ptFlashInfo)
{
	uint8_t							Ret = 0;
	FRESULT							eFileResult = FR_OK;
	UINT							ReadNum = 0;
	uint8_t 						Length = 0;


	// S3のレコード長を読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS3.szLength,sizeof(ptStypeRecord->tS3.szLength), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != sizeof(ptStypeRecord->tS3.szLength))
	{
		return 1;
	}
	Length = StrHex2Num((const char*)&ptStypeRecord->tS3.szLength);								// レコード長(16進数) : アドレス(4:8/2) + データ(*x2) + CheckSum(1)

	// ロード・アドレスを読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS3.szAddress,sizeof(ptStypeRecord->tS3.szAddress), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != sizeof(ptStypeRecord->tS3.szAddress))
	{
		return 1;
	}
	ptFlashInfo->Address = 0x00000000;
	for (uint8_t i = 0; i < 8; i++)
	{
		ptFlashInfo->Address += atohex(ptStypeRecord->tS3.szAddress[i]);
		if (i < 7)
		{
			ptFlashInfo->Address = ptFlashInfo->Address << 4;
		}
	}

	// コードを読み込む
	ptFlashInfo->DataSize = Length - (4 + 1);
	eFileResult = f_read(pFile, ptStypeRecord->tS3.szCode,(ptFlashInfo->DataSize * 2), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != (ptFlashInfo->DataSize * 2))
	{
		return 1;
	}
	for (uint8_t i = 0; i < ptFlashInfo->DataSize; i++)
	{
		ptFlashInfo->Data[i] = StrHex2Num((const char*)&ptStypeRecord->tS3.szCode[(i * 2)]);
	}

	// チェクサム・ニューラインを読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS3.szChecksum,(CHECK_SUM_SIZE + NEW_LINE_SIZE), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != (CHECK_SUM_SIZE + NEW_LINE_SIZE))
	{
		return 1;
	}

	// ニュー・ラインチェック
	if ((ptStypeRecord->tS3.szNewLine[0] != 0x0D) || (ptStypeRecord->tS3.szNewLine[1] != 0x0A))
	{
		return 1;
	}

	// チェクサム(レコード長 + アドレス + データ)
	Ret = CheckSum((const char*)&ptStypeRecord->tS3.szLength,
		(sizeof(ptStypeRecord->tS3.szLength) + sizeof(ptStypeRecord->tS3.szAddress) + (ptFlashInfo->DataSize * 2)),
		StrHex2Num((const char*)&ptStypeRecord->tS3.szChecksum));
	if (Ret == 1)
	{
		return 1;
	}

#ifdef DEBUG_PRINTF_DUMP
	printf("[%08p] : ", ptFlashInfo->Address);
	for (int j = 0; j < ptFlashInfo->DataSize; j++)
	{
		printf("0x%02X ", ptFlashInfo->Data[j]);
	}
	printf("\n");
#endif	// #ifdef DEBUG_PRINTF_DUMP

	return 0;
}


//-----------------------------------------------------------------------------
// S7レコード解析
//    戻り値　：　　 1:NG , 0:OK
//-----------------------------------------------------------------------------
static uint8_t Analyze_S7(FIL *pFile, S_TYPE_RECORD_TABLE* ptStypeRecord)
{
	uint8_t							Ret = 0;
	FRESULT							eFileResult = FR_OK;
	UINT							ReadNum = 0;
//	uint8_t 						Length = 0;


	// S7のレコード長を読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS7.szLength,sizeof(ptStypeRecord->tS7.szLength), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != sizeof(ptStypeRecord->tS7.szLength))
	{
		return 1;
	}
//	Length = StrHex2Num((const char*)&ptStypeRecord->tS7.szLength);								// レコード長(16進数) : エントリポイントアドレス(4:8/2) + CheckSum(1)

	// エントリポイントアドレスを読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS7.szEntryPointAddress,sizeof(ptStypeRecord->tS7.szEntryPointAddress), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != sizeof(ptStypeRecord->tS7.szEntryPointAddress))
	{
		return 1;
	}
	g_MotorolaInfo.EntryPointAddress = 0x00000000;
	for (uint8_t i = 0; i < sizeof(ptStypeRecord->tS7.szEntryPointAddress); i++)
	{
		g_MotorolaInfo.EntryPointAddress += atohex(ptStypeRecord->tS7.szEntryPointAddress[i]);
		if (i < 7)
		{
			g_MotorolaInfo.EntryPointAddress = g_MotorolaInfo.EntryPointAddress << 4;
		}
	}

	// チェクサム・ニューラインを読み込む
	eFileResult = f_read(pFile, ptStypeRecord->tS7.szChecksum,(CHECK_SUM_SIZE + NEW_LINE_SIZE), &ReadNum);
	if (eFileResult != FR_OK)
	{
		return 1;
	}
	if (ReadNum != (CHECK_SUM_SIZE + NEW_LINE_SIZE))
	{
		return 1;
	}

	// ニュー・ラインチェック
	if ((ptStypeRecord->tS7.szNewLine[0] != 0x0D) || (ptStypeRecord->tS7.szNewLine[1] != 0x0A))
	{
		return 1;
	}

	// チェクサム(レコード長 + エントリポイントアドレス)
	Ret = CheckSum((const char*)&ptStypeRecord->tS7.szLength,
		(sizeof(ptStypeRecord->tS7.szLength) + sizeof(ptStypeRecord->tS7.szEntryPointAddress)),
		StrHex2Num((const char*)&ptStypeRecord->tS7.szChecksum));
	if (Ret == 1)
	{
		return 1;
	}

#ifdef DEBUG_PRINTF_DUMP
	printf("*** Entry Point Address : %08p ***\n", g_MotorolaInfo.EntryPointAddress);
#endif	// #ifdef DEBUG_PRINTF_DUMP

	return 0;
}

//-----------------------------------------------------------------------------
// コードフラッシュ書き込みデータ表示(DEBUG用)
//-----------------------------------------------------------------------------
#ifdef DEBUG_PRINTF_DUMP
static void DebugDump(CF_WRITE_INFO_TABLE* ptCfWriteInfo, uint32_t CfWriteOffsetAddress)
{
	uint32_t						WriteAddress = 0x00000000;

	if ((FLASH_CF_LO_BANK_LO_ADDR <= ptCfWriteInfo->StartAddress) && (FLASH_CF_LO_BANK_HI_ADDR >= ptCfWriteInfo->StartAddress))
	{
		WriteAddress = ptCfWriteInfo->StartAddress + CfWriteOffsetAddress;
	}
	else
	{
		WriteAddress = ptCfWriteInfo->StartAddress;
	}

	printf("---[CF_WRITE_INFO]--------------------------------------------------------\n");


	for (unsigned int j = 0; j < (CF_WRITE_MIN_SIZE / 16); j++)
	{
		printf("[%08X (%08X)] : ", (WriteAddress+ (16 * j)), (ptCfWriteInfo->StartAddress + (16 * j)));
		for (unsigned int i = 0; i < 16; i++)
		{
			printf("%02X ", ptCfWriteInfo->Data[(16 * j) + i]);
		}
		printf("\n");
	}
	printf("--------------------------------------------------------------------------\n\n");
}
#endif // #if DEBUG_PRINTF_DUMP


//-----------------------------------------------------------------------------
// コードフラッシュ書き込み
//-----------------------------------------------------------------------------
static MOTOROLA_STYPE_RESULT_ENUM CF_Write(CF_WRITE_INFO_TABLE* ptCfWriteInfo, uint32_t CfWriteOffsetAddress)
{
	flash_err_t						eFlashResult = FLASH_SUCCESS;
	uint32_t						WriteAddress = 0x00000000;

#ifdef DEBUG_PRINTF_DUMP
	// コードフラッシュ書き込みデータ表示(DEBUG用)
	DebugDump(ptCfWriteInfo,CfWriteOffsetAddress);
#endif	// #if DEBUG_PRINTF_DUMP

#if 1
	if ((FLASH_CF_LO_BANK_LO_ADDR <= ptCfWriteInfo->StartAddress) && (FLASH_CF_LO_BANK_HI_ADDR >= ptCfWriteInfo->StartAddress))
	{
		WriteAddress = ptCfWriteInfo->StartAddress + CfWriteOffsetAddress;
	}
	else
	{
		WriteAddress = ptCfWriteInfo->StartAddress;
	}

	// プログラムコード以外は書き込まない
	if (FLASH_CF_LO_BANK_LO_ADDR > WriteAddress)
	{
		return MOTOROLA_STYPE_RESULT_SUCCESS;
	}

	// コードフラッシュ書き込み
	eFlashResult = R_FLASH_Write(ptCfWriteInfo->Data, WriteAddress, CF_WRITE_MIN_SIZE);
	if (eFlashResult != FLASH_SUCCESS)
	{
		printf("R_FLASH_Write Error. [eFlashResult:%d, Address:%08X]\n",eFlashResult,WriteAddress);
		return MOTOROLA_STYPE_RESULT_ERROR_FLASH_WRITE;
	}
#endif

	return MOTOROLA_STYPE_RESULT_SUCCESS;
}

