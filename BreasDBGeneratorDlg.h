
// BreasDBGeneratorDlg.h : header file
//

#pragma once
#include "afxdtctl.h"
#include "ATLComTime.h"
#include "afxwin.h"
#include "sqlite3.h"
#include <vector>
#include <map>
#include "VivoTypes.h"


#pragma pack(1)




typedef enum TimeStampTime_e
{
  SESSION_START = 1,      /**< Timestamp type session start */
  SESSION_STOP = 2,       /**< Timestamp type session stop */
  SESSION_24H = 3,        /**< Timestamp type 24 hour session */
  SESSION_SYNC = 4,       /**< Timestamp type data synchronized */
  SESSION_DB_CREATED = 5,    /**< Timestamp type db created */
  SESSION_RESTART = 6,
  SESSION_COMPLIANCESTART = 7,
}TTimeStampType;

typedef struct ComplianceDataRes_s {
  ULONG totalUsageSec; //seconds
  UWORD totalDays; //days
  UWORD daysWithUsage; //days
  ULONG averageUsageSec; //Seconds per day
  UWORD daysWithCompliance; //days
  UWORD averagePEEP; //.01 CmH2O
  UWORD averagepPeak; //.01 CmH2O
  UWORD averageVt; // ml
  UWORD averageLeakage; //.1 ml/S
  unsigned int startOfLog;
} TComplianceDataRes;
// CBreasDBGeneratorDlg dialog
class CBreasDBGeneratorDlg : public CDialogEx
{
// Construction
public:
	CBreasDBGeneratorDlg(CWnd* pParent = NULL);	// standard constructor
  bool isOpenDB = false;
  sqlite3 *dbfile;
  CString m_pathName;
  bool ConnectDB(const char* filename);
  void DisconnectDB();
  int execSQL(CString header, const char* query);
  int GenerateBreathDataDay(unsigned int startTime);
  int GenerateEventDataDay(unsigned int startTime);
  int prepareTrendLog(TTrendDataHolder* trend);
  int prepareBreathLog(TBreathData *eventBreathData);
  int prepareSettingsLog(TSettingsData *settingsData);
  int prepareUsageSessionLog(TUsageSessionData *eventUsageSessionData);
  int prepareUsageEvent(ILONG timeDate, UWORD milliseconds, TRecordType recordType, UINT8 recordId, TUsageEventData *data);
  int prepareTimeStampData(ULONG timeDate, UWORD milliSeconds, TTimeStampType timeStampType, UBYTE logVersion);
  int prepareLevelThreeDBEntry(const char* fn, UINT32 createTime, UINT32 closeTime, UINT32 fileSize);
  int GetSessionStopTimeStamp(UWORD timeStampId, ULONG* timeDate, UWORD* milliSeconds, TTimeStampType* type);
  int readTimestampIndex(UINT32* index);
  int updateCurrentIds();
  int checkBeginTransaction();
  void checkEndTransaction(bool bForce);
  UINT32 getLastTimeStampId();
  void updateComplianceData(std::vector<TUsageSessionData> usageSessionList, unsigned int logStartTime, TComplianceDataRes* data);
  void calcUsagePerDay(TUsageSessionData session, std::map<unsigned int, unsigned int>& usagePerDay);
  unsigned int getSessionLengthSec(TUsageSessionData session);
  void accumulateAvgs(TUsageSessionData session, unsigned int& avgPeep, unsigned int& avgpPeak, unsigned int& avgLeakage, unsigned int& avgVt, unsigned int totalUsage);
  void readUsageSessionRecord(UWORD maxEvents, CStringA outfile);
  void getCurrentIds();
  void CreateLogDb(bool bFill);
  void GenerateFakeLevel3(CString folder, std::vector<TUsageSessionData>& sd, std::vector<std::string>& fn, std::vector<ULONG>& dur);
  bool checkColumnNames(sqlite3_stmt* stm, const char* query);
  void readSettings(UWORD version, CStringA outfile);
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BREASDBGENERATOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  UINT32 lastTimeStampId;
  CDateTimeCtrl m_endDateCtrl;
  COleDateTime m_endDate;
  afx_msg void OnBnClickedGo();
  CEdit m_outputCtrl;
  afx_msg void OnBnClickedGenerateFull();
  afx_msg void OnClose();
  CEdit m_query;
  static void errorDbCallback(void *(pArg), int iErrCode, const char *zMsg);
  afx_msg void OnBnClickedParse();
  afx_msg void OnBnClickedParsepreuse();
  afx_msg void OnBnClickedButton1();
  afx_msg void OnBnClickedParseSessions();
  afx_msg void OnBnClickedLevel3();
  afx_msg void OnBnClickedGenpreallocDb();
  afx_msg void OnBnClickedParseLevel1();
  afx_msg void OnBnClickedFakelevel3();
  afx_msg void OnBnClickedReadsettings();
  afx_msg void OnBnClickedParseLevel2();
};
#pragma pack()