
// BreasDBGeneratorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BreasDBGenerator.h"
#include "BreasDBGeneratorDlg.h"
#include "afxdialogex.h"
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#pragma pack(1)
#define DAYS_TO_SECS(days) (days*24*60*60)
#define MAX_BREATH_LOG_ID (40 * 31 * 60 * 24) //1785600 - max of 40 breaths per min over 31 days
#define MAX_EVENT_LOG_ID  (10 * 365 * 24)  //87600 - 10 events per hour over 365 days
#define MAX_SETTINGS_LOG_ID  (1 * 365 * 24)  //8760 - 1 session start/stop per hour over 365 days 
#define MAX_SESSION_LOG_ID  (1 * 365 * 24) //8760 - 1 session start/stop per hour over 365 days
#define MAX_TIMESTAMP_LOG_ID  (1 * 365 * 24) //8760 - 1 session start/stop per hour over 365 days
static int currentBreathId = 0; //stored as static in order to avoid high-speed database reads.
static int currentEventId = 0;
static int currentSessionId = 0;
static int currentSettingsId = 0;
static int currentSessionStartTimeStampId = 0; //used for breath log - matching time stamp must be a start, 24H, or restart
static int currentTimeStampId = 0; //used for other events which can occur either when treatment is running or not.
static bool bMaxBreathIdReached = false;
static bool bMaxEventIdReached = false;
static bool bMaxSessionIdReached = false;
static bool bMaxSettingsIdReached = false;
static bool bMaxTimeStampIdReached = false;
static bool bTransactionBegun = false;
static int openInsertCount = 0;
static UINT64 lastTransactionBegun = 0;
#define MAX_OPEN_INSERTS 50000
#define LOG_DB_VERSION 5
#define SETTING_DB_VERSION 4
#include <vector>
/*#define CREATE_BREATH_LOG "CREATE TABLE IF NOT EXISTS BreathLog (\
    breathLogId                   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
    timeStampId                   INTEGER,\
    timeDate                      INTEGER,\
    breathData                    BLOB);\
    CREATE INDEX `idx_BreathLog_timeDate` ON `BreathLog` ( `timeDate` ASC, `breathLogId`, `timeStampId`, `breathData` )"

#define CREATE_ID_TABLE "CREATE TABLE IF NOT EXISTS IDTable \
  (idTableId INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
  timeStampId INTEGER UNIQUE,\
   breathLogId INTEGER,\
   settingsLogId INTEGER,\
   eventLogId INTEGER,\
   sessionLogId INTEGER,\
   sessionStartLogId INTEGER,\
  tIDmaxReached INTEGER,\
  bIDmaxReached INTEGER,\
  sIDmaxReached INTEGER,\
  eIDmaxReached INTEGER,\
  ssIDmaxReached INTEGER)"

#define CREATE_LEVEL_THREE_TABLE "CREATE TABLE IF NOT EXISTS LevelThreeFiles \
  (fileId INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
      fileName TEXT,\
      createTime INTEGER,\
      duration INTEGER,\
      size INTEGER)"

#define CREATE_SETTINGS_LOG "CREATE TABLE IF NOT EXISTS SettingsLog (\
    settingsId              INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
    timeStampId             INTEGER REFERENCES TimeStamp ( timeStampId ),\
    timeDate                INTEGER,\
    settingsData            BLOB);\
    CREATE INDEX IF NOT EXISTS 'idx_SettingsLog_timeDate' ON 'SettingsLog' ('timeDate' ASC,'settingsId','timeStampId','settingsData');"

#define CREATE_TIME_STAMP "CREATE TABLE IF NOT EXISTS TimeStamp (\
    timeStampId   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
    timeDate      INTEGER,\
    milliSeconds  INTEGER,\
    timeStampType INTEGER,\
    logVersion    INTEGER);\
    CREATE INDEX IF NOT EXISTS 'idx_TimeStamp_timeDate' ON 'TimeStamp' ('timeDate' ASC, 'timeStampId','milliSeconds','timeStampType','logVersion');"


#define CREATE_TREND_LOG "CREATE TABLE IF NOT EXISTS TrendLog \
  (trendLogId INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
      samplesPerPixel INTEGER,\
      head INTEGER,\
      tail INTEGER,\
      oldestTimeStamp INTEGER,\
      vals BLOB,\
      UNIQUE(samplesPerPixel))"

#define CREATE_USAGE_EVENT "CREATE TABLE IF NOT EXISTS UsageEvent ( \
    usageEventId INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
    timeStampId  INTEGER,\
    timeDate     INTEGER,\
    milliSeconds INTEGER,\
    recordType   INTEGER,\
    recordId     INTEGER,\
    eventData         BLOB);\
    CREATE INDEX IF NOT EXISTS 'idx_UsageEvent_timeDate' ON 'UsageEvent' ('timeDate' ASC, 'usageEventId', 'timeStampId', 'milliSeconds', 'recordType', 'recordId', 'eventData');"

#define CREATE_USAGE_SESSION_LOG "CREATE TABLE IF NOT EXISTS UsageSessionLog ( \
    usageSessionLogId        INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,\
    timeStampId              INTEGER,\
    timeDate                 INTEGER,\
    usageSession             BLOB);\
    CREATE INDEX IF NOT EXISTS 'idx_UsageSession_timeDate' ON 'UsageSessionLog' ('timeDate' ASC, 'usageSessionLogId', 'timeStampId','usageSession')"


#define SET_LOG_VERSION_1 "PRAGMA user_version = 1"
#define SET_LOG_VERSION_2 "PRAGMA user_version = 2"
#define SET_LOG_VERSION_3 "PRAGMA user_version = 3"
#define SET_LOG_VERSION_4 "PRAGMA user_version = 4"

#define INSERT_OR_REPLACE_BREATH_LOG_PREP "INSERT OR REPLACE INTO BreathLog VALUES (?, ?, ?, ?)"
#define INSERT_OR_REPLACE_EVENT_PREP "INSERT OR REPLACE INTO UsageEvent VALUES (?, ?, ?, ?, ?, ?, ?)"
#define INSERT_OR_REPLACE_SETTINGS_LOG_PREP "INSERT OR REPLACE INTO SettingsLog VALUES (?, ?, ?, ?)"
#define INSERT_OR_REPLACE_TIME_STAMP_PREP "INSERT OR REPLACE INTO TimeStamp VALUES (?, ?, ?, ?, ?)"
#define INSERT_OR_REPLACE_TREND_LOG_PREP "INSERT OR REPLACE INTO TrendLog VALUES (null, ?, ?, ?, ?, ?)"
#define INSERT_OR_REPLACE_USAGE_SESSION_LOG_PREP "INSERT OR REPLACE INTO UsageSessionLog VALUES (?, ?, ?, ?)"


#define INSERT_TIME_STAMP_PREP "INSERT INTO TimeStamp VALUES (null, ?, ?, ?, ?)"
#define INSERT_USAGE_EVENT_PREP "INSERT INTO UsageEvent VALUES (null, ?, ?, ?, ?, ?, ?)"
#define GET_MAX_TIMESTAMP_ID_PREP  "SELECT MAX(timeStampId) FROM TimeStamp"

#define READ_BREATH_LOG "SELECT breathLogId, timeStampId, timeDate, breathData FROM BreathLog where timeDate>0 and timeDate<current_time order by timeDate"
#define READ_PREUSE_EVENTS "SELECT data from UsageEvent WHERE recordId=22"
#define READ_SETTING_EVENTS "SELECT settingsData from SettingsLog"//WHERE settingsId > 500"
#define READ_X_LATEST_SESSION_LOG_PREP "SELECT timeStampId, timeDate, usageSession FROM UsageSessionLog ORDER BY timeStampId desc LIMIT 3650"

#define INSERT_LEVEL_THREE_ENTRY "INSERT INTO LevelThreeFiles VALUES (null, ?, ?, ?, ?)"
#define UPDATE_LAST_IDS "INSERT OR REPLACE INTO IDTable VALUES (0,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
*/
#include "C:\vivo45\RootKernel\src\logTask\database\logSqlStrings.h"
#include "C:\vivo45\RootKernel\src\paramServerTask\database\settingSqlStrings.h"
#define GET_MAX_TIMESTAMP_ID_PREP  "SELECT MAX(timeStampId) FROM TimeStamp"
#define READ_PREUSE_EVENTS "SELECT data from UsageEvent WHERE recordId=22"
// CAboutDlg dialog used for App About
double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);

  PCFreq = double(li.QuadPart) / 1000.0;

  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;
}
double GetCounter()
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return double(li.QuadPart - CounterStart) / PCFreq;
}

class CAboutDlg : public CDialogEx
{
public:
  CAboutDlg();

  // Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ABOUTBOX };
#endif

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CBreasDBGeneratorDlg dialog



CBreasDBGeneratorDlg::CBreasDBGeneratorDlg(CWnd* pParent /*=NULL*/)
  : CDialogEx(IDD_BREASDBGENERATOR_DIALOG, pParent)
  , m_endDate(COleDateTime::GetCurrentTime())
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  dbfile = NULL;
}

void CBreasDBGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_OUTPUT, m_outputCtrl);
  DDX_Control(pDX, IDC_QUERY, m_query);
}

BEGIN_MESSAGE_MAP(CBreasDBGeneratorDlg, CDialogEx)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDC_GO, &CBreasDBGeneratorDlg::OnBnClickedGo)
  ON_BN_CLICKED(IDC_GENERATE, &CBreasDBGeneratorDlg::OnBnClickedGenerateFull)
  ON_WM_CLOSE()
  ON_BN_CLICKED(IDC_PARSE, &CBreasDBGeneratorDlg::OnBnClickedParse)
  ON_BN_CLICKED(IDC_PARSEPREUSE, &CBreasDBGeneratorDlg::OnBnClickedParsepreuse)
  ON_BN_CLICKED(IDC_BUTTON1, &CBreasDBGeneratorDlg::OnBnClickedButton1)
  ON_BN_CLICKED(IDC_BUTTON2, &CBreasDBGeneratorDlg::OnBnClickedParseSessions)
  ON_BN_CLICKED(IDC_LEVEL3, &CBreasDBGeneratorDlg::OnBnClickedLevel3)
  ON_BN_CLICKED(IDC_GENPREALLOC_DB, &CBreasDBGeneratorDlg::OnBnClickedGenpreallocDb)
  ON_BN_CLICKED(IDC_PARSE_LEVEL1, &CBreasDBGeneratorDlg::OnBnClickedParseLevel1)
  ON_BN_CLICKED(IDC_FAKELEVEL3, &CBreasDBGeneratorDlg::OnBnClickedFakelevel3)
  ON_BN_CLICKED(IDC_READSETTINGS, &CBreasDBGeneratorDlg::OnBnClickedReadsettings)
  ON_BN_CLICKED(IDC_PARSE_LEVEL2, &CBreasDBGeneratorDlg::OnBnClickedParseLevel2)
END_MESSAGE_MAP()


// CBreasDBGeneratorDlg message handlers

BOOL CBreasDBGeneratorDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    BOOL bNameValid;
    CString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  lastTimeStampId = 0;
  m_endDate = COleDateTime::GetCurrentTime();
  UpdateData(false);

  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBreasDBGeneratorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialogEx::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBreasDBGeneratorDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBreasDBGeneratorDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}



void CBreasDBGeneratorDlg::OnBnClickedGo()
{
  if (dbfile == NULL)
  {
    // szFilters is a text string that includes two file name filters:
    TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

    // Create an OPEN dialog; the default file name extension is ".db".
    CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
      OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
    }
    else
    {
      return;
    }
  }
  UpdateData(true);
  CString query;
  m_query.GetWindowText(query);
  execSQL(_T("\r\nQuery result: "), CStringA(query));
}

void initBreath(TBreathData* breath)
{
  breath->spO2Perc = 99;                       /**< Onyx O2 saturation */
  breath->leakage = 12;                        /**< Breath Leakage */
                                              /* Expiration values */
  breath->peakPressure = 2000;
  breath->peep = 500;                    /**< Positive end-expiratory pressure */
  breath->maxTidalVolume = 300;
}

void initSession(TUsageSessionData* data)
{
  data->averagePeakPressure = 1500;;
  data->averagePEEP = 500;
  data->averageVt = 200;
  data->averageMV = 20;
  data->patientTriggeredBreathPercent = 46;
  data->averageEtCO2;
  data->averageLeakage = 20;
  data->averageFiO2 = 210;
  data->averageSpO2 = 99;

}


void CBreasDBGeneratorDlg::OnBnClickedGenpreallocDb()
{
  CreateLogDb(false);
}

void CBreasDBGeneratorDlg::OnBnClickedGenerateFull()
{
  CreateLogDb(true);
}

void CBreasDBGeneratorDlg::CreateLogDb(bool bFill)
{
  UpdateData(true);
  CString outText;
  // szFilters is a text string that includes two file name filters:
  TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

  // Create a Save dialog; the default file name extension is ".my".
  CFileDialog fileDlg(FALSE, _T("db"), _T("*.db"),
    OFN_HIDEREADONLY, szFilters);

  // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
  // returns IDOK.
  if (fileDlg.DoModal() == IDOK)
  {
    if (!ConnectDB(CStringA(fileDlg.GetPathName())))
    {

      m_outputCtrl.SetWindowText(_T("Could not create DB file"));
    }
    else
    {
      m_pathName = fileDlg.GetPathName();
      int rc = SQLITE_OK;
      if (dbfile == NULL)
      {
        return;
      }
      rc = sqlite3_exec(dbfile, CREATE_BREATH_LOG, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_SETTINGS_LOG, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_TIME_STAMP, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_USAGE_EVENT, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_USAGE_SESSION_LOG, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_LEVEL_THREE_TABLE, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, CREATE_TREND_LOG, NULL, NULL, NULL);
      rc = sqlite3_exec(dbfile, "PRAGMA user_version = 7", NULL, NULL, NULL); //Insert version of the database
      rc = sqlite3_exec(dbfile, CREATE_ID_TABLE, NULL, NULL, NULL);

      if (rc == SQLITE_OK)
      {
        execSQL(_T("Tables Created: "), "SELECT name FROM sqlite_master WHERE type = 'table'");
        if (bFill)
        {
          CTime t(2019, 1, 1, 8, 0, 0); //January 1, 2019 8AM
          struct tm tm_s;
          t.GetLocalTm(&tm_s);
          time_t endT = mktime(&tm_s);
          time_t startT = endT - DAYS_TO_SECS(365);

          unsigned int eventTime = (unsigned int)startT;
          prepareTimeStampData(ULONG(endT - DAYS_TO_SECS(365)), 0, SESSION_DB_CREATED, 4);

          m_outputCtrl.GetWindowTextW(outText);
          outText += " ";
          m_outputCtrl.SetWindowTextW(outText);
          RedrawWindow();
          TBreathData breath = { 0 };
          TSettingsData settings = { 0 };
          initBreath(&breath);
          TUsageEventData event = { 0 };
          int alarmCode = 0;
          int eventCode = 0;
          TUsageSessionData session = { 0 };
          initSession(&session);
          //****** prepare breath log
          prepareUsageSessionLog(&session);
          int nDays = 0;
          for (int i = 0; i < MAX_BREATH_LOG_ID; i++)
          {
            unsigned int inc = 2000 * i; //30 breaths/min = 1 breath every 2000 ms.
            breath.timeDate = (ULONG)(startT + (unsigned int)(inc / 1000));
            breath.milliSeconds = 0;
            prepareBreathLog(&breath);
          }
          execSQL(_T("\nBreaths Created: "), "SELECT COUNT(*) From BreathLog");

          for (int i = 0; i < (MAX_EVENT_LOG_ID)/3; i++)
          {
            event.eventData1 = 1;
            prepareUsageEvent((ILONG)startT + 720 * i, 0, AlarmRecord, ++alarmCode, &event);
            event.eventData1 = 0;
            prepareUsageEvent((ILONG)startT + 720 * i + 1, 1, AlarmRecord, alarmCode, &event);
            prepareUsageEvent((ILONG)startT + 720 * i + 360, 0, EventRecord, ++eventCode, &event);
            if (alarmCode > 166)
              alarmCode = 0;
            if (eventCode > 0x4f)
              eventCode = 0;
          }
          execSQL(_T("\nEvents Created: "), "SELECT COUNT(*) From UsageEvent");

          session.startTimeDate = (unsigned int)startT;
          session.timeDate = session.startTimeDate + DAYS_TO_SECS(1);
          int i = 0;
          while (session.timeDate < endT)
          {

            if (i % 5 == 0)
            {
              session.startTimeDate += 3600; //peep next session to start 6 hrs later
              int length = 43200; //session will be 12 hrs long
              prepareTimeStampData(session.startTimeDate, 0, TTimeStampType::SESSION_START, 4); //add its timmestamp
              prepareTimeStampData(session.startTimeDate + length, 0, TTimeStampType::SESSION_STOP, 4);
              prepareUsageSessionLog(&session); //create a new session
              session.timeDate = session.startTimeDate + length;
              char buf[256];
              sprintf_s(buf, 256, "H:\\BreasLevel3_%lu.v45", session.startTimeDate);
              prepareLevelThreeDBEntry(buf, (UINT32)session.startTimeDate, session.timeDate, 0); //create a level three entry
              
              if (i == 20) //make two level three files for testing purposes
              {
                std::vector<TUsageSessionData> sd;
                std::vector<std::string> fn;
                std::vector<ULONG> sz;
                GenerateFakeLevel3(fileDlg.GetFolderPath(),sd, fn, sz);
                CString outText;
                m_outputCtrl.GetWindowTextW(outText);
                for (int f = 0; f < fn.size(); f++)
                {
                  char fileName[32];
                  char ext[5];
                  _splitpath(fn[f].c_str(), 0, 0, fileName, ext);
                  std::stringstream newfn;
                  newfn << "H:\\" << fileName << ext;
                  prepareLevelThreeDBEntry(newfn.str().c_str(), sd[f].startTimeDate, sd[f].timeDate, sz[f]);
                  CStringA line; line.Format("%s %d \n", fn[f].c_str(), sd[f].timeDate - sd[f].startTimeDate);
                  outText += line;
                }
                m_outputCtrl.SetWindowTextW(outText);

              }
              prepareSettingsLog(&settings); //add a settings log for the session close
              session.startTimeDate += length + 7200;
            }
            else if (i % 58 == 0)
            {
              int length = DAYS_TO_SECS(1);
              prepareTimeStampData(session.startTimeDate, 0, TTimeStampType::SESSION_RESTART, 4); //add its timmestamp
              prepareTimeStampData(session.startTimeDate + length, 0, TTimeStampType::SESSION_STOP, 4);
              session.timeDate = session.startTimeDate + length;
              prepareUsageSessionLog(&session); //create a new session
              char buf[256];
              sprintf_s(buf, 256, "H:\\BreasLevel3_%lu.v45", session.startTimeDate);
              prepareLevelThreeDBEntry(buf, (UINT32)session.startTimeDate, session.timeDate, 0); //create a level three entry
              session.startTimeDate += DAYS_TO_SECS(1); //continuous
            }
            else if (i % 37 == 0)
            {
              int length = DAYS_TO_SECS(1);
              prepareTimeStampData(session.startTimeDate, 0, TTimeStampType::SESSION_START, 4); //add its timmestamp
              prepareTimeStampData(session.startTimeDate + length, 0, TTimeStampType::SESSION_SYNC, 4);
              session.timeDate = session.startTimeDate + length;
              prepareUsageSessionLog(&session); //create a new session
              char buf[256];
              sprintf_s(buf, 256, "H:\\BreasLevel3_%lu.v45", session.startTimeDate);
              prepareLevelThreeDBEntry(buf, (UINT32)session.startTimeDate, session.timeDate, 0); //create a level three entry
              session.startTimeDate += DAYS_TO_SECS(1); //continuous
            }
            else
            {
              int length = DAYS_TO_SECS(1);
              prepareTimeStampData(session.startTimeDate, 0, TTimeStampType::SESSION_START, 4); //add its timmestamp
              prepareTimeStampData(session.startTimeDate + length, 0, TTimeStampType::SESSION_24H, 4);
              session.timeDate = session.startTimeDate + length;
              prepareUsageSessionLog(&session); //create a new session
              char buf[256];
              sprintf_s(buf, 256, "H:\\BreasLevel3_%lu.v45", session.startTimeDate);
              prepareLevelThreeDBEntry(buf, (UINT32)session.startTimeDate, session.timeDate, 0); //create a level three entry
              session.startTimeDate += DAYS_TO_SECS(1); //continuous
            }
            i++;
          }
          TTrendDataHolder trend = { 0 };
          int spp[4] = { 9,55,221,443 };
          for (int j = 0; j < 4; j++)
          {
            trend.samplesPerPixel = spp[j];
            prepareTrendLog(&trend);
          }
        }
        execSQL(_T("\nSessions Created: "), "SELECT COUNT(*) From UsageSessionLog");
        execSQL(_T("\nTimestamps Created: "), "SELECT COUNT(*) From TimeStamp");
        execSQL(_T("\nSettings Created: "), "SELECT COUNT(*) From SettingsLog");
        execSQL(_T("\nTrends Created: "), "SELECT COUNT(*) From TrendLog");
        execSQL(_T("\nLevel 3 Created: "), "SELECT COUNT(*) From LevelThreeFiles");
        updateCurrentIds();
        checkEndTransaction(1);
        m_outputCtrl.GetWindowTextW(outText);
        outText += "done\r\n";
        m_outputCtrl.SetWindowTextW(outText);
      }
    }
  }
}
void CBreasDBGeneratorDlg::errorDbCallback(void *(pArg), int iErrCode, const char *zMsg)
{
  CString outText;
  CBreasDBGeneratorDlg* dlg = ((CBreasDBGeneratorDlg*)theApp.m_pMainWnd);
  dlg->m_outputCtrl.GetWindowTextW(outText);
  outText += "\nSQL Error: ";
  outText += CStringW(zMsg);
  outText += "\n";
  dlg->m_outputCtrl.SetWindowTextW(outText);
  dlg->RedrawWindow();
  OutputDebugString(CStringW(zMsg));
}
bool CBreasDBGeneratorDlg::ConnectDB(const char* filename)
{
  sqlite3_config(SQLITE_CONFIG_LOG, errorDbCallback, NULL);
  if (sqlite3_open(filename, &dbfile) == SQLITE_OK)
  {
    isOpenDB = true;
    return true;
  }

  return false;
}

void CBreasDBGeneratorDlg::DisconnectDB()
{
  if (isOpenDB == true)
  {
    sqlite3_close(dbfile);
    dbfile = nullptr;
  }
}

int CBreasDBGeneratorDlg::execSQL(CString header, const char* query)
{
  sqlite3_stmt *statement;
  int rc = sqlite3_prepare_v2(dbfile, query, (int)strlen(query), &statement, 0);
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += "\r\n";
  outText += header;
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  if (rc == SQLITE_OK)
  {
    do
    {
      rc = sqlite3_step(statement);
      int i = 0;
      if (rc == SQLITE_ROW)
      {
        const unsigned char* text = sqlite3_column_text(statement, 0);

        m_outputCtrl.GetWindowTextW(outText);
        outText += " ";
        outText += CStringA(text);
        m_outputCtrl.SetWindowTextW(outText);
        m_outputCtrl.RedrawWindow();
      }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(statement);
  }
  if (rc == SQLITE_DONE)
    rc = SQLITE_OK;
  return rc;

}


int CBreasDBGeneratorDlg::GenerateBreathDataDay(unsigned int startTime)
{
  int rc = SQLITE_OK;
  sqlite3_stmt * breathLogStm = NULL;
  TBreathData breath = { 0 };


  breath.timeDate = (ULONG)startTime;                       /**< The time in seconds from 1970 */
  breath.milliSeconds = 0;                   /**< The trailing ms from 1970 */
                                        /* Inspiration values */
  breath.spO2Perc = 99;                       /**< Onyx O2 saturation */
 //+++

  int increment_in_ms = 1000 * 60 / 30;
  int elapsed = 0;

  unsigned int endT = startTime + DAYS_TO_SECS(1);
  sqlite3_exec(dbfile, "BEGIN TRANSACTION", NULL, NULL, NULL);
  while (breath.timeDate < endT)
  {
    breath.timeDate = (ULONG)(startTime + elapsed / 1000);
    breath.milliSeconds = elapsed % 1000;
    elapsed += increment_in_ms;

    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_BREATH_LOG_PREP, sizeof(INSERT_OR_REPLACE_BREATH_LOG_PREP), &breathLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(breathLogStm, 1, 2);
      rc = sqlite3_bind_int(breathLogStm, 2, breath.timeDate);
      rc = sqlite3_bind_blob(breathLogStm, 3, &breath, sizeof(TBreathData), SQLITE_STATIC);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(breathLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }
      if (breathLogStm != NULL)
        sqlite3_clear_bindings(breathLogStm);
    }
    else
    {
      DisconnectDB();
      ConnectDB(CStringA(m_pathName));
      sqlite3_exec(dbfile, "BEGIN TRANSACTION", NULL, NULL, NULL);
    }
    sqlite3_reset(breathLogStm);
    sqlite3_finalize(breathLogStm);
  }


  sqlite3_exec(dbfile, "END TRANSACTION", NULL, NULL, NULL);

  return rc;
}


int CBreasDBGeneratorDlg::readTimestampIndex(UINT32* index)
{
  int rc;
  sqlite3_stmt * timestampIdStm = NULL;

  if (dbfile == NULL)
    return SQLITE_ERROR;

  rc = sqlite3_prepare_v2(dbfile, GET_MAX_TIMESTAMP_ID_PREP, sizeof(GET_MAX_TIMESTAMP_ID_PREP), &timestampIdStm, NULL);
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_step(timestampIdStm);
    if (rc == SQLITE_ROW)
    {
      *index = (UINT32)sqlite3_column_int(timestampIdStm, 0);
    }

    sqlite3_finalize(timestampIdStm);
  }

  return rc;
}

UINT32 CBreasDBGeneratorDlg::getLastTimeStampId()
{
  if (lastTimeStampId == 0)
  {
    readTimestampIndex(&lastTimeStampId);
  }
  return lastTimeStampId;
}

int CBreasDBGeneratorDlg::GenerateEventDataDay(unsigned int startTime)
{
  int rc = SQLITE_OK;
  sqlite3_stmt * eventLogStm = NULL;
  TUsageEventData userEvent = { 0 };


  userEvent.eventData1 = 1; //alarm active;

  int increment_in_ms = 15000; //one alarm every 15 seconds
  int elapsed = 0;
  unsigned int timeDate = (ULONG)startTime;                       /**< The time in seconds from 1970 */
  unsigned int milliSeconds = 0;                   /**< The trailing ms from 1970 */
  unsigned int endT = startTime + DAYS_TO_SECS(1);
  sqlite3_exec(dbfile, "BEGIN TRANSACTION", NULL, NULL, NULL);
  while (timeDate < endT)
  {
    timeDate = (ULONG)(startTime + elapsed / 1000);
    milliSeconds = elapsed % 1000;
    elapsed += increment_in_ms;

    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_EVENT_PREP, sizeof(INSERT_OR_REPLACE_EVENT_PREP), &eventLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(eventLogStm, 1, getLastTimeStampId());
      rc = sqlite3_bind_int(eventLogStm, 2, timeDate);
      rc = sqlite3_bind_int(eventLogStm, 3, milliSeconds);
      rc = sqlite3_bind_int(eventLogStm, 4, 1);
      rc = sqlite3_bind_int(eventLogStm, 5, 42); //low pressure alarm
      sqlite3_bind_blob(eventLogStm, 6, &userEvent, sizeof(TUsageEventData), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(eventLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }
      if (eventLogStm != NULL)
        sqlite3_clear_bindings(eventLogStm);
    }
    else
    {
      DisconnectDB();
      ConnectDB(CStringA(m_pathName));
      sqlite3_exec(dbfile, "BEGIN TRANSACTION", NULL, NULL, NULL);
    }
    sqlite3_reset(eventLogStm);
    sqlite3_finalize(eventLogStm);
  }


  sqlite3_exec(dbfile, "END TRANSACTION", NULL, NULL, NULL);

  return rc;
}




void CBreasDBGeneratorDlg::OnClose()
{
  checkEndTransaction(1);
  if (dbfile != NULL)
    DisconnectDB();
  CDialogEx::OnClose();
}

static int tableCountCallback(void *output, int count, char **data, char **columns)
{
  if (*data == NULL)
    return 1;
  if (count == 1)
  {
    int* count = (int*)output;
    *count = atoi(data[0]);
  }
  return 0;
}



void CBreasDBGeneratorDlg::OnBnClickedParse()
{
  CStringA outfile;
  if (dbfile == NULL)
  {
    // szFilters is a text string that includes two file name filters:
    TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

    // Create an OPEN dialog; the default file name extension is ".db".
    CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
      OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
      outfile = fileDlg.GetPathName();
      outfile.Replace(".db", ".txt");
    }
    else
    {
      return;
    }
  }
  else
  {
    outfile = m_pathName;
    outfile.Replace(".db", ".txt");
  }



  int dbTableCount = 0;
  const char tc[] = "SELECT COUNT(name) FROM sqlite_master WHERE type='table'";
  if (!sqlite3_exec(dbfile, tc, tableCountCallback, &dbTableCount, NULL) || dbTableCount != 9)
  {
  }
  else
  {
    MessageBox(_T("Invalid Vivo45L.db file"), MB_OK);
  }

  getCurrentIds();
  sqlite3_stmt *statement;
  StartCounter();
  int rc = sqlite3_prepare(dbfile, READ_BREATH_DATA_PREP, (int)strlen(READ_BREATH_DATA_PREP), &statement, 0);
  sqlite3_bind_int(statement, 1, 0);
  sqlite3_bind_int(statement, 2, 1509047996);
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("working");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  FILE* fp;
  fopen_s(&fp, outfile, "w+");
  if (!fp)
    return;
  fprintf(fp, "BrId\tTSId\ttimestamp\tpPeak\tPEEP\tvT\n");
  double avg = 0.;
  int nbreathsRead = 0;
  if (rc == SQLITE_OK)
  {
    do
    {
      StartCounter();
      rc = sqlite3_step(statement);
      double elapsed = GetCounter();
      int i = 0;
      avg += elapsed;
      if (rc == SQLITE_ROW)
      {
        nbreathsRead++;
        int breathId = sqlite3_column_int(statement, 0);
        int timestampId = sqlite3_column_int(statement, 1);
        TBreathData breath = { 0 };
        memcpy(&breath, sqlite3_column_blob(statement, 2), sizeof(TBreathData));
        fprintf(fp, "%d\t%d\t%d\t%d\t%d\n", breathId, timestampId, breath.peakPressure, breath.peep, breath.maxTidalVolume);
      }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(statement);
  }
  if (rc == SQLITE_DONE)
    rc = SQLITE_OK;
  if (fp)
    fclose(fp);
  avg /= (double)nbreathsRead;
  m_outputCtrl.GetWindowTextW(outText);
  CStringA elapsedStr; elapsedStr.Format(("done: %.6f s"), avg);
  outText += elapsedStr;
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
}




unsigned int getDate(unsigned int timestamp)
{
  struct tm tms;
  time_t t = timestamp;
  localtime_s(&tms, &t);
  tms.tm_hour = 0;
  tms.tm_min = 0;
  tms.tm_sec = 0;
  return (unsigned int)mktime(&tms);
}
void CBreasDBGeneratorDlg::calcUsagePerDay(TUsageSessionData session, std::map<unsigned int, unsigned int>& usagePerDay)
{
  unsigned int startDate = getDate(session.startTimeDate);
  unsigned int endDate = getDate(session.timeDate);

  if (session.timeDate <= session.startTimeDate) //invalid session - don't accumulate
  {
    //TRACE_DEBUG("Invalid session from %s to %s", sStart.toString().toLatin1().data(), sEnd.toString().toLatin1().data());
    return;
  }
  if (startDate == endDate) //session is withing single day
  {
    unsigned int sessionLengthSec = session.timeDate - session.startTimeDate;
    usagePerDay[startDate] += sessionLengthSec;
    // TRACE_DEBUG("Session from %s to %s usage: %f",sStart.toString().toLatin1().data(),sEnd.toString().toLatin1().data(),
    //      sessionLengthSec/secPerHour);

  }
  else
  {
    unsigned int sessionLengthSec = endDate - session.startTimeDate; //go from start to midnight next day.
    usagePerDay[startDate] += sessionLengthSec;
    sessionLengthSec = session.timeDate - endDate; //go from midnight to session end
    usagePerDay[endDate] += sessionLengthSec;
    //TRACE_DEBUG("Session from %s to %s usage: %f",sStart.toString().toLatin1().data(),sEnd.toString().toLatin1().data(),
    //    sessionLengthSec/secPerHour);

  }
}

unsigned int CBreasDBGeneratorDlg::getSessionLengthSec(TUsageSessionData session)
{
  return session.timeDate - session.startTimeDate;
}

void CBreasDBGeneratorDlg::accumulateAvgs(TUsageSessionData session, unsigned int& avgPeep, unsigned int& avgpPeak, unsigned int& avgLeakage, unsigned int& avgVt, unsigned int totalUsage)
{
  unsigned int sessionLengthSec = getSessionLengthSec(session);
  avgpPeak += session.averagePeakPressure*sessionLengthSec;
  avgPeep += session.averagePEEP*sessionLengthSec;
  avgVt += session.averageVt*sessionLengthSec;
  avgLeakage += session.averageLeakage*sessionLengthSec;
}

int rdn(unsigned int rdnDate)
{ /* Rata Die day one is 0001-01-01 */

  struct tm tms;
  time_t t = rdnDate;
  localtime_s(&tms, &t);
  int m = tms.tm_mon;
  int y = tms.tm_year;
  int d = tms.tm_mday;
  if (m < 3)
    y--, m += 12;
  return 365 * y + y / 4 - y / 100 + y / 400 + (153 * m - 457) / 5 + d - 306;
}

#define DEFAULT_TIME        1483228800   //§ 2017-01-01 00:00:00

void CBreasDBGeneratorDlg::updateComplianceData(std::vector<TUsageSessionData> usageSessionList, unsigned int logStartTime, TComplianceDataRes* data)
{
  unsigned int totalUsage = 0;
  int daysWithUsage = 0;
  unsigned int avgPeep = 0;
  unsigned int  avgpPeak = 0;
  unsigned int  avgVt = 0;
  unsigned int  avgLeakage = 0;
  unsigned int  totalDays = 0;
  unsigned int  avgDailyUsage = 0;
  unsigned int  daysWithCompliance = 0;

  std::map<unsigned int, unsigned int> usagePerDay;

  unsigned int firstDay = INT_MAX;
  unsigned int lastDay = 0;
  unsigned int firstTimestamp = INT_MAX;
  unsigned int lastTimestamp = 0;

  for (int i = 0; i < usageSessionList.size(); i++)
  {
    if (usageSessionList[i].startTimeDate > usageSessionList[i].timeDate || usageSessionList[i].startTimeDate < DEFAULT_TIME || usageSessionList[i].timeDate < DEFAULT_TIME)
    {
      TRACE3("Session found with invalid timestamps. s:%d e:%d lst:%d. Session skipped. RTC time may have changed.",
       usageSessionList[i].startTimeDate, usageSessionList[i].timeDate, logStartTime);
    }
    else
    {
      calcUsagePerDay(usageSessionList[i], usagePerDay);
    
    if (usageSessionList[i].startTimeDate < firstDay)
      firstDay = getDate(usageSessionList[i].startTimeDate);
    if (usageSessionList[i].timeDate > lastDay)
      lastDay = getDate(usageSessionList[i].timeDate);
    if (usageSessionList[i].startTimeDate < firstTimestamp)
      firstTimestamp = usageSessionList[i].startTimeDate;
    if (usageSessionList[i].timeDate > lastTimestamp)
      lastTimestamp = usageSessionList[i].timeDate;
    }
  }
  if (usageSessionList.size() == 0)
  {
    firstTimestamp = logStartTime;
    lastTimestamp = logStartTime;
    firstDay = getDate(logStartTime);
    lastDay = getDate(logStartTime);
    memset(data, 0, sizeof(TComplianceDataRes));
    return; //nothing to process
  }

  totalDays = usagePerDay.size();
  if (totalDays == 0 && lastTimestamp != firstTimestamp)
    totalDays = 1;
  //TRACE_DEBUG("Compliance Log duration:  %.2f days (%s-%s)",totalDays, dLogStart.toString().toLatin1().data(),
  //        dLogLast.toString().toLatin1().data());
  if (totalDays < 0)
    totalDays = 1;
  std::map<unsigned int, unsigned int>::iterator i;
  //TRACE_DEBUG("Summary: \n");
  for (i = usagePerDay.begin(); i != usagePerDay.end(); ++i)
  {
    unsigned int usage = i->second;
    avgDailyUsage += usage / totalDays;
    float complianceSetting = 0.f;
    if (usage >= complianceSetting * 60)
      daysWithCompliance++;
    if (usage > 0)
      daysWithUsage++;
    totalUsage += usage;
  }

  for (int i = 0; i < usageSessionList.size(); i++)
  {
    accumulateAvgs(usageSessionList[i], avgPeep, avgpPeak, avgLeakage, avgVt, totalUsage);
  }

  unsigned int fullDays = (rdn(lastDay) - rdn(firstDay));
  if (totalDays > fullDays)
    fullDays = totalDays;
  if (usagePerDay.size() > fullDays)
    fullDays = unsigned int(usagePerDay.size());
  float daysWUsagePercent = 100.f*daysWithUsage / fullDays;
  if (fullDays <= 0)
    daysWUsagePercent = 0;
  if (daysWUsagePercent > 100)
  {
    daysWUsagePercent = 100;
  }
  float daysWCompliancePercent = 100.f*daysWithCompliance / daysWithUsage;
  if (daysWithUsage <= 0)
    daysWCompliancePercent = 0;
  if (daysWCompliancePercent > 100)
    daysWCompliancePercent = 100;
  if (totalUsage > 0)
  {
    avgpPeak /= totalUsage;
    avgPeep /= totalUsage;
    avgVt /= totalUsage;
    avgLeakage /= totalUsage;
  }

  data->totalUsageSec = totalUsage;
  data->totalDays = fullDays;
  data->daysWithUsage = daysWithUsage;
  data->daysWithCompliance = daysWithCompliance;
  data->averageUsageSec = avgDailyUsage;

  data->averagePEEP = avgPeep;
  data->averagepPeak = avgpPeak;
  data->averageVt = avgVt;
  data->averageLeakage = avgLeakage;
}




void CBreasDBGeneratorDlg::OnBnClickedParsepreuse()
{
  CStringA outfile;
  // szFilters is a text string that includes two file name filters:
  TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

  // Create an OPEN dialog; the default file name extension is ".db".
  CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);
  if (dbfile == NULL)
  {


    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
      outfile = fileDlg.GetPathName();
      outfile.Replace(".db", ".txt");
    }
    else
    {
      return;
    }
  }
  else
  {
    outfile = m_pathName;
    outfile.Replace(".db", ".txt");
  }
  sqlite3_stmt *statement;
  int rc = sqlite3_prepare(dbfile, READ_PREUSE_EVENTS, (int)strlen(READ_PREUSE_EVENTS), &statement, 0);
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("working");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  FILE* fp;
  fopen_s(&fp, outfile, "w+");
  if (!fp)
    return;
  fprintf(fp, "Result\tR\tC\tCircuit\tLeak\n");
  if (rc == SQLITE_OK)
  {
    do
    {
      rc = sqlite3_step(statement);
      int i = 0;

      if (rc == SQLITE_ROW)
      {
        TUsageEventData res = { 0 };
        char bytes[256] = { 0 };
        memcpy(bytes, sqlite3_column_blob(statement, 0), sizeof(TUsageEventData));
        memcpy(&res, bytes, sizeof(TUsageEventData));
        fprintf(fp, "%d\t%d\t%d\t%d\t%d\n", res.eventData1, res.eventData2, res.eventData3, res.eventData4, res.eventData5);
      }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(statement);
  }
  if (rc == SQLITE_DONE)
    rc = SQLITE_OK;
  if (fp)
    fclose(fp);
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("done");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  DisconnectDB();
}


void CBreasDBGeneratorDlg::OnBnClickedButton1()
{
  CStringA outfile;
  // szFilters is a text string that includes two file name filters:
  TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

  // Create an OPEN dialog; the default file name extension is ".db".
  CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);
  if (dbfile == NULL)
  {


    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
      outfile = fileDlg.GetPathName();
      outfile.Replace(".db", ".txt");
    }
    else
    {
      return;
    }
  }
  else
  {
    outfile = m_pathName;
    outfile.Replace(".db", ".txt");
  }
  sqlite3_stmt *statement;
  int rc = sqlite3_prepare(dbfile, READ_SETTINGS_DATA_PREP, (int)strlen(READ_SETTINGS_DATA_PREP), &statement, 0);
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("working");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  FILE* fp;
  fopen_s(&fp, outfile, "w+");
  if (!fp)
    return;
  fprintf(fp, "Result\tR\tC\tCircuit\tLeak\n");
  TSettingsData resLast = { 0 };
  TSettingsData res = { 0 };

  if (rc == SQLITE_OK)
  {
    do
    {
      rc = sqlite3_step(statement);
      int i = 0;

      if (rc == SQLITE_ROW)
      {

        char bytes[256] = { 0 };
        memcpy(bytes, sqlite3_column_blob(statement, 0), sizeof(TSettingsData));
        memcpy(&res, bytes, sizeof(TSettingsData));
        int diffAt = memcmp(&res.inspPress, &resLast.inspPress, sizeof(TSettingsData) - sizeof(ULONG) - sizeof(UBYTE));
        memcpy(&resLast, &res, sizeof(TSettingsData));
      }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(statement);
  }
  if (rc == SQLITE_DONE)
    rc = SQLITE_OK;
  if (fp)
    fclose(fp);
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("done");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  DisconnectDB();
}

int CBreasDBGeneratorDlg::prepareBreathLog(TBreathData *eventBreathData)
{
  int rc = SQLITE_OK;
  sqlite3_stmt * breathLogStm = NULL;

  if (currentBreathId == MAX_BREATH_LOG_ID - 1)
    bMaxBreathIdReached = true;
  int breathLogId = (currentBreathId + 1) % MAX_BREATH_LOG_ID;

  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_BREATH_LOG_PREP, sizeof(INSERT_OR_REPLACE_BREATH_LOG_PREP), &breathLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(breathLogStm, 1, breathLogId);
      rc = sqlite3_bind_int(breathLogStm, 2, currentSessionStartTimeStampId);
      rc = sqlite3_bind_int(breathLogStm, 3, eventBreathData->timeDate);
      rc = sqlite3_bind_blob(breathLogStm, 4, eventBreathData, sizeof(TBreathData), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(breathLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(breathLogStm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }

  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);

  currentBreathId = breathLogId;
  return rc;
}

int CBreasDBGeneratorDlg::prepareTrendLog(TTrendDataHolder* trend)
{
  int rc = SQLITE_OK;
  sqlite3_stmt * trendLogStm = NULL;
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_TREND_LOG_PREP, sizeof(INSERT_OR_REPLACE_TREND_LOG_PREP), &trendLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(trendLogStm, 1, trend->samplesPerPixel);
      rc = sqlite3_bind_int(trendLogStm, 2, trend->head);
      rc = sqlite3_bind_int(trendLogStm, 3, trend->tail);
      rc = sqlite3_bind_int(trendLogStm, 4, trend->oldestTimeStamp);
      rc = sqlite3_bind_blob(trendLogStm, 5, trend->values, sizeof(trend->values), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(trendLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(trendLogStm);
    }

    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);

  return rc;
}

int CBreasDBGeneratorDlg::prepareSettingsLog(TSettingsData *settingsData)
{
  int rc;
  sqlite3_stmt * settingsLogStm = NULL; // Prepared statement that insert a settingsLog item in the log database
  if (currentSettingsId == MAX_SETTINGS_LOG_ID - 1)
    bMaxSettingsIdReached = true;
  int settingsLogId = (currentSettingsId + 1) % MAX_SETTINGS_LOG_ID;
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_SETTINGS_LOG_PREP, sizeof(INSERT_OR_REPLACE_SETTINGS_LOG_PREP), &settingsLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(settingsLogStm, 1, settingsLogId);
      rc = sqlite3_bind_int(settingsLogStm, 2, currentTimeStampId);
      rc = sqlite3_bind_int(settingsLogStm, 3, settingsData->timeDate);
      rc = sqlite3_bind_blob(settingsLogStm, 4, settingsData, sizeof(TSettingsData), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(settingsLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(settingsLogStm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);

  currentSettingsId = settingsLogId;
  return rc;
}

int CBreasDBGeneratorDlg::prepareLevelThreeDBEntry(const char* fn, UINT32 createTime, UINT32 closeTime, UINT32 fileSize)
{
  int rc;
  sqlite3_stmt * stm = NULL; // Prepared statement that insert a settingsLog item in the log database
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  if (closeTime == 0)
  {
    closeTime = createTime; //duration is zero for now - will be filled in when the file is closed.
  }
  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_LEVEL_THREE_ENTRY, sizeof(INSERT_LEVEL_THREE_ENTRY), &stm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_text(stm, 1, fn, (int)strlen(fn), SQLITE_TRANSIENT);
      rc = sqlite3_bind_int(stm, 2, createTime);
      rc = sqlite3_bind_int(stm, 3, closeTime - createTime);
      rc = sqlite3_bind_int(stm, 4, fileSize);
      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(stm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(stm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);

  return rc;
}

int CBreasDBGeneratorDlg::prepareTimeStampData(ULONG timeDate, UWORD milliSeconds, TTimeStampType timeStampType, UBYTE logVersion)
{
  int rc;
  sqlite3_stmt * timeStampStm = NULL; // Prepared statement that insert a timestamp in the log database

  if (currentTimeStampId == MAX_TIMESTAMP_LOG_ID - 1)
    bMaxTimeStampIdReached = true;
  int timeStampId = ((currentTimeStampId + 1) % MAX_TIMESTAMP_LOG_ID);
  if (timeStampType == SESSION_START || timeStampType == SESSION_RESTART || timeStampType == SESSION_24H)
  {
    if (timeStampId <= 1)
      timeStampId = 2; //don't overwrite the creation timestamp
    currentSessionStartTimeStampId = timeStampId;
  }
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_TIME_STAMP_PREP, sizeof(INSERT_OR_REPLACE_TIME_STAMP_PREP), &timeStampStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(timeStampStm, 1, timeStampId);
      rc = sqlite3_bind_int(timeStampStm, 2, timeDate);
      rc = sqlite3_bind_int(timeStampStm, 3, milliSeconds);
      rc = sqlite3_bind_int(timeStampStm, 4, timeStampType);
      rc = sqlite3_bind_int(timeStampStm, 5, logVersion);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(timeStampStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(timeStampStm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);
  currentTimeStampId = timeStampId;
  return rc;
}

int CBreasDBGeneratorDlg::prepareUsageEvent(ILONG timeDate, UWORD milliSeconds, TRecordType recordType, UINT8 recordId, TUsageEventData *data)
{
  int rc;
  sqlite3_stmt* usageEventLocalStm = NULL;

  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }
  if (currentEventId == MAX_EVENT_LOG_ID - 1)
    bMaxEventIdReached = true;
  int eventLogId = (currentEventId + 1) % MAX_EVENT_LOG_ID;
  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_EVENT_PREP, sizeof(INSERT_OR_REPLACE_EVENT_PREP), &usageEventLocalStm, NULL);
    if (rc == SQLITE_OK)
    {

      rc = sqlite3_bind_int(usageEventLocalStm, 1, eventLogId);
      rc = sqlite3_bind_int(usageEventLocalStm, 2, currentTimeStampId);
      rc = sqlite3_bind_int(usageEventLocalStm, 3, timeDate);
      rc = sqlite3_bind_int(usageEventLocalStm, 4, milliSeconds);
      rc = sqlite3_bind_int(usageEventLocalStm, 5, recordType);
      rc = sqlite3_bind_int(usageEventLocalStm, 6, recordId);
      sqlite3_bind_blob(usageEventLocalStm, 7, data, sizeof(TUsageEventData), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(usageEventLocalStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(usageEventLocalStm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);
  currentEventId = eventLogId;
  return rc;
}

int CBreasDBGeneratorDlg::prepareUsageSessionLog(TUsageSessionData *eventUsageSessionData)
{
  int rc;
  /** Prepared statement string used to insert item in UsageSessionLog table */
  sqlite3_stmt * usageSessionLogStm = NULL;
  if (currentSessionId == MAX_SESSION_LOG_ID - 1)
    bMaxSessionIdReached = true;
  int sessionId = (currentSessionId + 1) % MAX_SESSION_LOG_ID;
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, INSERT_OR_REPLACE_USAGE_SESSION_LOG_PREP, sizeof(INSERT_OR_REPLACE_USAGE_SESSION_LOG_PREP), &usageSessionLogStm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(usageSessionLogStm, 1, sessionId);
      rc = sqlite3_bind_int(usageSessionLogStm, 2, currentTimeStampId);
      rc = sqlite3_bind_int(usageSessionLogStm, 3, eventUsageSessionData->timeDate);
      rc = sqlite3_bind_blob(usageSessionLogStm, 4, eventUsageSessionData, sizeof(TUsageSessionData), SQLITE_TRANSIENT);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(usageSessionLogStm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(usageSessionLogStm);
    }
    if (rc == SQLITE_OK)
    {
      checkEndTransaction(0);
    }
  }
  else
    sqlite3_exec(dbfile, "ROLLBACK", NULL, NULL, NULL);
  currentSessionId = sessionId;
  return rc;
}

int CBreasDBGeneratorDlg::checkBeginTransaction()
{
  int rc = SQLITE_OK;
  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }
  if (!bTransactionBegun)
  {
    bTransactionBegun = true;
    rc = sqlite3_exec(dbfile, "BEGIN TRANSACTION", NULL, NULL, NULL);
  }
  else
  {
    openInsertCount++;
  }
  return rc;
}

void CBreasDBGeneratorDlg::checkEndTransaction(bool bForce)
{
  if (!bTransactionBegun || dbfile == NULL)
    return;
  if (openInsertCount > MAX_OPEN_INSERTS || bForce)
  {
    updateCurrentIds();
    sqlite3_exec(dbfile, "END TRANSACTION", NULL, NULL, NULL);
    bTransactionBegun = false;
    openInsertCount = 0;
  }
}

int CBreasDBGeneratorDlg::updateCurrentIds()
{
  int rc = SQLITE_OK;
  sqlite3_stmt * stm = NULL;

  if (dbfile == NULL)
  {
    return SQLITE_ERROR;
  }

  rc = checkBeginTransaction();
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_prepare_v2(dbfile, UPDATE_LAST_IDS, sizeof(UPDATE_LAST_IDS), &stm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_bind_int(stm, 1, currentTimeStampId);
      rc = sqlite3_bind_int(stm, 2, currentBreathId);
      rc = sqlite3_bind_int(stm, 3, currentSettingsId);
      rc = sqlite3_bind_int(stm, 4, currentEventId);
      rc = sqlite3_bind_int(stm, 5, currentSessionId);
      rc = sqlite3_bind_int(stm, 6, currentSessionStartTimeStampId);
      rc = sqlite3_bind_int(stm, 7, bMaxTimeStampIdReached);
      rc = sqlite3_bind_int(stm, 8, bMaxBreathIdReached);
      rc = sqlite3_bind_int(stm, 9, bMaxSettingsIdReached);
      rc = sqlite3_bind_int(stm, 10, bMaxEventIdReached);
      rc = sqlite3_bind_int(stm, 11, bMaxSessionIdReached);

      if (rc == SQLITE_OK)
      {
        rc = sqlite3_step(stm);
        rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
      }

      sqlite3_finalize(stm);
    }
  }

  return rc;
}


void CBreasDBGeneratorDlg::getCurrentIds()
{
  if (dbfile != NULL)
  {
    int rc;
    sqlite3_stmt * stm = NULL;

    rc = sqlite3_prepare_v2(dbfile, GET_LAST_IDS, sizeof(GET_LAST_IDS), &stm, NULL);
    if (rc == SQLITE_OK)
    {
      rc = sqlite3_step(stm);
      if (rc == SQLITE_ROW)
      {
        currentTimeStampId = (UINT32)sqlite3_column_int(stm, 0);
        currentBreathId = (UINT32)sqlite3_column_int(stm, 1);
        currentSettingsId = (UINT32)sqlite3_column_int(stm, 2);
        currentEventId = (UINT32)sqlite3_column_int(stm, 3);
        currentSessionId = (UINT32)sqlite3_column_int(stm, 4);
        currentSessionStartTimeStampId = (UINT32)sqlite3_column_int(stm, 5);
        bMaxTimeStampIdReached = (sqlite3_column_int(stm, 6)) != 0;
        bMaxBreathIdReached = sqlite3_column_int(stm, 7) != 0;
        bMaxSettingsIdReached = sqlite3_column_int(stm, 8) != 0;
        bMaxEventIdReached = sqlite3_column_int(stm, 9) != 0;
        bMaxSessionIdReached = sqlite3_column_int(stm, 10) != 0;
        /* TRACE_DEBUG("***%s ts%d %d b%d %d e%d %d s%d %d ss%d %d", __FUNCTION__, currentTimeStampId, bMaxTimeStampIdReached, currentBreathId, bMaxBreathIdReached,
           currentEventId, bMaxEventIdReached, currentSettingsId, bMaxSettingsIdReached, currentSessionId, bMaxSessionIdReached);*/
      }
      sqlite3_finalize(stm);
    }
  }
}
UBYTE UBYTEfromBuf(UBYTE* data)
{
  UBYTE outval = data[0];
  return outval;
}
ULONG ULONGfromBuf(UBYTE* data)
{
  ULONG outval = outval = data[0];
  outval += data[1] << 8;
  outval += data[2] << 16;
  outval += data[3] << 24;
  return outval;
}
UWORD UWORDfromBuf(UBYTE* data)
{
  UWORD outval = outval = data[0];
  outval += data[1] << 8;
  return outval;
}
IWORD IWORDfromBuf(UBYTE* data)
{
  IWORD outval = outval = data[0];
  outval += data[1] << 8;
  return outval;
}

void CBreasDBGeneratorDlg::OnBnClickedLevel3()
{
  CStringA outfile;
  CString infile;
  // szFilters is a text string that includes two file name filters:
  TCHAR szFilters[] = _T("V45 Files (*.V45)|*.V45|All Files (*.*)|*.*||");

  // Create an OPEN dialog; the default file name extension is ".db".
  CFileDialog fileDlg(TRUE, _T("V45"), _T("*.V45"),
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);
  // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
  if (fileDlg.DoModal() == IDOK)
  {
    infile = fileDlg.GetPathName();
    outfile = infile;// "C:\\Users\\rabur\\Desktop\\dummyfile.txt";
    outfile.Replace(".V45", ".txt");
    outfile.Replace("_B", "");
    CFile fp;
    FILE* fpo = NULL;
    CFileException e;
    UBYTE data[31];
    fopen_s(&fpo, outfile, "w+");
    if (fp.Open(infile, CFile::modeRead) && fpo != NULL)
    {

      char buf[512];
      int nRead = fp.Read(&buf, 512);
      fprintf(fpo, "MODEL:%sSerial Number:%s\n", &buf[0], &buf[24]);
      fprintf(fpo, "Profile1Name%s\n", &buf[48]);
      fprintf(fpo, "Profile2Name%s\n", &buf[60]);
      fprintf(fpo, "Profile3Name%s\n", &buf[72]);
      fprintf(fpo, "DatabaseVersion:%s", &buf[84]);
      fprintf(fpo, "LogLevel:%s", &buf[85]);

      
      int i = 0;
      ULONGLONG length = fp.GetLength();
      ULONGLONG nTotalPackets = (length - 512) / 31;
      fprintf(fpo, "Approximate length: %lld packets (%lf s)", nTotalPackets, (double)nTotalPackets / 62.5);
      int ts = 0;
      int ms = 0;
      fprintf(fpo, "\ntime,breathId,patientPressure,patientFlow,totalFlow,tidalVolume,momentaryCO2,atmP,fio2Perc,trigger,sigh,leakage,thoracic,abdominal,breathFlag\n");

      while (fp.Read(&data, 31))
      {
        TRealtimeData packet = { 0 };
        packet.timestamp = ULONGfromBuf(&data[0]);
        packet.ms = UBYTEfromBuf(&data[4]);
        int breathId = ULONGfromBuf(&data[5]);
        packet.patientPressure = UWORDfromBuf(&data[9]);
        packet.patientFlow = IWORDfromBuf(&data[11]);
        packet.totalFlow = IWORDfromBuf(&data[13]);
        packet.tidalVolume = UWORDfromBuf(&data[15]);
        packet.momentaryCO2 = UWORDfromBuf(&data[17]);
        packet.atmosphericP = UWORDfromBuf(&data[19]);
        packet.fIO2Perc = UBYTEfromBuf(&data[21]);
        packet.trigger = UBYTEfromBuf(&data[22]);
        packet.sigh = UBYTEfromBuf(&data[23]);
        packet.leakage = UWORDfromBuf(&data[24]);
        packet.thoracic = IWORDfromBuf(&data[26]);
        packet.abdominal = IWORDfromBuf(&data[28]);
        packet.breathFlag = UBYTEfromBuf(&data[30]);

        fprintf(fpo, "%d.%d,", packet.timestamp, packet.ms * 4);
        fprintf(fpo, "%d,", breathId);
        fprintf(fpo, "%d,", packet.patientPressure);
        fprintf(fpo, "%d,", packet.patientFlow);
        fprintf(fpo, "%d,", packet.totalFlow);
        fprintf(fpo, "%d,", packet.tidalVolume); 
        fprintf(fpo, "%d,", packet.momentaryCO2); 
        fprintf(fpo, "%d,", packet.atmosphericP);
        fprintf(fpo, "%d,", packet.fIO2Perc); 
        fprintf(fpo, "%d,", packet.trigger); 
        fprintf(fpo, "%d,", packet.sigh); 
        fprintf(fpo, "%d,", packet.leakage); 
        fprintf(fpo, "%d,", packet.thoracic);
        fprintf(fpo, "%d,", packet.abdominal); 
        fprintf(fpo, "%d\n", packet.breathFlag); 


        i++;
      }
      fp.Close();

      fclose(fpo);

    }
  }
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("done");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
}


static int matchCallback(void *output, int count, char **data, char **columns)
{
  if (*data == NULL)
    return 1;
  if (count == 2 /*&& *data != NULL)*/)
  {
    TDateTime* dt = (TDateTime*)output;
    dt->l_CTime = atoi(data[0]);
    dt->ui_TimeMs = atoi(data[1]);
  }
  return 0;
}

int CBreasDBGeneratorDlg::GetSessionStopTimeStamp(UWORD timeStampId, ULONG* timeDate, UWORD* milliSeconds, TTimeStampType* type)
{
  int rc = SQLITE_OK;
  const char ts[] = "SELECT timeDate,milliSeconds,timeStampType FROM TimeStamp WHERE timeStampId = ?";
  sqlite3_stmt * sessionStopLogStm = NULL;
  if (dbfile == NULL)
    return SQLITE_ERROR;

  rc = sqlite3_prepare_v2(dbfile, ts, sizeof(ts), &sessionStopLogStm, NULL);
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_bind_int(sessionStopLogStm, 1, timeStampId);
    if (rc == SQLITE_OK)
    {
      do
      {
        rc = sqlite3_step(sessionStopLogStm);
        if (rc == SQLITE_ROW)
        {
          int i = 0;
          *timeDate = sqlite3_column_int(sessionStopLogStm, i++);
          *milliSeconds = sqlite3_column_int(sessionStopLogStm, i++);
          *type = (TTimeStampType)sqlite3_column_int(sessionStopLogStm, i++);
        }

      } while (rc == SQLITE_ROW);

      rc = rc != SQLITE_DONE ? rc : SQLITE_OK;
    }
    sqlite3_finalize(sessionStopLogStm);
  }

  return rc;
}



void CBreasDBGeneratorDlg::readUsageSessionRecord(UWORD maxEvents, CStringA outfile)
{
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("working");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
  RedrawWindow();
  FILE* fp;
  fopen_s(&fp, outfile, "w+");
  if (!fp)
    return;
  fprintf(fp, "TSId\tStart Time\tEnd Time\tduration\tpPeak\tPEEP\tvT\taverageMV\tpatientTriggeredBreathPercent\taverageEtCO2Perc\t\n");
  int nSessionsRead = 0;
  char ts[256];
  TDateTime t;
  std::vector<TUsageSessionData> usageSessions;
  unsigned int logStartTime = 0;
  int startWrap = 0;
  int endWrap = 0;
  getCurrentIds();
  int startBeginning = currentSessionId - maxEvents;
  int endBeginning = currentSessionId;
  if (startBeginning < 0 && bMaxEventIdReached)
  {
    startWrap = MAX_SESSION_LOG_ID + startBeginning; //get X sessions from before the wrap (startBeginning is negative)
    endWrap = MAX_SESSION_LOG_ID;
    startBeginning = 0;
  }
  else if (startBeginning < 0)
  {
    startBeginning = 0;
  }
  else
  {
    startWrap = MAX_SESSION_LOG_ID + 1;
    endWrap = MAX_SESSION_LOG_ID + 1;
  }
  sprintf_s(ts, 256, "SELECT timeDate,milliSeconds from TimeStamp WHERE timestampType=7 OR timeStampType=5"); //find the log start time
  if (!sqlite3_exec(dbfile, ts, matchCallback, &t, NULL))
    logStartTime = t.l_CTime;
  sqlite3_stmt *statement;
  int rc = sqlite3_prepare(dbfile, READ_X_LATEST_SESSION_LOG_PREP_NEW, (int)strlen(READ_X_LATEST_SESSION_LOG_PREP_NEW), &statement, 0);
  rc = sqlite3_bind_int(statement, 1, startBeginning);
  rc = sqlite3_bind_int(statement, 2, endBeginning);
  rc = sqlite3_bind_int(statement, 3, startWrap);
  rc = sqlite3_bind_int(statement, 4, endWrap);
  rc = sqlite3_bind_int(statement, 5, logStartTime);

  if (rc == SQLITE_OK)
  {
    TUsageSessionData usageSessionData;

    do
    {
      rc = sqlite3_step(statement);
      if (rc == SQLITE_ROW)
      {
        int i = 0;

        nSessionsRead++;
        int test = sizeof(TUsageSessionData);
        memset(&usageSessionData, 0,sizeof(TUsageSessionData));
        memcpy(&usageSessionData, sqlite3_column_blob(statement, 0), sizeof(TUsageSessionData));
        time_t tS = usageSessionData.startTimeDate;
        time_t tE = usageSessionData.timeDate;
        struct tm tms;
        localtime_s(&tms, &tS);
        char bufS[255];
        strftime(bufS, 32, "%m/%d/%Y %H:%M:%S", &tms);
        char bufE[255];
        localtime_s(&tms, &tE);
        strftime(bufE, 32, "%m/%d/%Y %H:%M:%S", &tms);
        float length = (tE - tS + (usageSessionData.milliSeconds - usageSessionData.msStart) / 1000.f) / 3600.f;
        /*fprintf(fp, "%d\t%s\t%s\t%.3f\t%d\t%d\t%d\t%d\t%d\t%d\n", timeStampId, bufS, bufE, length, usageSessionData.averagePeakPressure, usageSessionData.averagePEEP, usageSessionData.averageVt,
          usageSessionData.averageMV, usageSessionData.patientTriggeredBreathPercent, usageSessionData.averageEtCO2);*/
        fprintf(fp, "{%d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d,0,0,0,0}\n", usageSessionData.startTimeDate, usageSessionData.msStart, usageSessionData.timeDate, usageSessionData.milliSeconds,
          usageSessionData.averagePeakPressure, usageSessionData.averagePEEP, usageSessionData.averageVt,
          usageSessionData.averageMV, usageSessionData.patientTriggeredBreathPercent, usageSessionData.averageEtCO2, usageSessionData.averageAtmP, usageSessionData.averageLeakage);
        usageSessions.push_back(usageSessionData);


      }
    } while (rc == SQLITE_ROW);
    TComplianceDataRes data = { 0 };
    data.startOfLog = logStartTime;
    time_t tSL = data.startOfLog;
    struct tm tmsL;
    localtime_s(&tmsL, &tSL);
    char bufSL[255];
    strftime(bufSL, 32, "%m-%d-%Y", &tmsL);
    fprintf(fp, "\n\nStart of Log: %s", bufSL);
    updateComplianceData(usageSessions, logStartTime, &data);
    fprintf(fp, "\nUH\tD\tDWU\tDWUP\tmUH\tDC\tDCP\tmIP\tmEP\tavVt\tavLk");
    fprintf(fp, "\n%.1f\t%d\t%d\t%.1f\t%.1f\t%d\t%.1f\t%.1f\t%.1f\t%.1f\t%.0f", (float)(data.totalUsageSec / 3600.f),
      data.totalDays,
      data.daysWithUsage,
      (float)(data.daysWithUsage * 100.f / data.totalDays),
      (float)(data.averageUsageSec / 3600.f),
      data.daysWithCompliance,
      (float)(data.daysWithCompliance * 100 / data.totalDays),
      (float)(data.averagepPeak / 100.f),
      (float)(data.averagePEEP / 100.f),
      (float)(data.averageVt),
      (float)(data.averageLeakage));
    if (fp)
      fclose(fp);
    m_outputCtrl.GetWindowTextW(outText);
    outText += _T("done");
    outText += "\r\n";
    m_outputCtrl.SetWindowText(outText);
    RedrawWindow();
  }
}

void CBreasDBGeneratorDlg::OnBnClickedParseSessions()
{
  CStringA outfile;
  if (dbfile == NULL)
  {
    // szFilters is a text string that includes two file name filters:
    TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

    // Create an OPEN dialog; the default file name extension is ".db".
    CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
      OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
      outfile = fileDlg.GetPathName();
      outfile.Replace(".db", ".txt");
    }
    else
    {
      return;
    }
  }
  else
  {
    outfile = m_pathName;
    outfile.Replace(".db", ".txt");
  }
  readUsageSessionRecord(3500, outfile);
}

#pragma warning( disable : 4996)
void CBreasDBGeneratorDlg::OnBnClickedParseLevel1()
{
  /*  CStringA outfile;
   CString infile;
   // szFilters is a text string that includes two file name filters:
   TCHAR szFilters[] = _T("V45 Files (*.V45)|*.V45|All Files (*.*)|*.*||");

   // Create an OPEN dialog; the default file name extension is ".db".
   CFileDialog fileDlg(TRUE, _T("V45"), _T("*.V45"),
     OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);
   // Display the file dialog. When user clicks OK, fileDlg.DoModal()
   // returns IDOK.
   if (fileDlg.DoModal() == IDOK)
   {
     infile = fileDlg.GetPathName();
     outfile = infile;// "C:\\Users\\rabur\\Desktop\\dummyfile.txt";
     outfile.Replace(".V45", ".txt");
     CFile fp;
     FILE* fpo = NULL;
     CFileException e;
     UBYTE* data;
     fopen_s(&fpo, outfile, "w+");

     if (fp.Open(infile, CFile::modeRead) && fpo != NULL)
     {

       UBYTE buf[512];
       int nRead = fp.Read(&buf, 512);
       fprintf(fpo, "MODEL:%sSerial Number:%s\n", &buf[0], &buf[24]);
       fprintf(fpo, "Profile1Name%s\n", &buf[48]);
       fprintf(fpo, "Profile2Name%s\n", &buf[60]);
       fprintf(fpo, "Profile3Name%s\n", &buf[72]);
       fprintf(fpo, "DatabaseVersion:%s", &buf[84]);
       fprintf(fpo, "LogLevel:%s", &buf[85]);


      int i = 0;
       int j = 0;
       ULONGLONG length = fp.GetLength()-512;
       std::vector<timeStampFromLog> timestamps;
       if (length > 0)
       {
         fprintf(fpo, "Timestamp log :");
         fprintf(fpo, "TimeStampId timeDate milliSeconds timestamptype logversion");
         fprintf(fpo,"EventLog:\n");
         fprintf(fpo,"usageEventId timeStampId timeDate milliSeconds recordType recordId content d0 d1 d2 d3 d4 d5 d6 d7\n");
         data = new UBYTE[length];
         fp.Read(data, (UINT)length);
         while (j < length)
         {
           if (data[j] == 2) //start byte
           {
             if (data[j + 1] == 4)
             {  //timestamp
               j += 2;
               timeStampFromLog ts;
               ts.timestampId = ULONGfromBuf(&data[j]);
               ts.ts = time_t(ULONGfromBuf(&data[j+4]));
               ts.ms = data[j + 8];
               ts.tsType = data[j + 9];
               ts.ver = data[j + 10];
               time_t t = time_t(ts.ts);
               fprintf(fpo, "%d %s.%d %d %d",ts.timestampId,ctime(&t),ts.ms*4,ts.tsType,ts.ver);
               j += (sizeof(timeStampFromLog)+1); //advance past stop byte
             }
             else if (data[j + 1] == 5) //session
             {
               fprintf(fpo, "");
               j += sizeof(usageSessionFromLog);
             }
             else if (data[j + 1] == 6) //event
             {
               j += 2;
               usageEventFromLog ue;
               ue.usageLogId = ULONGfromBuf(&data[j]);
               ue.timestampId = ULONGfromBuf(&data[j + 4]);
               ue.ts = time_t(ULONGfromBuf(&data[j + 8]));
               ue.ms = data[j + 12];
               ue.recordType = data[j + 13];
               ue.recordId = data[j + 14];
               time_t t = time_t(ue.ts);
               CStringA outStr;
               UINT8 extra = unpackEvent(ue.recordType, ue.recordId, &data[15],outStr);
               fprintf(fpo, "%d %d %s.%d %d %d %s", ue.usageLogId, ue.timestampId, ctime(&t), ue.ms * 4, ue.recordType,ue.recordId,outStr.GetBuffer());
               j += (sizeof(usageEventFromLog) + extra + 1); //advance past stop byte

             }
             else if (data[j + 1] == 7) //setting
             {
               j += 16;
             }
             else
             {
               TRACE1("%d", data[j + 1]);
               do {
                 j++;
               } while (data[j] != 3 && data[j + 1] != 2);
             }
           }
         }
       }

       fp.Close();

       fclose(fpo);

     }
   }
   CString outText;
   m_outputCtrl.GetWindowTextW(outText);
   outText += _T("done");
   outText += "\r\n";
   m_outputCtrl.SetWindowText(outText);*/
}


/** The position of the serial number in the log header */
#define HEADER_SERIAL_NUMBER_POS  24
/** The start position of the profiles in the log file header */
#define HEADER_PROFILE_START_POS  48
/** The position of the log version in the log header */
#define HEADER_VERSION_POS        84
/** The position level type in the log header */
#define HEADER_LEVEL_TYPE_POS     85
#define HEADER_SIZE 512
UINT16 addHeader(UINT8* buf, int loglevel)
{
  char gProfile[3][9] = { "Profile1","Profile2","Profile3" };
  sprintf((char*)buf, "%s", "VIVO45"); //24 bytes with /0
  sprintf((char*)&buf[HEADER_SERIAL_NUMBER_POS], "%s", "VD15"); //use the globally stored device Serial number
  int i = 0;
  for (i = 0; i < 3; i++)
  {
    sprintf((char*)&buf[HEADER_PROFILE_START_POS + (i * 12)], "%s", gProfile[i]); //12 bytes with /0
  }
  buf[HEADER_VERSION_POS] = LOG_DB_VERSION & 0xFF;

  switch (loglevel)
  {
  case 1:
    buf[HEADER_LEVEL_TYPE_POS] = '1';
    break;
  case 2:
    buf[HEADER_LEVEL_TYPE_POS] = '2';
    break;
  case 3:
    buf[HEADER_LEVEL_TYPE_POS] = '3';
    break;
  default:
    break;
  }

  return HEADER_SIZE;
}

void addToLog(UINT8* buf, ULONG breathId, TRealtimeData realtimeData)
{
  int bufPos = 0;
  buf[bufPos++] = realtimeData.timestamp & 0xFF;
  buf[bufPos++] = (realtimeData.timestamp >> 8) & 0xFF;
  buf[bufPos++] = (realtimeData.timestamp >> 16) & 0xFF;
  buf[bufPos++] = (realtimeData.timestamp >> 24) & 0xFF;

  buf[bufPos++] = (realtimeData.ms);

  buf[bufPos++] = breathId & 0xFF;
  buf[bufPos++] = (breathId >> 8) & 0xFF;
  buf[bufPos++] = (breathId >> 16) & 0xFF;
  buf[bufPos++] = (breathId >> 24) & 0xFF;

  buf[bufPos++] = realtimeData.patientPressure & 0xFF;
  buf[bufPos++] = (realtimeData.patientPressure >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.patientFlow & 0xFF;
  buf[bufPos++] = (realtimeData.patientFlow >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.totalFlow & 0xFF;
  buf[bufPos++] = (realtimeData.totalFlow >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.tidalVolume & 0xFF;
  buf[bufPos++] = (realtimeData.tidalVolume >> 8) & 0xFF;

  buf[bufPos++] = realtimeData.momentaryCO2 & 0xFF;
  buf[bufPos++] = (realtimeData.momentaryCO2 >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.atmosphericP & 0xFF;
  buf[bufPos++] = (realtimeData.atmosphericP >> 8) & 0xFF;

  buf[bufPos++] = realtimeData.fIO2Perc;
  buf[bufPos++] = realtimeData.trigger;
  buf[bufPos++] = realtimeData.sigh;

  buf[bufPos++] = realtimeData.leakage & 0xFF;
  buf[bufPos++] = (realtimeData.leakage >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.thoracic & 0xFF;
  buf[bufPos++] = (realtimeData.thoracic >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.abdominal & 0xFF;
  buf[bufPos++] = (realtimeData.abdominal >> 8) & 0xFF;
  buf[bufPos++] = realtimeData.breathFlag;

}
void CBreasDBGeneratorDlg::OnBnClickedFakelevel3()
{
  CFolderPickerDialog m_dlg;
  CString m_Folder;
  m_dlg.m_ofn.lpstrTitle = _T("Select folder for Level 3 files");
  if (m_dlg.DoModal() == IDOK)
  {
    m_Folder = m_dlg.GetPathName();   // Use this to get the selected folder name 
    std::vector<TUsageSessionData> sd;
    std::vector<std::string> fn;
    std::vector<ULONG> dur;
    GenerateFakeLevel3(m_Folder, sd, fn, dur);
    CString outText;
    m_outputCtrl.GetWindowTextW(outText);
    for (int i = 0; i < fn.size(); i++)
    {
      CStringA line; line.Format("%s %d\n", fn[i], sd[i].timeDate - sd[i].startTimeDate);
      outText += line;
    }
    outText += _T("done");
    outText += "\r\n";
    m_outputCtrl.SetWindowText(outText);
  }
}

#include <sstream>
void CBreasDBGeneratorDlg::GenerateFakeLevel3(CString folder, std::vector<TUsageSessionData>& sd, std::vector<std::string>& fn, std::vector<ULONG>& sz)
{
  CTime t(2019, 1, 1, 1, 0, 0); //Jan 1,2019, 1:00 AM
  struct tm tm_s;
  t.GetLocalTm(&tm_s);
  time_t endT = mktime(&tm_s);
  time_t startT = endT - 3600 - 24 * 3600;
  
  fn.resize(2);
  sd.resize(2);
  sz.resize(2);
  std::stringstream buf;
  CT2A asciiFolder(folder);
    buf << asciiFolder << "\\Breas_Level_3_" << startT << ".V45";
    fn[0] = buf.str();
    TRealtimeData rtdata = { startT,0,1500,50,100,300,30,1017,21,0,0,50,100,100,0 };
    UINT8 data[31] = { 0 };
    FILE* fp = fopen(fn[0].c_str(), "w+");
    UINT8 header[512] = { 0 };
    addHeader(header, 3);
    //Create two files - one an hour long and the other (later) 23.5 hours long)
    //we currently sample every 16 ms - 1 hr of data is 60 min/hr * 60 sec/min * 62.5 samples/sec = 225000 samples/hour
    ULONG breathId = 0;
    if (fp)
    {
      fwrite(header, 512, 1, fp);
      sd[0].startTimeDate = startT;
      for (int i = 0; i < 225000; i++)
      {
        rtdata.ms += 4;
        if(rtdata.ms >= 1000)
          rtdata.timestamp++;
        rtdata.ms = rtdata.ms % 1000;
        if (i % 65 == 0)
          rtdata.breathFlag = !rtdata.breathFlag;
        if (i % 130 == 0)
          breathId++;
        addToLog(data, breathId, rtdata);

        fwrite(data, 1,31, fp);
      }
      sd[0].timeDate = rtdata.timestamp;
      sz[0] = 225000 * 31+512;
      fclose(fp);
    }
    buf.clear();
    std::stringstream buf2;
    buf2 << asciiFolder << "\\Breas_Level_3_" << startT + 1800 << ".V45";
    fn[1] = buf2.str();
    fp = fopen(fn[1].c_str(), "w+");
    sd[1].startTimeDate = startT + 1800;
    rtdata.timestamp = sd[1].startTimeDate;
    if (fp)
    {
      fwrite(header, 512, 1, fp);
      for (int i = 0; i < int(225000 * 23.5); i++)
      {
        rtdata.ms += 4;
        if (rtdata.ms >= 1000)
          rtdata.timestamp++;
        rtdata.ms = rtdata.ms % 1000;
        if (i % 65 == 0)
          rtdata.breathFlag = !rtdata.breathFlag;
        if (i % 130 == 0)
          breathId++;
        addToLog(data, breathId, rtdata);
        fwrite(data, 1, 31, fp);
      }
      sd[1].timeDate = sd[1].startTimeDate + 23.5 * 60 * 60;
      sz[1] = 225000 * 31 * 23.5 + 512;
      fclose(fp);
  }
}
bool CBreasDBGeneratorDlg::checkColumnNames(sqlite3_stmt* stmt, const char* queryStr)
{
  bool status = 0;
  char queryCopy[2048];
  strcpy(queryCopy, queryStr);
  int col = 0;
  char * columnName = strtok(queryCopy, " ,");
  do
  {
    columnName = strtok(NULL, " ,"); //skip the 'select'

    const char * text = sqlite3_column_name(stmt, col++);
    if (text != NULL && strcmp((const char*)text, (const char*)columnName) != 0)
    {
      if (strcmp("updated", (const char*)text) != 0 && strcmp("FROM", columnName) != 0)
      {
        status = SQLITE_ERROR;
        TRACE3("column name check ERROR: %d %s / %s", col, text, columnName);
      }

    }

  } while (columnName != NULL);

  if (status != SQLITE_OK)
    TRACE1("Settings check FAIL %s", queryStr);

  return status;

}

void CBreasDBGeneratorDlg::OnBnClickedReadsettings()
{
  CStringA outfile;
  if (dbfile == NULL)
  {
    // szFilters is a text string that includes two file name filters:
    TCHAR szFilters[] = _T("DB Files (*.db)|*.db|All Files (*.*)|*.*||");

    // Create an OPEN dialog; the default file name extension is ".db".
    CFileDialog fileDlg(TRUE, _T("db"), _T("*.db"),
      OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if (fileDlg.DoModal() == IDOK)
    {
      if (!ConnectDB(CStringA(fileDlg.GetPathName())))
      {
        return;
      }
      outfile = fileDlg.GetPathName();
      outfile.Replace(".db", ".txt");
    }
    else
    {
      return;
    }
  }
  else
  {
    outfile = m_pathName;
    outfile.Replace(".db", ".txt");
  }
  readSettings(3, outfile);
}

void CBreasDBGeneratorDlg::readSettings(UWORD version, CStringA outfile)
{
  /*int rc = SQLITE_OK;
  INT i = 0;
  int nRowsRead = 0;
  static sqlite3_stmt * otherParamReadStm = NULL;

  //as the dB version is incremented, add entries in the table below. If the particular
  //table doesn't change between versions, just add a duplicate entry
  const char* const readStringVer = version < SETTING_DB_VERSION ?  READ_OTHER_PREP;

  rc = sqlite3_prepare_v2(dbfile, readStringVer, strlen(readStringVer), &otherParamReadStm, NULL);

  if (rc == SQLITE_OK)
  {
    do
    {
      i = 0;
      rc = sqlite3_step(otherParamReadStm);
      if (checkColumnNames(otherParamReadStm, readStringVer) != SQLITE_OK)
      {
        sqlite3_finalize(otherParamReadStm);
        return;// SQLITE_ERROR;
      }

      if (rc == SQLITE_ROW)
      {
        TOtherParameters other;
        other.languageIdClinical = sqlite3_column_int(otherParamReadStm, i++);
        other.languageIdHome = sqlite3_column_int(otherParamReadStm, i++);
        other.languageId1 = sqlite3_column_int(otherParamReadStm, i++);
        other.wifi = sqlite3_column_int(otherParamReadStm, i++);
        other.bluetooth = sqlite3_column_int(otherParamReadStm, i++);
        other.displayLight = (TDispLight)sqlite3_column_int(otherParamReadStm, i++);
        other.lightIntensity = sqlite3_column_int(otherParamReadStm, i++);
        other.keypadLock = sqlite3_column_int(otherParamReadStm, i++);
        other.circuitRecognition = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.currentCircuitId = (TCircuitType)sqlite3_column_int(otherParamReadStm, i++);
        other.tempCircuit = (TCircuitType)sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.circuitResistance = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.circuitCompliance = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.humidifier = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.heatedWire = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.heatedWireF = 810;
        other.treatment.heatedWireF = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.humidifierOnOff = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.heatedWireOnOff = sqlite3_column_int(otherParamReadStm, i++);
        other.alarmSoundLevel = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.patientMode = (TPatientMode)sqlite3_column_int(otherParamReadStm, i++);
        other.deviceMode = (TDeviceMode)sqlite3_column_int(otherParamReadStm, i++);
        other.profile1Activated = sqlite3_column_int(otherParamReadStm, i++);
        other.profile2Activated = sqlite3_column_int(otherParamReadStm, i++);
        other.profile3Activated = sqlite3_column_int(otherParamReadStm, i++);
        other.selectedProfile = (TProfileNumber)sqlite3_column_int(otherParamReadStm, i++);
        other.adjustmentInHome = sqlite3_column_int(otherParamReadStm, i++);
        other.pressureUnit = sqlite3_column_int(otherParamReadStm, i++);
        other.treatment.etCO2Unit = sqlite3_column_int(otherParamReadStm, i++);
        other.curvePressureScale = sqlite3_column_int(otherParamReadStm, i++);
        other.curveFlowScale = sqlite3_column_int(otherParamReadStm, i++);
        other.curveVolumeScale = sqlite3_column_int(otherParamReadStm, i++);
        other.curveTimeBase = sqlite3_column_int(otherParamReadStm, i++);
        other.curveCo2Scale = sqlite3_column_int(otherParamReadStm, i++);
        other.trendRateScale = sqlite3_column_int(otherParamReadStm, i++);
        other.trendLeakageScale = sqlite3_column_int(otherParamReadStm, i++);
        other.trendTimeBase = sqlite3_column_int(otherParamReadStm, i++);
        other.useStaticIP = sqlite3_column_int(otherParamReadStm, i++);
        other.timeFormat = (TTimeFormat)sqlite3_column_int(otherParamReadStm, i++);
        other.dateFormat = (TDateFormat)sqlite3_column_int(otherParamReadStm, i++);
        if (version >= 2)
        {
          other.temperatureUnit = sqlite3_column_int(otherParamReadStm, i++);
        }
        else
        {
          other.temperatureUnit = 0;
        }
        other.complianceTime = sqlite3_column_int(otherParamReadStm, i++);

        nRowsRead++;
      }
      else if (rc == SQLITE_ERROR)
      {
        TRACE1("Error reading other parameters, parameter %d", i);
        break; //if we get any errors, stop & return failure. Params will be loaded from default
      }
    } while (rc == SQLITE_ROW);
    rc = (rc != SQLITE_DONE) ? rc : SQLITE_OK;

    sqlite3_clear_bindings(otherParamReadStm); //Always returns SQLITE_OK
    sqlite3_reset(otherParamReadStm);
    sqlite3_finalize(otherParamReadStm);
    otherParamReadStm = NULL;
  }
  if (nRowsRead != 1) //make sure we have the correct number of rows
    rc = SQLITE_ERROR;

  return;*/
}


void CBreasDBGeneratorDlg::OnBnClickedParseLevel2()
{
  CStringA outfile;
  CString infile;
  // szFilters is a text string that includes two file name filters:
  TCHAR szFilters[] = _T("V45 Files (*.V45)|*.V45|All Files (*.*)|*.*||");

  // Create an OPEN dialog; the default file name extension is ".db".
  CFileDialog fileDlg(TRUE, _T("V45"), _T("*.V45"),
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);
  // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
  // returns IDOK.
  if (fileDlg.DoModal() == IDOK)
  {
    infile = fileDlg.GetPathName();
    outfile = infile;// "C:\\Users\\rabur\\Desktop\\dummyfile.txt";
    outfile.Replace(".V45", ".txt");
    CFile fp;
    FILE* fpo = NULL;
    CFileException e;
    //UBYTE data[256];
    TBreathData breath;
    fopen_s(&fpo, outfile, "w+");
    if (fp.Open(infile, CFile::modeRead) && fpo != NULL)
    {

      char buf[512];
      int nRead = fp.Read(&buf, 512);
      fprintf(fpo, "MODEL:%sSerial Number:%s\n", &buf[0], &buf[24]);
      fprintf(fpo, "Profile1Name%s\n", &buf[48]);
      fprintf(fpo, "Profile2Name%s\n", &buf[60]);
      fprintf(fpo, "Profile3Name%s\n", &buf[72]);
      fprintf(fpo, "DatabaseVersion:%s", &buf[84]);
      fprintf(fpo, "LogLevel:%s\n", &buf[85]);

      int i = 0;
      ULONG breathId = 0;
      ULONG timeStamp = 0;
      ULONGLONG length = fp.GetLength();
      int nSize = sizeof(TBreathData);
      ULONGLONG nTotalBreaths = (length - 512) / nSize;
      fprintf(fpo, "Approximate length: %lld breaths", nTotalBreaths);
     // fprintf(fpo, "\ntime,breathId,patientPressure,patientFlow,totalFlow,tidalVolume,momentaryCO2,atmP,fio2Perc,trigger,sigh,leakage,thoracic,abdominal,breathFlag\n");
      UBYTE* data = new UBYTE[length];
      fp.Read(data, length);
      while (i < nTotalBreaths)
      {
        memcpy(&breath, &data[i*(nSize+1)], sizeof(breath));
        breathId = ULONGfromBuf(&data[0+47*i]);
        timeStamp = ULONGfromBuf(&data[7+47*i]);
        breath.maxTidalVolume = UWORDfromBuf(&data[20+ 47 * i]);
        fprintf(fpo, "%d,%d,%d,%d\n", timeStamp, breath.maxTidalVolume);
        i++;
        
      }
      delete data;
      fp.Close();

      fclose(fpo);

    }
  }
  CString outText;
  m_outputCtrl.GetWindowTextW(outText);
  outText += _T("done");
  outText += "\r\n";
  m_outputCtrl.SetWindowText(outText);
}
