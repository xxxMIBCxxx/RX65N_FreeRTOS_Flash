#ifndef MOTOROLA_STYPE_H
#define MOTOROLA_STYPE_H

#include "task_function.h"
#include "r_usb_hmsc_apl_config.h"
#include "r_usb_hmsc_apl.h"
#include <stdio.h>

#define LENGTH_SIZE									( 2 )
#define CHECK_SUM_SIZE								( 2 )
#define	NEW_LINE_SIZE								( 2 )
#define CF_WRITE_MIN_SIZE							( 128 ) 	// 最小プログラムサイズ(BYTE)


//-----------------------------------------------
// 処理結果種別
//-----------------------------------------------
typedef enum
{
	MOTOROLA_STYPE_RESULT_SUCCESS = 0,							// 正常終了
	MOTOROLA_STYPE_RESULT_ERROR_FILE,							// ファイル関連でエラー
	MOTOROLA_STYPE_RESULT_ERROR_ANALYZE,						// 解析エラー
	MOTOROLA_STYPE_RESULT_ERROR_FLASH_WRITE,					// フラッシュ書き込みエラー
	MOTOROLA_STYPE_RESULT_ERROR_UNKNOWN_RECORD_KIND,			// 不明なレコード種別
	MOTOROLA_STYPE_RESULT_ERROR_SYSTEM = 99,					// システムエラー
} MOTOROLA_STYPE_RESULT_ENUM;


//-----------------------------------------------
// コード書き込み情報
//-----------------------------------------------
typedef struct
{
	uint32_t  										StartAddress;	// 書込み開始アドレス
	uint32_t										EndAddress;		// 書込み開始アドレス

	uint8_t											Data[CF_WRITE_MIN_SIZE];

} CF_WRITE_INFO_TABLE;

//-----------------------------------------------
// S-Type レコード種別
//-----------------------------------------------
typedef enum
{
	RECORD_KIND_S0 = 0,			// S0レコード
	RECORD_KIND_S1,				// S1レコード
	RECORD_KIND_S2,				// S2レコード
	RECORD_KIND_S3,				// S3レコード
	RECORD_KIND_S7,				// S7レコード
	RECORD_KIND_S8,				// S8レコード
	RECORD_KIND_S9				// S9レコード
} RECORD_KIND_ENUM;


//-----------------------------------------------
// S1/S2/S3の書込み情報
//-----------------------------------------------
typedef struct
{
	uint32_t				Address;							// ロード・アドレス
	uint8_t					DataSize;							// データ長
	uint8_t					Data[99];							// 書込みデータ
} FLASH_INFO_TABLE;




//-----------------------------------------------
// S0レコード
//-----------------------------------------------
typedef struct
{
	char					sz0E[2];							// "0E"固定
	char					sz0000[4];							// "0000"固定
	char					szFileName[22];						// "ファイル名(8文字) + ファイル形式(3文字)
	char					szChecksum[CHECK_SUM_SIZE];			// チェックサム
	char					szNewLine[NEW_LINE_SIZE];			// ニューライン
} S0_RECORD_TABLE;

//-----------------------------------------------
// S3レコード
//-----------------------------------------------
typedef struct
{
	char					szLength[LENGTH_SIZE];				// レコード長
	char					szAddress[8];						// ロード・アドレス(4BYTE:32bit)
	char					szCode[(0xFF * 2)];					// コード
	char					szChecksum[CHECK_SUM_SIZE];			// チェックサム
	char					szNewLine[NEW_LINE_SIZE];			// ニューライン
} S3_RECORD_TABLE;

//-----------------------------------------------
// S7レコード
//-----------------------------------------------
typedef struct
{
	char					szLength[LENGTH_SIZE];				// レコード長
	char					szEntryPointAddress[8];				// エントリ・ポイント・アドレス
	char					szChecksum[CHECK_SUM_SIZE];			// チェックサム
	char					szNewLine[NEW_LINE_SIZE];			// ニューライン
} S7_RECORD_TABLE;

//-----------------------------------------------
// S-Type　レコード
//-----------------------------------------------
typedef struct
{
	char					szRecordName[2];					// レコード名(最初の2バイトはレコード種別のため、共通項目とする)

	union
	{
		S0_RECORD_TABLE		tS0;
		S3_RECORD_TABLE		tS3;
		S7_RECORD_TABLE		tS7;
	};


	RECORD_KIND_ENUM		eRecordKind;						// レコード種別

} S_TYPE_RECORD_TABLE;


//-----------------------------------------------------------------------------
// モトローラ S-Typeファイルの解析
//-----------------------------------------------------------------------------
MOTOROLA_STYPE_RESULT_ENUM MotorolaStypeAnalyze(FIL *pFile, uint8_t bWriteFlag, uint32_t CfWriteOffsetAddress );


#endif // #ifndef MOTOROLA_STYPE_H
