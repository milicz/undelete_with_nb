
// NTFSDrive.cpp: implementation of the CNTFSDrive class.
//
//////////////////////////////////////////////////////////////////////

// #include "stdafx.h"
// #include "NTFSDrive.h"
// #include "MFTRecord.h"

#include "windows.h"
#include "milan.h"
#include "stdio.h"
//#include "string.h"
#include <cstring>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PHYSICAL_DRIVE "\\\\.\\PhysicalDrive0"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNTFSDrive::CNTFSDrive()
{
	m_bInitialized = false;
	
	m_hDrive = 0;
	m_dwBytesPerCluster = 0;
	m_dwBytesPerSector = 0;

	m_puchMFTRecord = 0;
	m_dwMFTRecordSz = 0;

	m_puchMFT = 0;
	m_dwMFTLen = 0;

	m_dwStartSector = 0;	
}

CNTFSDrive::~CNTFSDrive()
{
	if(m_puchMFT)
		delete m_puchMFT;
	m_puchMFT = 0;
	m_dwMFTLen = 0;
}

void CNTFSDrive::SetDriveHandle(HANDLE hDrive)
{
	m_hDrive = hDrive;
	m_bInitialized = false;
}

// this is necessary to start reading a logical drive 
void CNTFSDrive::SetStartSector(DWORD dwStartSector, DWORD dwBytesPerSector)
{
	m_dwStartSector = dwStartSector;
	m_dwBytesPerSector = dwBytesPerSector;
}

int QuickCheck(HANDLE m_hDrive, DWORD m_dwStartSector, DWORD m_dwBytesPerSector){

	NTFS_PART_BOOT_SEC ntfsBS;
	DWORD dwBytes;
	LARGE_INTEGER n84StartPos;

	memset(&n84StartPos, '\0', sizeof(n84StartPos));
	memset(&ntfsBS, '\0', sizeof(ntfsBS));

	n84StartPos.QuadPart = (LONGLONG)m_dwBytesPerSector*m_dwStartSector;

	// point the starting NTFS volume sector in the physical drive
	DWORD dwCur = SetFilePointer(m_hDrive,n84StartPos.LowPart,&n84StartPos.HighPart,FILE_BEGIN);

	// Read the boot sector for the MFT infomation
	int nRet = ReadFile(m_hDrive,&ntfsBS,sizeof(NTFS_PART_BOOT_SEC),&dwBytes,NULL);
	if(!nRet)
		return GetLastError();

	printf("ntfsBS.chOemID: '%4.4s'\n", ntfsBS.chOemID);

}


// initialize will read the MFT record
///   and passes to the LoadMFT to load the entire MFT in to the memory
int CNTFSDrive::Initialize()
{
	NTFS_PART_BOOT_SEC ntfsBS;
	DWORD dwBytes;
	LARGE_INTEGER n84StartPos;

	memset(&n84StartPos, '\0', sizeof(n84StartPos));
	memset(&ntfsBS, '\0', sizeof(ntfsBS));

	n84StartPos.QuadPart = (LONGLONG)m_dwBytesPerSector*m_dwStartSector;

	// point the starting NTFS volume sector in the physical drive
	DWORD dwCur = SetFilePointer(m_hDrive,n84StartPos.LowPart,&n84StartPos.HighPart,FILE_BEGIN);

	// Read the boot sector for the MFT infomation
	int nRet = ReadFile(m_hDrive,&ntfsBS,sizeof(NTFS_PART_BOOT_SEC),&dwBytes,NULL);
	if(!nRet)
		return GetLastError();

	printf("ntfsBS.chOemID: '%4.4s'", ntfsBS.chOemID);

	if(memcmp(ntfsBS.chOemID,"NTFS",4)) // check whether it is realy ntfs
		return ERROR_INVALID_DRIVE;

	/// Cluster is the logical entity
	/// which is made up of several sectors (a physical entity)
	m_dwBytesPerCluster = ntfsBS.bpb.uchSecPerClust * ntfsBS.bpb.wBytesPerSec;	

	if(m_puchMFTRecord)
		delete m_puchMFTRecord;

	m_dwMFTRecordSz = 0x01<<((-1)*((char)ntfsBS.bpb.nClustPerMFTRecord));
	m_puchMFTRecord = new BYTE[m_dwMFTRecordSz];

	m_bInitialized = true;

	// MFTRecord of MFT is available in the MFTRecord variable
	//   load the entire MFT using it
	nRet = LoadMFT(ntfsBS.bpb.n64MFTLogicalClustNum);
	if(nRet)
	{
		m_bInitialized = false;
		return nRet;
	}
	return ERROR_SUCCESS;
}

//// nStartCluster is the MFT table starting cluster
///    the first entry of record in MFT table will always have the MFT record of itself
int CNTFSDrive::LoadMFT(LONGLONG nStartCluster)
{
	DWORD dwBytes;
	int nRet;
	LARGE_INTEGER n64Pos;

	if(!m_bInitialized)
		return ERROR_INVALID_ACCESS;

	CMFTRecord cMFTRec;
	
	wchar_t uszMFTName[10];
	mbstowcs(uszMFTName,"$MFT",10);

	// NTFS starting point
	n64Pos.QuadPart = (LONGLONG)m_dwBytesPerSector*m_dwStartSector;
	// MFT starting point
	n64Pos.QuadPart += (LONGLONG)nStartCluster*m_dwBytesPerCluster;
	

	//  set the pointer to the MFT start
	nRet = SetFilePointer(m_hDrive,n64Pos.LowPart,&n64Pos.HighPart,FILE_BEGIN);
	if(nRet == 0xFFFFFFFF)
		return GetLastError();

	/// reading the first record in the NTFS table.
	//   the first record in the NTFS is always MFT record
	nRet = ReadFile(m_hDrive,m_puchMFTRecord,m_dwMFTRecordSz,&dwBytes,NULL);
	if(!nRet)
		return GetLastError();

	// now extract the MFT record just like the other MFT table records
	cMFTRec.SetDriveHandle(m_hDrive);
	cMFTRec.SetRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz,m_dwBytesPerCluster);
	nRet = cMFTRec.ExtractFile(m_puchMFTRecord,dwBytes);
	if(nRet)
		return nRet;

	if(memcmp(cMFTRec.m_attrFilename.wFilename,uszMFTName,8))
		return ERROR_BAD_DEVICE; // no MFT file available

	if(m_puchMFT)
		delete m_puchMFT;
	m_puchMFT = 0;
	m_dwMFTLen = 0;

	// this data(m_puchFileData) is special since it is the data of entire MFT file
	m_puchMFT = new BYTE[cMFTRec.m_dwFileDataSz];
	m_dwMFTLen = cMFTRec.m_dwFileDataSz;
	
	// store this file to read other files
	memcpy(m_puchMFT, cMFTRec.m_puchFileData, m_dwMFTLen);

	return ERROR_SUCCESS;
}


/// this function if suceeded it will allocate the buufer and passed to the caller
//    the caller's responsibility to free it
int CNTFSDrive::Read_File(DWORD nFileSeq, BYTE *&puchFileData, DWORD &dwFileDataLen)
{
	int nRet;
	
	if(!m_bInitialized)
		return ERROR_INVALID_ACCESS;
	
	CMFTRecord cFile;

	// point the record of the file in the MFT table
	memcpy(m_puchMFTRecord,&m_puchMFT[nFileSeq*m_dwMFTRecordSz],m_dwMFTRecordSz);

	// Then extract that file from the drive
	cFile.SetDriveHandle(m_hDrive);
	cFile.SetRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz,m_dwBytesPerCluster);
	nRet = cFile.ExtractFile(m_puchMFTRecord,m_dwMFTRecordSz);
	if(nRet)
		return nRet;

	
	puchFileData = new BYTE[cFile.m_dwFileDataSz];
	dwFileDataLen = cFile.m_dwFileDataSz;

	// pass the file data, It should be deallocated by the caller
	memcpy(puchFileData,cFile.m_puchFileData,dwFileDataLen);

	return ERROR_SUCCESS;
}


int CNTFSDrive::GetFileDetail(DWORD nFileSeq, ST_FILEINFO &stFileInfo)
{
	int nRet;

	if(!m_bInitialized)
		return ERROR_INVALID_ACCESS;

	if((nFileSeq*m_dwMFTRecordSz+m_dwMFTRecordSz) >= m_dwMFTLen)
		return ERROR_NO_MORE_FILES;

	CMFTRecord cFile;
	// point the record of the file in the MFT table
	memcpy(m_puchMFTRecord,&m_puchMFT[nFileSeq*m_dwMFTRecordSz],m_dwMFTRecordSz);

	// read the only file detail not the file data
	cFile.SetDriveHandle(m_hDrive);
	cFile.SetRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz,m_dwBytesPerCluster);
	nRet = cFile.ExtractFile(m_puchMFTRecord,m_dwMFTRecordSz,true);
	if(nRet)
		return nRet;

	// set the struct and pass the struct of file detail
	memset(&stFileInfo,0,sizeof(ST_FILEINFO));
	wcstombs(stFileInfo.szFilename,(wchar_t*)cFile.m_attrFilename.wFilename,MAX_PATH);
	
	stFileInfo.dwAttributes = cFile.m_attrFilename.dwFlags;

	stFileInfo.n64Create = cFile.m_attrStandard.n64Create;
	stFileInfo.n64Modify = cFile.m_attrStandard.n64Modify;
	stFileInfo.n64Access = cFile.m_attrStandard.n64Access;
	stFileInfo.n64Modfil = cFile.m_attrStandard.n64Modfil;

	stFileInfo.n64Size	 = cFile.m_attrFilename.n64Allocated;
	stFileInfo.n64Size	/= m_dwBytesPerCluster;
	stFileInfo.n64Size	 = (!stFileInfo.n64Size)?1:stFileInfo.n64Size;
	
	stFileInfo.bDeleted = !cFile.m_bInUse;

	return ERROR_SUCCESS;
}

// MFTRecord.cpp: implementation of the CMFTRecord class.
//
//////////////////////////////////////////////////////////////////////

// #include "stdafx.h"
// #include "MFTRecord.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMFTRecord::CMFTRecord()
{
	m_hDrive = 0;

	m_dwMaxMFTRecSize = 1023; // usual size
	m_pMFTRecord = 0;
	m_dwCurPos = 0;

	m_puchFileData = 0; // collected file data buffer
	m_dwFileDataSz = 0; // file data size , ie. m_pchFileData buffer length

	memset(&m_attrStandard,0,sizeof(ATTR_STANDARD));
	memset(&m_attrFilename,0,sizeof(ATTR_FILENAME));

	m_bInUse = false;;
}

CMFTRecord::~CMFTRecord()
{
	if(m_puchFileData)
		delete m_puchFileData;
	m_puchFileData = 0;
	m_dwFileDataSz = 0;
}

// set the drive handle
void CMFTRecord::SetDriveHandle(HANDLE hDrive)
{
	m_hDrive = hDrive;
}

// set the detail
//  n64StartPos is the byte from the starting of the physical disk
//  dwRecSize is the record size in the MFT table
//  dwBytesPerCluster is the bytes per cluster
int CMFTRecord::SetRecordInfo(LONGLONG  n64StartPos, DWORD dwRecSize, DWORD dwBytesPerCluster)
{
	if(!dwRecSize)
		return ERROR_INVALID_PARAMETER;

	if(dwRecSize%2)
		return ERROR_INVALID_PARAMETER;

	if(!dwBytesPerCluster)
		return ERROR_INVALID_PARAMETER;

	if(dwBytesPerCluster%2)
		return ERROR_INVALID_PARAMETER;

	m_dwMaxMFTRecSize = dwRecSize;
	m_dwBytesPerCluster = dwBytesPerCluster;
	m_n64StartPos =  n64StartPos;
	return ERROR_SUCCESS;
}


/// puchMFTBuffer is the MFT record buffer itself (normally 1024 bytes)
//  dwLen is the MFT record buffer length
//  bExcludeData = if true the file data will not be extracted
//                 This is useful for only file browsing
int CMFTRecord::ExtractFile(BYTE *puchMFTBuffer, DWORD dwLen, bool bExcludeData)
{
	if(m_dwMaxMFTRecSize > dwLen)
		return ERROR_INVALID_PARAMETER;
	if(!puchMFTBuffer)
		return ERROR_INVALID_PARAMETER;

	NTFS_MFT_FILE	ntfsMFT;
	NTFS_ATTRIBUTE	ntfsAttr;

	BYTE *puchTmp = 0;
	BYTE *uchTmpData =0;
	DWORD dwTmpDataLen;
	int nRet;
	
	m_pMFTRecord = puchMFTBuffer;
	m_dwCurPos = 0;

	if(m_puchFileData)
		delete m_puchFileData;
	m_puchFileData = 0;
	m_dwFileDataSz = 0;

	// read the record header in MFT table
	memcpy(&ntfsMFT,&m_pMFTRecord[m_dwCurPos],sizeof(NTFS_MFT_FILE));

	if(memcmp(ntfsMFT.szSignature,"FILE",4))
		return ERROR_INVALID_PARAMETER; // not the right signature

	m_bInUse = (ntfsMFT.wFlags&0x01); //0x01  	Record is in use
									  //0x02 	Record is a directory

	//m_dwCurPos = (ntfsMFT.wFixupOffset + ntfsMFT.wFixupSize*2); 
	m_dwCurPos = ntfsMFT.wAttribOffset;

	do
	{	// extract the attribute header
		memcpy(&ntfsAttr,&m_pMFTRecord[m_dwCurPos],sizeof(NTFS_ATTRIBUTE));

		switch(ntfsAttr.dwType) // extract the attribute data 
		{
			// here I haven' implemented the processing of all the attributes.
			//  I have implemented attributes necessary for file & file data extraction
		case 0://UNUSED
			break;

		case 0x10: //STANDARD_INFORMATION
			nRet = ExtractData(ntfsAttr,uchTmpData,dwTmpDataLen);
			if(nRet)
				return nRet;
			memcpy(&m_attrStandard,uchTmpData,sizeof(ATTR_STANDARD));

			delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;
			break;

		case 0x30: //FILE_NAME
			nRet = ExtractData(ntfsAttr,uchTmpData,dwTmpDataLen);
			if(nRet)
				return nRet;
			memcpy(&m_attrFilename,uchTmpData,dwTmpDataLen);

			delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;

			break;

		case 0x40: //OBJECT_ID
			break;
		case 0x50: //SECURITY_DESCRIPTOR
			break;
		case 0x60: //VOLUME_NAME
			break;
		case 0x70: //VOLUME_INFORMATION
			break;
		case 0x80: //DATA
			if(!bExcludeData) 
			{
				nRet = ExtractData(ntfsAttr,uchTmpData,dwTmpDataLen);
				if(nRet)
					return nRet;

				if(!m_puchFileData)
				{
					m_dwFileDataSz = dwTmpDataLen;
					m_puchFileData = new BYTE[dwTmpDataLen];
					
					memcpy(m_puchFileData,uchTmpData,dwTmpDataLen);
				}
				else
				{
					puchTmp = new BYTE[m_dwFileDataSz+dwTmpDataLen];
					memcpy(puchTmp,m_puchFileData,m_dwFileDataSz);
					memcpy(puchTmp+m_dwFileDataSz,uchTmpData,dwTmpDataLen);

					m_dwFileDataSz += dwTmpDataLen;
					delete m_puchFileData;
					m_puchFileData = puchTmp;
				}

				delete uchTmpData;
				uchTmpData = 0;
				dwTmpDataLen = 0;
			}
			break;

		case 0x90: //INDEX_ROOT
		case 0xa0: //INDEX_ALLOCATION
			// todo: not implemented to read the index mapped records
			return ERROR_SUCCESS;
			continue;
			break;
		case 0xb0: //BITMAP
			break;
		case 0xc0: //REPARSE_POINT
			break;
		case 0xd0: //EA_INFORMATION
			break;
		case 0xe0: //EA
			break;
		case 0xf0: //PROPERTY_SET
			break;
		case 0x100: //LOGGED_UTILITY_STREAM
			break;
		case 0x1000: //FIRST_USER_DEFINED_ATTRIBUTE
			break;

		case 0xFFFFFFFF: // END 
			if(uchTmpData)
				delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;
			return ERROR_SUCCESS;

		default:
			break;
		};

		m_dwCurPos += ntfsAttr.dwFullLength; // go to the next location of attribute
	}
	while(ntfsAttr.dwFullLength);

	if(uchTmpData)
		delete uchTmpData;
	uchTmpData = 0;
	dwTmpDataLen = 0;
	return ERROR_SUCCESS;
}

// extract the attribute data from the MFT table
//   Data can be Resident & non-resident
int CMFTRecord::ExtractData(NTFS_ATTRIBUTE ntfsAttr, BYTE *&puchData, DWORD &dwDataLen)
{
	DWORD dwCurPos = m_dwCurPos;

	if(!ntfsAttr.uchNonResFlag)
	{// residence attribute, this always resides in the MFT table itself
		
		puchData = new BYTE[ntfsAttr.Attr.Resident.dwLength];
		dwDataLen = ntfsAttr.Attr.Resident.dwLength;

		memcpy(puchData,&m_pMFTRecord[dwCurPos+ntfsAttr.Attr.Resident.wAttrOffset],dwDataLen);
	}
	else
	{// non-residence attribute, this resides in the other part of the physical drive

		if(!ntfsAttr.Attr.NonResident.n64AllocSize) // i don't know Y, but fails when its zero
			ntfsAttr.Attr.NonResident.n64AllocSize = (ntfsAttr.Attr.NonResident.n64EndVCN - ntfsAttr.Attr.NonResident.n64StartVCN) + 1;

		// ATTR_STANDARD size may not be correct
		dwDataLen = ntfsAttr.Attr.NonResident.n64RealSize;

		// allocate for reading data
		puchData = new BYTE[ntfsAttr.Attr.NonResident.n64AllocSize];

		BYTE chLenOffSz; // length & offset sizes
		BYTE chLenSz; // length size
		BYTE chOffsetSz; // offset size
		LONGLONG n64Len, n64Offset; // the actual lenght & offset
		LONGLONG n64LCN =0; // the pointer pointing the actual data on a physical disk
		BYTE *pTmpBuff = puchData;
		int nRet;

		dwCurPos += ntfsAttr.Attr.NonResident.wDatarunOffset;;

		for(;;)
		{
			///// read the length of LCN/VCN and length ///////////////////////
			chLenOffSz = 0;

			memcpy(&chLenOffSz,&m_pMFTRecord[dwCurPos],sizeof(BYTE));

			dwCurPos += sizeof(BYTE);

			if(!chLenOffSz)
				break;

			chLenSz		= chLenOffSz & 0x0F;
			chOffsetSz	= (chLenOffSz & 0xF0) >> 4;

			///// read the data length ////////////////////////////////////////
	
			n64Len = 0;

			memcpy(&n64Len,&m_pMFTRecord[dwCurPos],chLenSz);

			dwCurPos += chLenSz;

			///// read the LCN/VCN offset //////////////////////////////////////
	
			n64Offset = 0;

			memcpy(&n64Offset,&m_pMFTRecord[dwCurPos],chOffsetSz);
			
			dwCurPos += chOffsetSz;

			////// if the last bit of n64Offset is 1 then its -ve so u got to make it -ve /////
			if((((char*)&n64Offset)[chOffsetSz-1])&0x80)
				for(int i=sizeof(LONGLONG)-1;i>(chOffsetSz-1);i--)
					((char*)&n64Offset)[i] = 0xff;
			
			n64LCN += n64Offset;
			
			n64Len *= m_dwBytesPerCluster;
			///// read the actual data /////////////////////////////////////////
			/// since the data is available out side the MFT table, physical drive should be accessed
			nRet = ReadRaw(n64LCN,pTmpBuff,(DWORD&)n64Len);
			if(nRet)
				return nRet;

			pTmpBuff += n64Len;
		}
	}
	return ERROR_SUCCESS;
}

// read the data from the physical drive
int CMFTRecord::ReadRaw(LONGLONG n64LCN, BYTE *chData, DWORD &dwLen)
{
	int nRet;

	LARGE_INTEGER n64Pos;
	
	n64Pos.QuadPart = (n64LCN)*m_dwBytesPerCluster;
	n64Pos.QuadPart += m_n64StartPos;

	//   data is available in the relative sector from the begining od the drive	
	//    so point that data
	nRet = SetFilePointer(m_hDrive,n64Pos.LowPart,&n64Pos.HighPart,FILE_BEGIN);
	if(nRet == 0xFFFFFFFF)
		return GetLastError();

	BYTE *pTmp			= chData;
	DWORD dwBytesRead	=0;
	DWORD dwBytes		=0;
	DWORD dwTotRead		=0;

	while(dwTotRead <dwLen)
	{
		// v r reading a cluster at a time
		dwBytesRead = m_dwBytesPerCluster;

		// this can not read partial sectors
		nRet = ReadFile(m_hDrive,pTmp,dwBytesRead,&dwBytes,NULL);
		if(!nRet)
			return GetLastError();

		dwTotRead += dwBytes;
		pTmp += dwBytes;
	}

	dwLen = dwTotRead;

	return ERROR_SUCCESS;
}

int main(int argc, char *argv[]) {

	printf("Zoran Car !\n");
    int iz = 0;

	while (1)
	{
		 iz++;
	}

	return 0;
	int i,nRet;
	DWORD dwBytes;

	PARTITION *PartitionTbl;
	DRIVEPACKET stDrive;

	memset (&stDrive, '\0', sizeof(DRIVEPACKET));

	BYTE szSector[512];
	WORD wDrive =0;
	DWORD dwDeleted = 0;

	char szTmpStr[64];
	char szTxt[255];
	char szBuffer[255];

	DWORD dwMainPrevRelSector =0;
	DWORD dwPrevRelSector =0;

	HANDLE hDrive = CreateFile(PHYSICAL_DRIVE,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
	// if(hDrive == INVALID_HANDLE_VALUE)
		// return GetLastError();

	nRet = ReadFile(hDrive,szSector,512,&dwBytes,0); //read first 512B from the drive
	// if(!nRet)
		// return GetLastError();

	dwPrevRelSector =0;
	dwMainPrevRelSector =0;
	int br = 0;

	PartitionTbl = (PARTITION*)(szSector+0x1BE); //set position to the partition table

		for(i=0; i<4; i++) /// scanning partitions in the physical disk
	{
		stDrive.wCylinder = PartitionTbl->chCylinder;
		stDrive.wHead = PartitionTbl->chHead;
		stDrive.wSector = PartitionTbl->chSector;
		stDrive.dwNumSectors = PartitionTbl->dwNumberSectors;
		stDrive.wType = ((PartitionTbl->chType == PART_EXTENDED) || (PartitionTbl->chType == PART_DOSX13X)) ? EXTENDED_PART:BOOT_RECORD;

		if((PartitionTbl->chType == PART_EXTENDED) || (PartitionTbl->chType == PART_DOSX13X))
		{
			dwMainPrevRelSector			= PartitionTbl->dwRelativeSector;
			stDrive.dwNTRelativeSector	= dwMainPrevRelSector;
		}
		else
		{
			stDrive.dwNTRelativeSector = dwMainPrevRelSector + PartitionTbl->dwRelativeSector;
		}

		if(stDrive.wType == EXTENDED_PART)
			break;

		printf("Partition type: '%x'\n", PartitionTbl->chType );
		if(PartitionTbl->chType == 0)
			break;

		switch(PartitionTbl->chType)
		{
		case PART_DOS2_FAT:
			strcpy(szTmpStr, "FAT12");
			break;
		case PART_DOSX13:
		case PART_DOS4_FAT:
		case PART_DOS3_FAT:
			strcpy(szTmpStr, "FAT16");
			break;
		case PART_DOS32X:
		case PART_DOS32:
			strcpy(szTmpStr, "FAT32");			//Normal FAT32
			break;
		case 7:
			strcpy(szTmpStr, "NTFS");	// NTFS , v r interested only on this
			QuickCheck(hDrive, stDrive.dwNTRelativeSector,512);
			i=6;
			break;
		case PART_LINUX_SWAP:
			strcpy(szTmpStr, "Linux Swap");	// NTFS , v r interested only on this
			break;
		case PART_LINUX:
			strcpy(szTmpStr, "Linux Partition");	// NTFS , v r interested only on this
			break;
		default:
			strcpy(szTmpStr, "Unknown");
			break;
		}

		wsprintf(szTxt, "%s Drive %d", szTmpStr,wDrive);

		//AddDrive(szTxt,&stDrive);
		printf("%s Drive %d \n\n", szTmpStr,wDrive);
		PartitionTbl++;
		wDrive++;
		if(br) break;

	}



	// HANDLE  m_hDrive;

	// PARTITION *PartitionTbl;
	// DRIVEPACKET stDrive;

	// m_hDrive = CreateFile(PHYSICAL_DRIVE,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,NULL);




	CNTFSDrive m_cNTFS;



	m_cNTFS.SetDriveHandle(hDrive); // set the physical drive handle

	m_cNTFS.SetStartSector(stDrive.dwNTRelativeSector,512); // set the starting sector of the NTFS
	
	m_cNTFS.Initialize();

	CNTFSDrive::ST_FILEINFO stFInfo;
	
	for(i=0;(i<0xFFFFFFFF);i++)
	{

	memset(szBuffer, '\0', 255);
	
	nRet = m_cNTFS.GetFileDetail(i+30,stFInfo); // get the file detail one by one
		if((nRet == ERROR_NO_MORE_FILES)||(nRet == ERROR_INVALID_PARAMETER))
			exit;

		strcpy(szBuffer,"");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_READONLY)
			strcat(szBuffer,"R-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_HIDDEN)
			strcat(szBuffer,"H-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_SYSTEM)
			strcat(szBuffer,"S-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_DIRECTORY)
			strcat(szBuffer,"D-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_ARCHIVE)
			strcat(szBuffer,"A-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_ENCRYPTED)
			strcat(szBuffer,"E-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_NORMAL)
			strcat(szBuffer,"N-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_TEMPORARY)
			strcat(szBuffer,"T-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_SPARSE_FILE)
			strcat(szBuffer,"Sp-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_REPARSE_POINT)
			strcat(szBuffer,"ReSp-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_COMPRESSED)
			strcat(szBuffer,"C-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_OFFLINE)
			strcat(szBuffer,"O-");

		if(stFInfo.dwAttributes&FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
			strcat(szBuffer,"I-");

		if(stFInfo.dwAttributes&0x4000)
			strcat(szBuffer,"En-");  // if it is encrypted

		if(stFInfo.dwAttributes&0x10000000)
			strcat(szBuffer,"Dir-"); // if it is directory

		if(stFInfo.dwAttributes&0x10000000)
			strcat(szBuffer,"In-"); // if it is indexed

		// strcpy(szBuffer,"");
		if(stFInfo.bDeleted)
		{
			dwDeleted++;
			strcpy(szBuffer,"Yes");
		}

	printf("stFInfo.szFilename %s: szBuffer: %s\n", stFInfo.szFilename, szBuffer);

	}



    return 0;
}

