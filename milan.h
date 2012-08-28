/*
 * milan.h
 *
 *  Created on: Aug 14, 2012
 *      Author: mmilic
 */

#ifndef MILAN_H_
#define MILAN_H_




#endif /* MILAN_H_ */

// MFTRecord.h: interface for the CMFTRecord class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFTRECORD_H__04A1B8DF_0EB0_4B72_8587_2703342C5675__INCLUDED_)
#define AFX_MFTRECORD_H__04A1B8DF_0EB0_4B72_8587_2703342C5675__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// #pragma pack(push, curAlignment)
//#include "NTFSDrive.h"

#pragma pack(push, 8)

#pragma pack(1)

////////////////////////////MFT record header and attribute header //////////////////////
struct NTFS_MFT_FILE
{
	char		szSignature[4];		// Signature "FILE"
	WORD		wFixupOffset;		// offset to fixup pattern
	WORD		wFixupSize;			// Size of fixup-list +1
	LONGLONG	n64LogSeqNumber;	// log file seq number
	WORD		wSequence;			// sequence nr in MFT
	WORD		wHardLinks;			// Hard-link count
	WORD		wAttribOffset;		// Offset to seq of Attributes
	WORD		wFlags;				// 0x01 = NonRes; 0x02 = Dir
	DWORD		dwRecLength;		// Real size of the record
	DWORD		dwAllLength;		// Allocated size of the record
	LONGLONG	n64BaseMftRec;		// ptr to base MFT rec or 0
	WORD		wNextAttrID;		// Minimum Identificator +1
	WORD		wFixupPattern;		// Current fixup pattern
	DWORD		dwMFTRecNumber;		// Number of this MFT Record
								// followed by resident and
								// part of non-res attributes
};

typedef struct	// if resident then + RESIDENT
{					//  else + NONRESIDENT
	DWORD	dwType;
	DWORD	dwFullLength;
	BYTE	uchNonResFlag;
	BYTE	uchNameLength;
	WORD	wNameOffset;
	WORD	wFlags;
	WORD	wID;

	union ATTR
	{
		struct RESIDENT
		{
			DWORD	dwLength;
			WORD	wAttrOffset;
			BYTE	uchIndexedTag;
			BYTE	uchPadding;
		} Resident;

		struct NONRESIDENT
		{
			LONGLONG	n64StartVCN;
			LONGLONG	n64EndVCN;
			WORD		wDatarunOffset;
			WORD		wCompressionSize; // compression unit size
			BYTE		uchPadding[4];
			LONGLONG	n64AllocSize;
			LONGLONG	n64RealSize;
			LONGLONG	n64StreamSize;
			// data runs...
		}NonResident;
	}Attr;
} NTFS_ATTRIBUTE;

//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////// Attributes /////////////////////////////////////////////////////
typedef struct
{
	LONGLONG	n64Create;		// Creation time
	LONGLONG	n64Modify;		// Last Modify time
	LONGLONG	n64Modfil;		// Last modify of record
	LONGLONG	n64Access;		// Last Access time
	DWORD		dwFATAttributes;// As FAT + 0x800 = compressed
	DWORD		dwReserved1;	// unknown

} ATTR_STANDARD;

typedef struct
{
	LONGLONG	dwMftParentDir;            // Seq-nr parent-dir MFT entry
	LONGLONG	n64Create;                  // Creation time
	LONGLONG	n64Modify;                  // Last Modify time
	LONGLONG	n64Modfil;                  // Last modify of record
	LONGLONG	n64Access;                  // Last Access time
	LONGLONG	n64Allocated;               // Allocated disk space
	LONGLONG	n64RealSize;                // Size of the file
	DWORD		dwFlags;					// attribute
	DWORD		dwEAsReparsTag;				// Used by EAs and Reparse
	BYTE		chFileNameLength;
	BYTE		chFileNameType;            // 8.3 / Unicode
	WORD		wFilename[512];             // Name (in Unicode ?)

}ATTR_FILENAME;
//////////////////////////////////////////////////////////////////////////////////////////

// #pragma pack(pop, curAlignment)

#pragma pack(pop)

class CMFTRecord
{
protected:
	HANDLE	m_hDrive;

	BYTE *m_pMFTRecord;
	DWORD m_dwMaxMFTRecSize;
	DWORD m_dwCurPos;
	DWORD m_dwBytesPerCluster;
	LONGLONG m_n64StartPos;

	int ReadRaw(LONGLONG n64LCN, BYTE *chData, DWORD &dwLen);
	int ExtractData(NTFS_ATTRIBUTE ntfsAttr, BYTE *&puchData, DWORD &dwDataLen);

public:
///////// attributes //////////////////////
	ATTR_STANDARD m_attrStandard;
	ATTR_FILENAME m_attrFilename;

	BYTE *m_puchFileData; // collected file data buffer
	DWORD m_dwFileDataSz; // file data size , ie. m_pchFileData buffer length

	bool m_bInUse;

public:
	int SetRecordInfo(LONGLONG n64StartPos, DWORD dwRecSize, DWORD dwBytesPerCluster);
	void SetDriveHandle(HANDLE hDrive);

	int ExtractFile(BYTE *puchMFTBuffer, DWORD dwLen, bool bExcludeData=false);

	CMFTRecord();
	virtual ~CMFTRecord();

};

#endif // !defined(AFX_MFTRECORD_H__04A1B8DF_0EB0_4B72_8587_2703342C5675__INCLUDED_)

// NTFSDrive.h: interface for the CNTFSDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NTFSDRIVE_H__078B2392_2978_4C23_97FD_166C4B234BF3__INCLUDED_)
#define AFX_NTFSDRIVE_H__078B2392_2978_4C23_97FD_166C4B234BF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// #pragma pack(push, curAlignment)

#pragma pack(push, 8)
#pragma pack(1)
////////////////////////////  boot sector info ///////////////////////////////////////
struct NTFS_PART_BOOT_SEC
{
	char		chJumpInstruction[3];
	char		chOemID[4];
	char		chDummy[4];

	struct NTFS_BPB
	{
		WORD		wBytesPerSec;
		BYTE		uchSecPerClust;
		WORD		wReservedSec;
		BYTE		uchReserved[3];
		WORD		wUnused1;
		BYTE		uchMediaDescriptor;
		WORD		wUnused2;
		WORD		wSecPerTrack;
		WORD		wNumberOfHeads;
		DWORD		dwHiddenSec;
		DWORD		dwUnused3;
		DWORD		dwUnused4;
		LONGLONG	n64TotalSec;
		LONGLONG	n64MFTLogicalClustNum;
		LONGLONG	n64MFTMirrLogicalClustNum;
		int			nClustPerMFTRecord;
		int			nClustPerIndexRecord;
		LONGLONG	n64VolumeSerialNum;
		DWORD		dwChecksum;
	} bpb;

	char		chBootstrapCode[426];
	WORD		wSecMark;
};
/////////////////////////////////////////////////////////////////////////////////////////
// #pragma pack(pop, curAlignment)
#pragma pack(pop)


class CNTFSDrive
{
protected:
//////////////// physical drive info/////////////
	HANDLE	m_hDrive;
	DWORD m_dwStartSector;
	bool m_bInitialized;

	DWORD m_dwBytesPerCluster;
	DWORD m_dwBytesPerSector;

	int LoadMFT(LONGLONG nStartCluster);

/////////////////// the MFT info /////////////////
	BYTE *m_puchMFT;  /// the var to hold the loaded entire MFT
	DWORD m_dwMFTLen; // size of MFT

	BYTE *m_puchMFTRecord; // 1K, or the cluster size, whichever is larger
	DWORD m_dwMFTRecordSz; // MFT record size

public:
	struct ST_FILEINFO // this struct is to retrieve the file detail from this class
	{
		char szFilename[MAX_PATH]; // file name
		LONGLONG	n64Create;		// Creation time
		LONGLONG	n64Modify;		// Last Modify time
		LONGLONG	n64Modfil;		// Last modify of record
		LONGLONG	n64Access;		// Last Access time
		DWORD		dwAttributes;	// file attribute
		LONGLONG	n64Size;		// no of cluseters used
		bool 		bDeleted;		// if true then its deleted file
	};

	int GetFileDetail(DWORD nFileSeq, ST_FILEINFO &stFileInfo);
	int Read_File(DWORD nFileSeq, BYTE *&puchFileData, DWORD &dwFileDataLen);

	void SetDriveHandle(HANDLE hDrive);
	void  SetStartSector(DWORD dwStartSector, DWORD dwBytesPerSector);

	int Initialize();
	CNTFSDrive();
	virtual ~CNTFSDrive();

};

#endif // !defined(AFX_NTFSDRIVE_H__078B2392_2978_4C23_97FD_166C4B234BF3__INCLUDED_)

// UndeleteDlg.h : header file
//

#if !defined(AFX_UNDELETEDLG_H__8B418511_9F89_47D7_9F50_B4D15178914B__INCLUDED_)
#define AFX_UNDELETEDLG_H__8B418511_9F89_47D7_9F50_B4D15178914B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000




typedef struct
{
	WORD	wCylinder;
	WORD	wHead;
	WORD	wSector;
	DWORD	dwNumSectors;
	WORD	wType;
	DWORD	dwRelativeSector;
	DWORD	dwNTRelativeSector;
	DWORD	dwBytesPerSector;

}DRIVEPACKET;


#pragma pack(push, 8)
#pragma pack(1)
typedef struct
{
	BYTE	chBootInd;
	BYTE	chHead;
	BYTE	chSector;
	BYTE	chCylinder;
	BYTE	chType;
	BYTE	chLastHead;
	BYTE	chLastSector;
	BYTE	chLastCylinder;
	DWORD	dwRelativeSector;
	DWORD	dwNumberSectors;

}PARTITION;

#pragma pack(pop)

#define PART_TABLE 0
#define BOOT_RECORD 1
#define EXTENDED_PART 2

#define PART_UNKNOWN 0x00		//Unknown.
#define PART_DOS2_FAT 0x01		//12-bit FAT.
#define PART_DOS3_FAT 0x04		//16-bit FAT. Partition smaller than 32MB.
#define PART_EXTENDED 0x05		//Extended MS-DOS Partition.
#define PART_DOS4_FAT 0x06		//16-bit FAT. Partition larger than or equal to 32MB.
#define PART_DOS32 0x0B			//32-bit FAT. Partition up to 2047GB.
#define PART_DOS32X 0x0C		//Same as PART_DOS32(0Bh), but uses Logical Block Address Int 13h extensions.
#define PART_DOSX13 0x0E		//Same as PART_DOS4_FAT(06h), but uses Logical Block Address Int 13h extensions.
#define PART_DOSX13X 0x0F		//Same as PART_EXTENDED(05h), but uses Logical Block Address Int 13h extensions.
#define PART_LINUX_SWAP 0x82
#define PART_LINUX 0x83


/////////////////////////////////////////////////////////////////////////////
// CUndeleteDlg dialog


class CUndeleteDlg //: public CDialog
{
// Construction
public:
	void AddDrive(char *szDrvTxt, DRIVEPACKET *pstDrive);
	// CUndeleteDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CUndeleteDlg)
	// enum { IDD = IDD_UNDELETE_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUndeleteDlg)
	protected:
	// virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int ScanLogicalDrives();
	HICON m_hIcon;
	// HTREEITEM m_hTreeRoot;

	CNTFSDrive m_cNTFS;

	HANDLE m_hDrive;

	// CString m_cszFindText;
	DWORD m_dwFoundItem;

	HANDLE m_hScanFilesThread;
	bool m_bStopScanFilesThread;
	static DWORD WINAPI ScanFilesThread(LPVOID lpVoid);


	// Generated message map functions
	//{{AFX_MSG(CUndeleteDlg)
	// virtual BOOL OnInitDialog();
	// afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	// afx_msg void OnPaint();
	// afx_msg HCURSOR OnQueryDragIcon();
	// afx_msg void OnBtnScan();
	// afx_msg void OnDestroy();
	// afx_msg void OnMnuFindFirst();
	// afx_msg void OnMnuFindNext();
	// afx_msg void OnMnuSave();
	// afx_msg void OnRClickLstFiles(NMHDR* pNMHDR, LRESULT* pResult);
	// afx_msg void OnKeyDownLstFiles(NMHDR* pNMHDR, LRESULT* pResult);
	// }}AFX_MSG
	// DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UNDELETEDLG_H__8B418511_9F89_47D7_9F50_B4D15178914B__INCLUDED_)

