/*
    This file is part of utilfuncs comprising utilfuncs.h and utilfuncs.cpp

    utilfuncs - Copyright Marius Albertyn

    utilfuncs is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    utilfuncs is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with utilfuncs in a file named COPYING.
    If not, see <http://www.gnu.org/licenses/>.
*/

//#include <utilfuncs/utilfuncs.h>
#include "utilfuncs.h"
#include <map>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <iomanip>
#include <system_error>
#include <ctime>
#include <sys/stat.h>
#include <thread>
#include <mutex>

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
//#if d e f ined(PLATFORM_POSIX) || defined(__linux__)
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <sys/statvfs.h>
#include <sys/sendfile.h>
#endif

//--------------------------------------------------------------------------------------------------
//bool MSTR::HasValue(const std::string &sv) const { for (auto p:(*this)) if (sieqs(p.second, sv)) { return true; } return false; }

//--------------------------------------------------------------------------------------------------
std::string s_error_report="";
std::string get_error_report() { return s_error_report; }
void clear_error_report() { s_error_report.clear(); }
bool report_error(const std::string &serr, bool btell) { s_error_report=serr; if (btell) return sayerr(serr); return false; }
bool log_report_error(const std::string &serr, bool btell)
{
	std::string slog=path_append(homedir(), says(path_name(thisapp()), "_LOGGER.LOG"));
	std::ofstream(slog.c_str(), std::ios::app) << serr << "\n";
	return report_error(serr, btell);
}
void write_to_log(const std::string &slogname, const std::string &sinfo)
{
	std::string slog=path_append(homedir(), "LOGFILES");
	if (!path_realize(slog)) { sayerr("cannot create/access log-file-dir: ", slog); return; }
	slog=path_append(slog, slogname);
	std::ofstream(slog.c_str(), std::ios::app) << sinfo << "\n";
}

//--------------------------------------------------------------------------------------------------
std::string s_utilfuncs_error="";
std::string get_utilfuncs_error() { return s_utilfuncs_error; }
void clear_utilfuncs_error() { s_utilfuncs_error.clear(); }
bool set_utilfuncs_error(const std::string &serr, bool btell)
{
	s_utilfuncs_error=serr;
	if (btell) sayerr(s_utilfuncs_error);
	return false;
}


//--------------------------------------------------------------------------------------------------

///todo 
/*
	change this to use X directly (or Wayland?)
	does X have "msgbox()"?
	
	if X/Wayland is running ...
	
*/
#ifdef flagGUI
#include <CtrlLib/CtrlLib.h>
#include <CtrlCore/CtrlCore.h>
using namespace Upp;
	void WRITESTRING(const std::string &s)
	{
		std::string S{s};
		ReplaceChars(S, "\t", "    ");
		PromptOK(DeQtf(S.c_str()));
	}
	std::string READSTRING(const std::string &sprompt)
	{
		std::string s;
		struct Dlg : public TopWindow
		{
			EditString e;
			Button k;
			Button c;
			std::string sN;
	
			virtual ~Dlg(){}
			Dlg(const std::string &st, const std::string &sd)
			{
				SetRect(Size(350, 70));
				Title(st.c_str());
				Add(e.HSizePosZ().TopPos(0,20));
				e.SetData(sd.c_str());
				Add(k.SetLabel(t_("OK")).RightPosZ(140, 60).TopPosZ(30, 20));
				Add(c.SetLabel(t_("Cancel")).RightPosZ(70, 60).TopPosZ(30, 20));
				k.WhenPush << [&]{ sN=e.GetData().ToString().ToStd(); TRIM(sN); Close(); };
				c.WhenPush << [&]{ sN=""; Close(); };
			}
			bool Key(dword key, int)
			{
				if (key==K_ENTER) { k.WhenPush(); return true; }
				else if (key==K_ESCAPE) { c.WhenPush(); return true; }
				return false;
			}
		};
		Dlg dlg(sprompt, ""); //for now ...
		dlg.Execute();
		s=dlg.sN;
		return s;
	}

	const bool READOK(const std::string &sprompt) { PromptOK(DeQtf(sprompt.c_str())); return true; }

	void waitenter(const std::string &msg) { PromptOK(DeQtf(msg.c_str())); }

	void MsgOK(std::string msg, std::string title)
	{
		Prompt(title.c_str(), Image(), msg.c_str(), "OK");
	}

	void RTFtostring(const std::string &srtf, std::string &stext)
	{
		stext=ParseRTF(srtf.c_str()).GetPlainText().ToString().ToStd();
	}

#else
	void WRITESTRING(const std::string &s) { std::cout << s << std::flush; }
	std::string READSTRING(const std::string &sprompt)
	{
		std::string s{};
		std::cout << sprompt << ": ";
		std::cin >> s;
		return s;
	}

	const bool READOK(const std::string &sprompt)
	{
		std::string s{};
		std::cout << sprompt << " [y/n]: ";
		std::cin >> s;
		return (sieqs(s, "y"));
	}

	void waitenter(const std::string &msg) { std::cout << msg << "\nPress Enter to continue..\n"; std::cin.get(); }

	void RTFtostring(const std::string &srtf, std::string &stext)
	{
		stext=srtf; //todo...can't remember why
	}

#endif

std::string askuser(const std::string &sprompt)
{
	return READSTRING(sprompt);
}

bool askok(const std::string &smsg)
{
	return READOK(smsg);
}

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)

	bool validate_app_path(std::string sFP, std::string &sRet)
	{ //ignores cmdline parameters..
		if (file_exist(sFP)) { sRet=sFP; return true; }
		sRet.clear();
		char SPACE=32, PATH_SEP='/';
		std::string sp{sFP}, st;
		size_t p, l, pls;
		p=std::string::npos;
		pls=sp.find_last_of(PATH_SEP, p);
		if (pls==std::string::npos) return false;
		do
		{
			l=sp.find_last_of(SPACE, p);
			st=sp.substr(0,l);
			if (file_exist(st)) { sRet=st; return true; }
			p=(l-1);
		} while ((l!=std::string::npos)&&(l>pls));
		return false;
	}
	
	std::string thisapp()
	{
		std::string sp{}, st;
		std::ifstream(says("/proc/", getpid(), "/cmdline").c_str()) >> st;
		st=st.c_str(); //remove terminating '\0's; also problem if name contain spaces/parms - validate..
		if (!validate_app_path(st, sp)) sp.clear(); //="Error: application not found"; //
		return sp;
	}
	std::string username() { return getpwuid(getuid())->pw_name; }
	std::string username(int uid)
	{
		struct passwd *p=getpwuid(uid);
		std::string s{"?"};
		if (p) s=p->pw_name;
		return s;
	}
	std::string homedir() { return getpwuid(getuid())->pw_dir; }
	std::string hostname() { char buf[1024]; gethostname(buf, 1024); return std::string((char*)buf); }
	
#elif defined(_WIN64)
	std::string thisapp()
	{
		TCHAR buf[MAX_PATH];
		GetModuleFileName(NULL, buf, MAX_PATH);
		std::string sp=buf;
		return sp;
	}
	const std::string username() { return GetUserName().ToStd(); }
	const std::string username(int uid) { return "(N/A)"; }
	const std::string homedir() { return GetHomeDirectory().ToStd(); }
	const std::string hostname() { return "(N/A)"; }
#endif

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	int realuid() { return (int)getuid(); }
	int effectiveuid() { return (int)geteuid(); }
	bool has_root_access() { return seqs(username(effectiveuid()), "root"); }
#endif

//--------------------------------------------------------------------------------------------------
void default_output_path(std::string &outpath)
{
	//order: 1 app-path/, 2 (linux)~/.config/<appname>/, 3 (linux)~/<appname>/ | (win) <homedir>/<app_name_ext>,  4 homedir
	std::string sapp=thisapp();
	std::string sp{};
	sp=path_path(sapp);
	if (canwrite(sp)) { outpath=sp; return; }
	std::string sn{};
	sn=path_name(sapp);
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
		sp=path_append(homedir(), ".config");
		sp=path_append(sp, sn);
#elif defined(_WIN64)
	//sn=file_name_noext(path_name(sapp))
	sn=SanitizeName(path_name(sapp));
	sp=path_append(homedir(), sn);
#endif
	if (path_realize(sp)) outpath=sp;
	else outpath=homedir();
}

//--------------------------------------------------------------------------------------------------
// bash/linux...
//see: https://unix.stackexchange.com/questions/347332/what-characters-need-to-be-escaped-in-files-without-quotes
std::string const ec{" \t!\"'#$&()*,;<>=?[]\\^`{}|~"}; //chars that need to be escaped for bash
bool is_bash_safe_name(const std::string &sn)
{
	if (!sn.size()||(sn[0]=='-')) return false;
	//std::string ec=" \t!\"'#$&()*,;<>=?[]\\^`{}|~"; //chars that need to be escaped for bash
	for (auto c:sn) { if (ec.find(c)!=std::string::npos) return false; }
	return true;
}

std::string bash_escape_name(const std::string &name)
{
	std::string ename=""; //, ec=" \t!\"'#$&()*,;<>=?[]\\^`{}|~";
	char bs=char(92);
	for (auto c:name) { if (ec.find(c)!=std::string::npos) ename+=bs; ename+=c; }
	if (ename[0]=='-') ename.insert(0, "./"); //special handling if name starts with a minus
	return ename;
}

//--------------------------------------------------------------------------------------------------
pid_t find_pid(const std::string &spath)
{
	pid_t pid{0};
	DirEntries de{};
	if (dir_read("/proc", de)) //, true);
	{
		bool b{false};
		std::string sp;
		std::string t;
		for (auto p:de)
		{
			sp=says("/proc/", p.first, "/cmdline");
			if (file_read(sp, t))
			{
				if ((b=slike(spath, t))) { pid=stot<pid_t>(p.first); break; }
			}
		}
	}
	return pid;
}

bool kill_pid(pid_t pid)
{
	if (pid<=0) return false; //no suicide
	int r=kill(pid, SIGKILL);
	if (r!=0) set_utilfuncs_error(says("Failed to kill process (pid=", pid, ") ", std::strerror(errno)));
	return !r;
}

void kill_self() { kill(getpid(), SIGKILL); }

bool kill_app(const std::string &spath)
{
	pid_t pid=find_pid(spath);
	if (pid!=0) return kill_pid(pid);
	else return set_utilfuncs_error(says("Failed to kill app: ", spath, "  ", std::strerror(errno)));
}


#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
#include <termios.h>
#include <fcntl.h>
	bool checkkeypress(int k)
	{
		//this shit isn't working no more ...
		//or does not work in gui? only console?
		
		struct termios keep, temp;
		int curflags;
		int K=0;
		tcgetattr(STDIN_FILENO, &keep);
		temp=keep;
		curflags=fcntl(STDIN_FILENO, F_GETFL, 0);
		temp.c_lflag&=~(ICANON|ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &temp);
//		curflags=fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, curflags|O_NONBLOCK);
		K=getchar();
//		if (K!=(-1)) //for debugging
//		{
//			int x;
//			x=K;
//		}
	fcntl(STDIN_FILENO, F_SETFL, curflags);
		tcsetattr(STDIN_FILENO, TCSANOW, &keep);
		///fcntl(STDIN_FILENO, F_SETFL, curflags);
		return (K==k);
	}
	
	bool askpass(std::string &pw)
	{
		pw.clear();
	//pw="this is a test"; return true;
		
		std::cout << "Password: ";
		struct termios temp;
		tcgetattr(STDIN_FILENO, &temp);
		temp.c_lflag&=~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &temp);
		std::cin >> pw;
		temp.c_lflag|=ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &temp);
		return (!pw.empty());
	}
	
#elif defined(_WIN64)
	bool checkkeypress(int k)																	//todo...
	{																							//untested
		HANDLE h=GetStdHandle(STD_INPUT_HANDLE);
		DWORD evt;
		INPUT_RECORD ir;
		int K=0;

		PeekConsoleInput(h, &ir, 1, &evt);
		if (evt>0)
		{
			ReadConsoleInput(h, &ir, 1, &evt);
			K=ir.Event.KeyEvent.wVirtualKeyCode;
		}
		return (K==k);
	}
	bool askpass(std::string pw)
	{
		pw.clear();
		std::cout << "Password: ";
		HANDLE h=GetStdHandle(STD_INPUT_HANDLE);
		DWORD cm;
		GetConsoleMode(h, &cm);
		cm&=~ENABLE_ECHO_INPUT;
		SetConsoleMode(h, cm);
		std::cin >> pw;
		cm|=ENABLE_ECHO_INPUT;
		SetConsoleMode(h, cm);
		return (!pw.empty());
	}
	
#endif

//--------------------------------------------------------------------------------------------------
std::string to_sKMGT(size_t n)
{
	std::string s("");
	int i=0,t;
	double f=n;
	while (f>1024.0) { i++; f/=1024.0; }
	t=(int)(f*100.0); f=((double)t/100.0);
	return says(f, i["bKMGTPEZY"]);
}

//--------------------------------------------------------------------------------------------------
#define MAX_SHORT 65535L

DTStamp make_dtstamp(struct tm *ptm, uint64_t usecs)
{
	DTStamp dts=0;
	uint16_t ms;
	dts=ptm->tm_year;
	dts<<=8; dts+=ptm->tm_mon+1;
	dts<<=8; dts+=ptm->tm_mday;
	dts<<=8; dts+=ptm->tm_hour;
	dts<<=8; dts+=ptm->tm_min;
	dts<<=8; dts+=ptm->tm_sec;
	ms=(uint16_t)(usecs%MAX_SHORT); //difference in 4-digit microsecond-tail
	dts<<=16; dts+=ms;
	return dts;
	
}

DTStamp dt_stamp()
{
//	static uint16_t prev=0;
	time_t t;
	struct tm *ptmcur;
	DTStamp dts=0;
	time(&t);
	ptmcur = localtime(&t);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	dts=make_dtstamp(ptmcur, tv.tv_usec);
	return dts;
}

std::string ymdhms_stamp()
{
	time_t t;
	struct tm tmcur;
	std::string dts{};
	int y, m;
	auto zp=[=](int n)->std::string{ std::string r=((n<10)?"0":""); r+=ttos(n); return r; };
	time(&t);
	tmcur = *localtime(&t);
	y=(tmcur.tm_year+1900);
	m=tmcur.tm_mon+1;
	dts=says(zp(y), zp(m), zp(tmcur.tm_mday), zp(tmcur.tm_hour), zp(tmcur.tm_min), zp(tmcur.tm_sec));
	return dts;
}

std::string h_dt_stamp()
{
	//output: 0y0m0d0H0M0Sxx (e.g.: "181123111051A3" => 2018 Nov 23 11:10:51 (A3 is reduced-microsec-suffix)
	
	static uint8_t prev=0;
	time_t t;
	struct tm tmcur;
	struct timeval tv;
	std::string hdts{};
	uint8_t y,m,ms;
	auto zp=[=](int n)->const std::string{ std::string r=((n<10)?"0":""); r+=ttos(n); return r; }; //param must be int! (not uint8_t)
	time(&t);
	tmcur = *localtime(&t);
	y=((tmcur.tm_year+1900)%100); //just years (0..99)
	m=tmcur.tm_mon+1;
	gettimeofday(&tv, NULL);
	ms=(uint8_t)((double(tv.tv_usec)/1000000.0)*255.0); //reduce to range 0..255 *** NB! (if call-freq > 255, will get dupes!)
	if (ms==prev) { prev=(prev<255)?(prev+1):0; ms=prev; } prev=ms; // *** try to prevent dupes
	hdts=says(zp(y), zp(m), zp(tmcur.tm_mday), zp(tmcur.tm_hour), zp(tmcur.tm_min), zp(tmcur.tm_sec), tohex(ms));
	return hdts;
}

DTStamp ToDTStamp(const std::string &yyyymmddHHMMSS)
{
	int n, l=yyyymmddHHMMSS.size();
	DTStamp dts=0;
	if (l>=8) //"yyyymmdd" at least
	{
		n=(stot<int>(yyyymmddHHMMSS.substr(0,4))-1900); //"[XXXX]mmddHHMMSS"
		if ((n>0)&&(n<255))
		{
			dts=(uint8_t)n;
			n=stot<int>(yyyymmddHHMMSS.substr(4,2)); //"yyyy[XX]ddHHMMSS"
			if ((n>0)&&(n<=12))
			{
				dts<<=8; dts+=(uint8_t)n;
				n=stot<int>(yyyymmddHHMMSS.substr(6,2)); //"yyyymm[XX]HHMMSS"
				if ((n>0)&&(n<=31))
				{
					dts<<=8; dts+=(uint8_t)n;
					if (l>=10)
					{
						n=stot<int>(yyyymmddHHMMSS.substr(8,2)); //"yyyymmdd[XX]MMSS"
						if ((n>=00)&&(n<24))
						{
							dts<<=8; dts+=(uint8_t)n;
							if (l>=12)
							{
								n=stot<int>(yyyymmddHHMMSS.substr(10,2)); //"yyyymmddHH[XX]SS"
								if ((n>=00)&&(n<60))
								{
									dts<<=8; dts+=(uint8_t)n;
									if (l>=14)
									{
										n=stot<int>(yyyymmddHHMMSS.substr(12,2)); //"yyyymmddHHMM[XX]"
										if ((n>=00)&&(n<60)) { dts<<=8; dts+=(uint8_t)n; }
										else dts<<=8;
									}
									else dts<<=8;
								}
								else dts<<=16;
							}
							else dts<<=16;
						}
						else dts<<=24;
					}
					else dts<<=24;
				}
				else dts=0; //error invalid day
			}
			else dts=0; //error invalid month
		}
		else dts=0; //error invalid year
	}
	if (dts!=0) dts<<=16; //ms
	return dts;
}

std::string month_name(int m, bool bfull)
{
	if ((m<1)||(m>12)) return "?m?";
	static const VSTR vf("January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December");
	static const VSTR va={ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	if (bfull) return vf[m-1];
	return va[m-1];
}

std::string ToDateStr(DTStamp dts, int ds, bool btime, bool bmsec)
{
	std::string sdt{}, st{}, su{};
	uint32_t y,m,d,H,M,S,U;
	auto zp=[=](int n)->std::string{ std::string r=((n<10)?"0":""); r+=ttos(n); return r; };
	
	y=(((dts>>56)&0xff)+1900);
	m=((dts>>48)&0xff);
	d=((dts>>40)&0xff);
	H=((dts>>32)&0xff);
	M=((dts>>24)&0xff);
	S=((dts>>16)&0xff); st=says(zp(H), ":", zp(M), ":", zp(S));
	U=(dts&0xffff); su=says(".", U);

	if (ds==DS_SQUASH)
	{
		y%=100; sdt=says(zp(y), zp(m), zp(d), zp(H), zp(M), zp(S), (bmsec)?su:"");
		if (btime) sayss(sdt, st, (bmsec)?su:"");
	}
	else
	{
		sdt=says(y, " ", (ds==DS_FULL)?month_name(m):(ds==DS_SHORT)?month_name(m, false):ttos(m), " ", (int)d);
		if (btime) sayss(sdt, " ", st, (bmsec)?su:"");
	}
	return sdt;
}

uint64_t get_unique_index() { return dt_stamp(); }

std::string get_unique_name(const std::string sprefix, const std::string ssuffix) { return says(sprefix, dt_stamp(), ssuffix); }

//--------------------------------------------------------------------------------------------------
void LTRIM(std::string& s, const char *sch)
{
	std::string ss=sch;
	int i=0, n=s.length();
	while ((i<n) && (ss.find(s.at(i),0)!=std::string::npos)) i++;
	s=(i>0)?s.substr(i,n-i):s;
}

void RTRIM(std::string& s, const char *sch)
{
	std::string ss=sch;
	int n = s.length()-1;
	int i=n;
	while ((i>=0) && (ss.find( s.at(i),0)!=std::string::npos)) i--;
	s=(i<n)?s.substr(0,i+1):s;
}

void TRIM(std::string& s, const char *sch) { LTRIM(s, sch); RTRIM(s, sch); }

void ReplacePhrase(std::string& S, const std::string& sPhrase, const std::string& sNew)
{ //replaces EACH occurrence of sPhrase with sNew: ..("aXXbXXc, "XX", "YYY") -> "aYYYbYYYc"
	size_t pos = 0, fpos;
	std::string s=S;
	while ((fpos=s.find(sPhrase, pos)) != std::string::npos)
	{
		s.replace(fpos, sPhrase.size(), sNew);
		pos=fpos + sNew.length();
	}
	S=s;
}

void ReplaceChars(std::string &s, const std::string &sCharList, const std::string &sReplacement)
{ //replaces EACH occurrence of EACH single char in sCharList with sReplacement: ..("abcde", "ace", "XX") -> "XXbXXdXX"
    size_t pos=0;
    while ((pos=s.find_first_of(sCharList, pos))!= std::string::npos)
    {
        s.replace(pos, 1, sReplacement);
        pos += sReplacement.length();
    }
}

std::string SanitizeName(const std::string &sp)
{
	std::string s=sp;
	ReplaceChars(s, " \\+-=~`!@#$%^&*()[]{}:;\"|'<>,.?/", "_");
	TRIM(s, "_");
	std::string t;
	do { t=s; ReplacePhrase(s, "__", "_"); } while (t.compare(s)!=0);
	return s;
}

std::string EscapeChars(const std::string &raw, const std::string &sChars)
{
    std::string sret{};
    for (size_t pos=0; pos<raw.size(); pos++)
    {
        if ((raw[pos]=='\\')||(sChars.find(raw[pos])!=std::string::npos)) sret+='\\';
        sret+=raw[pos];
    }
    return sret;
}

std::string UnescapeChars(const std::string &sesc)
{
    std::string sret{};
    bool bp{false};
    for (size_t pos=0; pos<sesc.size(); pos++)
    {
		if (sesc[pos]=='\\')
		{
			if (!bp) bp=true;
			else { sret+=sesc[pos]; bp=false; }
			
		}
		else { bp=false; sret+=sesc[pos]; }
	}
	return sret;
}

std::string ucase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::toupper(static_cast<unsigned char>(c));
	return w;
}

//const std::string ucase(const std::string &s) { return ucase(s.c_str()); }
//void toucase(std::string& s) { s=ucase(s.c_str()); }

std::string lcase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::tolower(static_cast<unsigned char>(c));
	return w;
}

//const std::string lcase(const std::string &s) { return lcase(s.c_str()); }
//void tolcase(std::string &s) { s=lcase(s.c_str()); }


/*
int scmp(const std::string &s1, const std::string &s2) { return s1.compare(s2); }
int sicmp(const std::string &s1, const std::string &s2)
{
	std::string t1=s1, t2=s2;
	return ucase(t1.c_str()).compare(ucase(t2.c_str()));
}
bool seqs(const std::string &s1, const std::string &s2) { return (scmp(s1,s2)==0); }
bool sieqs(const std::string &s1, const std::string &s2) { return (sicmp(s1,s2)==0); }
*/

bool scontain(const std::string &data, const std::string &fragment) { return (data.find(fragment)!=std::string::npos); }
bool sicontain(const std::string &data, const std::string &fragment) { return scontain(lcase(data), lcase(fragment)); }

bool scontainany(const std::string &s, const std::string &charlist)
{
    for (auto c:s) if (charlist.find(c)!=std::string::npos) return true;
	return false;
}

bool hextoch(const std::string &shex, char &ch) //shex <- { "00", "01", ... , "fe", "ff" }
{
	if (shex.length()==2)
	{
		std::string sh=shex;
        tolcase(sh);
		int n=0;
		if		((sh[0]>='0')&&(sh[0]<='9')) n=((sh[0]-'0')*16);
		else if ((sh[0]>='a')&&(sh[0]<='f')) n=((sh[0]-'a'+10)*16);
		else return false;
		if		((sh[1]>='0')&&(sh[1]<='9')) n+=(sh[1]-'0');
		else if ((sh[1]>='a')&&(sh[1]<='f')) n+=(sh[1]-'a'+10);
		else return false;
		ch=char(n);
		return true;
	}
	return false;
}

int shextodec(const std::string &shex) //shex contain hexdigits (eg "1E23A" or "1e23a" or "0x1E23A" or "0x1e23a" or "0X1E23A" or "0X1e23a")
{
	int n=0;
	auto ishex=[](const std::string &s)->bool{ for (auto c:s) if (!(((c>='0')&&(c<='9'))||((c>='a')&&(c<='f')))) return false; return true; };
	auto c2d=[](char c)->int{ int d=0; if ((c>='0')&&(c<='9')) d=(c-'0'); else if ((c>='a')&&(c<='f')) d=(c-'a'+10); return d; };
	std::string sh=shex; tolcase(sh); LTRIM(sh,"0x");
	if (ishex(sh)) { for (auto c:sh) { n<<=4; n+=c2d(c); }}
	return n;
}

void fshex(const std::string &sraw, std::string &shex, int rawlen)
{
	//output: hex-values + tab + {char|.}'s, e.g.: "Hello" ->  "48 65 6C 6C 6F | Hello"			/////"48 65 6C 6C 6F    Hello"
	//										 e.g.: "Hel\tlo" ->  "48 65 6C 09 6C 6F | Hel.lo"	/////"48 65 6C 09 6C 6F    Hel.lo"
	int i, l=((rawlen<=0)||(rawlen>(int)sraw.length()))?sraw.length():rawlen;
	auto c2s=[](unsigned char u)->std::string
		{
			char b[3];
			char *p=b;
			if(u<0x80) *p++=(char)u;
			//else if(u<=0xff) { *p++=(0xc0|(u>>6)); *p++=(0x80|(u&0x3f)); }
			else { *p++=(0xc0|(u>>6)); *p++=(0x80|(u&0x3f)); }
			return std::string(b, p-b);
		};
	shex.clear();
	for (i=0; i<l; i++) { shex+=tohex<unsigned char>(sraw[i], 2); shex+=" "; }
	//shex+="\t"; //problem with variable tab-sizes
	//shex+="    ";//simulate 4-space 'tab'
	shex+=" | ";//space-bar-space ---> "Hello" ->  "48 65 6C 6C 6F | Hello"
	unsigned char u;
	for (i=0; i<l; i++) { u=sraw[i]; if (((u>=32)&&(u<=126))||((u>159)&&(u<255))) shex+=c2s(u); else shex+='.'; }
}

std::string ashex(const std::string &sraw)
{
	std::string s("");
	unsigned int i;
	for (i=0; i<sraw.length(); i++) { s+=tohex<unsigned char>(sraw[i], 2); if (i<(sraw.length()-1)) s+=" "; }
	return s;
}

std::string ashex(unsigned char u)
{
	return tohex<unsigned char>(u);
};
		
std::string asbin(unsigned char u)
{
	std::string s("");
	int i=7;
	while (i>=0) { s+=((u>>i)&1)?"1":"0"; i--; }
	return s;
}

std::string en_quote(const std::string &s, char Q)
{
//	std::stringstream ss;
//	ss << std::quoted(s);
//	return ss.str();
	std::string sr{};
	sr+=Q;
	for (size_t i=0; i<s.size(); i++) { if ((s[i]=='\\')||(s[i]==Q)) sr+="\\"; sr+=s[i]; }
	sr+=Q;
	return sr;

}

std::string de_quote(const std::string &s)
{
//	std::stringstream ss(s);
//	std::string r;
//	ss >> std::quoted(r);
//	return r;
	std::string sr{};
	if (!s.empty())
	{
		if (s[0]==s[s.size()-1]) //the 'quote' can be any char
		{
			bool b{false};
			for (auto c:s.substr(1, s.size()-2))
			{
				if (c=='\\') { if (b) { sr+=c; b=false; } else b=true; }
				else sr+=c;
			}
		}
		else sr=s;
	}
	return sr;

}
/* ...check if above en & de works like below... todo-done they do
	auto enqt = [](std::string s)->std::string
		{
			std::string sr{};
			sr+="\"";
			for (auto c:s) { if ((c=='\\')||(c=='"')) sr+="\\"; sr+=c; }
			sr+="\"";
			return sr;
		};
	auto deqt = [](std::string s)->std::string
		{
			std::string sr{};//, st;
			if ((s[0]=='"')&&(s[s.size()-1]=='"'))
			{
				bool b{false};
				//st=s.substr(1, s.size()-2);
				for (auto c:s.substr(1, s.size()-2))
				{
					if (c=='\\') { if (b) { sr+=c; b=false; } else b=true; }
					else sr+=c;
				}
			}
			else sr=s;
			return sr;
		};
*/


std::string schop(const std::string &s, int nmaxlen, bool bdots)
{
	std::string w;
	if (bdots) { nmaxlen-=2; if (nmaxlen<=2) { w=".."; return w; }}
	if (nmaxlen>=(int)s.length()) { w=s; return w; }
	w=s.substr(0,nmaxlen);
	if (bdots) w+="..";
	return w;
}

std::string spad(const std::string &s, int nmaxlen, char c, bool bfront, int tabsize)
{
	std::string w=s;
	int nt=0;
	for (size_t i=0; i<w.size(); i++) { if (w[i]=='\t') nt++; }
	int n=nmaxlen-w.length()-(nt*tabsize)+nt;
	if (n>0) w.insert((bfront?0:w.length()), n, c);
	return w;
}

std::string spadct(const std::string &s, int nmaxlen, char c, bool bfront, int tabcols, int startpos)
{
	auto NTD=[](int p, int c)->int{ return (((p/c)*c)+c-p-1); }; //Next-Tab-Distance (-1 because 0-based)
	std::string w{};
	int d=0;
	for (int i=0; i<(int)s.size(); i++)
	{
		if (s[i]=='\t') { d=NTD(w.size()+startpos, tabcols); while (d-->0) w+=' '; }
		else w+=s[i];
	}
	int n=nmaxlen-w.length();
	if (n>0) w.insert((bfront?0:w.length()), n, c);
	return w;
}

std::string sfit(const std::string &s, int nmaxlen, int fitbit, char cpad)
{
	if (nmaxlen<=0) return "";
	if (nmaxlen<=3) return s.substr(0, nmaxlen);
	bool bdots=((fitbit&FIT_DOT)==FIT_DOT);
	bool bpad=((fitbit&FIT_PAD)==FIT_PAD);
	int ns=s.size();
	switch (fitbit&~(FIT_DOT|FIT_PAD))
	{
		case FIT_NONE: //fall-thru
		case FIT_R:
					{
						if (ns>nmaxlen) return schop(s, nmaxlen, bdots);
						else if (bpad) return spad(s, nmaxlen, cpad, false);
					} break;
		case FIT_L:
					{
						if (ns>nmaxlen)
						{
							std::string sf{};
							if (bdots) sf="..";
							sf+=s.substr(s.size()-nmaxlen+((bdots)?2:0));
							return sf;
						}
						else if (bpad) return spad(s, nmaxlen, cpad);
					} break;
		case FIT_M:
					{
						std::string sf{};
						if (ns>nmaxlen)
						{
							int l=(nmaxlen/2);
							int r=(nmaxlen-l);
							//if (bdots) { //always dotted...
							l--; r=(ns-r+1); sf=s.substr(0,l); sf+=".."; sf+=s.substr(r);
							//}
							return sf;
						}
						else if (bpad&&(ns<nmaxlen))
						{
							int l=(ns/2);
							int r=(ns-l);
							sf=s.substr(0,l); spad(sf, nmaxlen-ns, cpad); sf+=s.substr(s.size()-r);
							return sf;
						}
					} break;
	}
	return s;
}

std::string slcut(std::string &S, size_t n)
{
	std::string sc{};
	if (!S.empty())
	{
		sc=S.substr(0,n);
		if (S.size()>n) S=S.substr(n); else S.clear();
	}
	return sc;
}

std::string srcut(std::string &S, size_t n)
{
	std::string sc{};
	if (S.size()>n)
	{
		size_t p=(S.size()-n);
		sc=S.substr(p);
		S=S.substr(0, p);
	}
	else { sc=S; S.clear(); }
	return sc;
}

bool slike(const std::string &sfront, const std::string &sall) //case-sensitive compare sfront with start of sall
{
	if (sfront.size()>sall.size()) return false;
	std::string s=sall.substr(0, sfront.size());
	return seqs(sfront, s);
}

bool silike(const std::string &sfront, const std::string &sall) //non-case-sensitive compare sfront with start of sall
{
	if (sfront.size()>sall.size()) return false;
	std::string s=sall.substr(0, sfront.size());
	return sieqs(sfront, s);
}


//--------------------------------------------------------------------------------------------------
unicodepoint u8toU(std::string u8)
{
    uint32_t U{};
	int l=u8.size();
	if (l==1) U=(uint32_t)(u8[0]);
	uint8_t B=(uint8_t)(u8[0]);
	if (((B&0xe0)==0xc0)&&(l>=2)) { U=(B&0x1f); U<<=6; U+=(u8[1]&0x3f); }
	else if (((B&0xf0)==0xe0)&&(l>=3)) { U=(B&0x0f); U<<=6; U+=(u8[1]&0x3f); U<<=6; U+=(u8[2]&0x3f); }
	else if (((u8[0]&0xf8)==0xf0)&&(l>=4)) { U=(B&0x07); U<<=6; U+=(u8[1]&0x3f); U<<=6; U+=(u8[2]&0x3f); U<<=6; U+=(u8[3]&0x3f); }
	return (unicodepoint)U;
}

std::string Utou8(unicodepoint U)
{
	using UC=uint8_t;
	std::string s("");
	if (U<0x80) s=UC(U);
	else if ((U==0xfffe) //non-characters
		||(U==0xffff)
		||((U>=0xfdd0)&&(U<=0xfdef))
		||(U==0x1fffe)||(U==0x2fffe)||(U==0x3fffe)||(U==0x4fffe)||(U==0x5fffe)||(U==0x6fffe)||(U==0x7fffe)||(U==0x8fffe)
		||(U==0x9fffe)||(U==0xafffe)||(U==0xbfffe)||(U==0xcfffe)||(U==0xdfffe)||(U==0xefffe)||(U==0xffffe)||(U==0x10fffe)
		||(U==0x1ffff)||(U==0x2ffff)||(U==0x3ffff)||(U==0x4ffff)||(U==0x5ffff)||(U==0x6ffff)||(U==0x7ffff)||(U==0x8fffe)
		||(U==0x9ffff)||(U==0xaffff)||(U==0xbffff)||(U==0xcffff)||(U==0xdffff)||(U==0xeffff)||(U==0xfffff)
		||(U>=0x10ffff))
	{
		s=INVALID_UTF8;
	}
	else
	{
		int n=(U<=0x7FF)?1:(U<=0xFFFF)?2:3;
		int d=n;
		while (d-->0) { s.insert(s.begin(), UC(0x80|(U&0x3f))); U>>=6; }
		s.insert(s.begin(), UC((0x1e<<(6-n))|(U&(0x3F>>n))));
	}
	return s;
}

///reworked copy from: https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
bool isvalidu8(const std::string &s)
{
	const unsigned char *p=(const unsigned char*)s.c_str(); //ensure null-terminated (.data may not be?)
	while (*p)
	{
		if (*p<0x80) p++; //ascii
		else if ((p[0]&0xe0)==0xc0) //110XXXXx 10xxxxxx
		{
			if ((p[1]&0xc0)!=0x80||(p[0]&0xfe)==0xc0) return false; //too long
			else p+=2;
		}
		else if ((p[0]&0xf0)==0xe0) //1110XXXX 10Xxxxxx 10xxxxxx
		{
			if (((p[1]&0xc0)!=0x80)
					|| ((p[2]&0xc0)!=0x80)
					|| ((p[0]==0xe0)&&((p[1]&0xe0)==0x80)) //) //too long
					|| ((p[0]==0xed)&&((p[1]&0xe0)==0xa0)) //surrogate ???????
					|| ((p[0]==0xef)&&(p[1]==0xbf)&&((p[2]&0xfe)==0xbe))) //U+FFFE or U+FFFF
				return false;
			else p+=3;
		}
		else if ((p[0]&0xf8)==0xf0) //11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
		{
			if (((p[1]&0xc0)!=0x80)
					|| ((p[2]&0xc0)!=0x80)
					|| ((p[3]&0xc0)!=0x80)
					|| ((p[0]==0xf0)&&((p[1]&0xf0)==0x80)) //too long
					|| ((p[0]==0xf4)&&(p[1]>0x8f))||(p[0]>0xf4) )   // > U+10FFFF?
				return false;
			else p+=4;
		}
		else return false;
	}
	return true;
}

std::string u8charat(const std::string &s, size_t &upos) //inc's upos to next utf8-char
{
	std::string sr{};
	if (upos<s.size())
	{
		uint8_t n, c;
		c=(uint8_t)s.at(upos);
		n=((c&0xe0)==0xc0)?2:((c&0xf0)==0xe0)?3:((c&0xf8)==0xf0)?4:1;
		sr=s.substr(upos, n);
		upos+=n;
	}
	return sr;
}

size_t u8charcount(const std::string &s)
{
	size_t i{0}, nc{0};
	while (i<s.size())
	{
		unsigned char c=(unsigned char)s.at(i);
		nc++;
		i+=((c&0xe0)==0xc0)?2:((c&0xf0)==0xe0)?3:((c&0xf8)==0xf0)?4:1;
	}
	return nc;
}

std::string u8substr(const std::string &s, size_t pos, size_t len) //like std::string::substr()
{
	if (!len) len=s.size();
	if (len&&(pos<s.size()))
	{
		size_t i=0, n, nc=0, mc=0, f=0;
		unsigned char c;
		mc=(pos+len);
		while (i<s.size())
		{
			if (nc==pos) f=i; else if (nc==mc) { return s.substr(f, i-f); }
			c=(unsigned char)s.at(i);
			n=((c&0xe0)==0xc0)?2:((c&0xf0)==0xe0)?3:((c&0xf8)==0xf0)?4:1;
			i+=n;
			nc++;
		}
		if (nc>pos) return s.substr(f);
	}
	return std::string();
}

size_t u8charpostobytepos(const std::string &s, size_t pos)
{
	size_t n, i{0}, nc{0}; //n=utility, i=byte-count, nc=charcount
	unsigned char c; //byte-value at i
	while (i<s.size())
	{
		if (nc==pos) break;
		c=(unsigned char)s.at(i);
		n=((c&0xe0)==0xc0)?2:((c&0xf0)==0xe0)?3:((c&0xf8)==0xf0)?4:1;
		i+=n;
		nc++;
	}
	return i; //byte-pos or string-size
}

void u8insert(std::string &su8, const std::string &sins, size_t pos)
{
	size_t bp=u8charpostobytepos(su8, pos);
	if (bp<su8.size())
	{
		std::string s;
		s=says(su8.substr(0, bp), sins, su8.substr(bp));
		su8=s;
	}
	else su8+=sins;
}


//--------------------------------------------------------------------------------------------------
//simple encryption ... todo check if safe?!?
void encs(const std::string &raw, std::string &enc, const std::string &pw)
{
	enc.clear();
	if (raw.empty()) return;
	size_t i, n=raw.size();
	enc+=raw[0];
	for (i=1; i<n; i++) { enc+=(uint8_t)cmod(((uint8_t)raw[i]+(uint8_t)enc[i-1]), 255); }
	if (!pw.empty())
	{
		n=pw.size();
		while (n-->0) { for (i=0; i<enc.size(); i++) { enc[i]=(uint8_t)cmod(((uint8_t)enc[i]+(uint8_t)pw[i%pw.size()]), 255); }}
	}
}

void decs(const std::string &enc, std::string &raw, const std::string &pw)
{
	raw.clear();
	if (enc.empty()) return;
	std::string t=enc;
	size_t i, n;
	if (!pw.empty())
	{
		n=pw.size();
		while (n-->0) { for (i=0; i<t.size(); i++) { t[i]=(uint8_t)cmod(((uint8_t)t[i]-(uint8_t)pw[i%pw.size()]), 255); }}
	}
	n=t.size();
	raw+=t[0];
	for (i=1; i<n; i++) { raw+=(uint8_t)cmod(((uint8_t)t[i]-(uint8_t)t[i-1]), 255); }
}

std::string encs(const std::string &raw, const std::string &pw) { std::string e{}; encs(raw, e, pw); return e; }
std::string decs(const std::string &enc, const std::string &pw) { std::string r{}; decs(enc, r, pw); return r; }


//--------------------------------------------------------------------------------------------------
size_t splitslist(const std::string &list, char delim, VSTR &vs, bool bIncEmpty)
{
	std::stringstream ss(list);
	std::string s;
	//NB: do not clear vs (may contain preset-values) - caller's task; - just add to it
	while (std::getline(ss, s, delim))
	{
		TRIM(s);
		if (!s.empty()||bIncEmpty) vs.push_back(s);
	}
	return vs.size();
}

//size_t splitsslist(const std::string &list, const std::string &delim, VSTR &vs, bool bIncEmpty)
size_t splitslist(const std::string &list, const std::string &delim, VSTR &vs, bool bIncEmpty)
{
	std::string s="";
	size_t p=0, pp=0, n=delim.size();
	while ((p=list.find(delim.c_str(), pp))!=std::string::npos)
	{
		s=list.substr(pp, p-pp);
		if (!s.empty()||bIncEmpty) vs.push_back(s);
		pp=(p+n);
	}
	if (pp<list.size()) s=list.substr(pp); else s="";
	if (!s.empty()||bIncEmpty) vs.push_back(s);
	return vs.size();
}

//size_t splitqslist(const std::string &list, char delim, VSTR &vs, bool bIncEmpty)
//{
//	//std::stringstream ss(list);
//	//std::string s;
//	//NB: do not clear vs - may be preset with defaults
//
//	///todo:... //quoted strings
//	//quoted list[0]==quote sing/dbl
//
//	//while (std::getline(ss, s, delim)) { TRIM(s); if (!s.empty()||bIncEmpty) vs.push_back(s); }
//	return vs.size();
//}

void splitslen(const std::string &src, size_t len, VSTR &vs)
{
	size_t i=0, j=0;
	vs.clear();
	std::string s{};
	while (i<src.size()) { s+=src[i]; j++; if (j==(len-1)) { vs.push_back(s); s.clear(); j=0; } i++; }
	if (j>0) {  vs.push_back(s); }
}

void splitslr(std::string s, char cdiv, std::string &l, std::string &r)
{
	size_t p;
	r.clear();
	if ((p=s.find(cdiv))!=std::string::npos) { l=s.substr(0, p); TRIM(l); r=s.substr(p+1); TRIM(r); }
	else l=s;
}

void splitslr(std::string s, const std::string &sdiv, std::string &l, std::string &r)
{
	size_t p;
	r.clear();
	if ((p=s.find(sdiv.c_str()))!=std::string::npos) { l=s.substr(0, p); TRIM(l); r=s.substr(p+sdiv.size()); TRIM(r); }
	else l=s;
}

//--------------------------------------------------------------------------------------------------
//replaced with explicitly specializations of ensv & desv templates
//const std::string vtos(const VSTR &v, char delim)
//{
//	std::string list("");
//	std::ostringstream oss("");
//	for (auto t:v) { if (!oss.str().empty()) oss << delim; oss << t; }
//	list=oss.str();
//	return list;
//}
//
//VSTR stov(const std::string &list, char delim)
//{
//	VSTR v;
//	if (list.empty()) return v;
//	std::string s{};
//	int i=0, n=list.size();
//	while (i<n)
//	{
//		if (list[i]!=delim) { s+=list[i]; }
//		else { v.push_back(s); s.clear(); }
//		i++;
//	}
//	if (!s.empty()||(list[list.size()-1]==delim)) v.push_back(s);
//	return v;
//}


//--------------------------------------------------------------------------------------------------
const std::string B64REF="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool encode_b64(const std::string &sfrom, std::string &result)
{
	result.clear();
	if (sfrom.empty()) return true; //empty==empty!
	
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    size_t l=sfrom.size(), ll{0};

	in=(const unsigned char*)sfrom.data();
    end = in+l;
    ll=4*((l+2)/3); /* 3-byte blocks to 4-byte */
    if (ll<l) return false;

    result.resize(ll);
    out=(unsigned char*)result.data();
    pos=out;
    while ((end-in)>=3)
    {
        *pos++ = B64REF[in[0] >> 2];
        *pos++ = B64REF[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = B64REF[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = B64REF[in[2] & 0x3f];
        in+=3;
    }

    if (end-in)
    {
        *pos++ = B64REF[in[0] >> 2];
        if ((end-in)==1)
        {
            *pos++ = B64REF[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = B64REF[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = B64REF[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    return true;
}

const int B64INDEX[123] =
{	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
	0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0, 63,
	0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
bool decode_b64(const std::string &sfrom, std::string &result)
{
	result.clear();
	if (sfrom.empty()) return true; //empty==empty!
	
    const unsigned char* p = (const unsigned char*)sfrom.data();
    size_t l=sfrom.size();
    
    int pad=((l>0) && ((l%4) || (p[l-1]=='=')));
    const size_t ll = (((l+3)/4-pad)*4);
    size_t lr=((ll*3/4)+pad);
    result.resize(lr, '\0');

    for (size_t i=0, j=0; i<ll; i+=4)
    {
        int n = B64INDEX[p[i]] << 18 | B64INDEX[p[i+1]] << 12 | B64INDEX[p[i+2]] << 6 | B64INDEX[p[i+3]];
        result[j++] = n >> 16;
        result[j++] = n >> 8 & 0xFF;
        result[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64INDEX[p[ll]] << 18 | B64INDEX[p[ll+1]] << 12;
        result[lr-1] = n >> 16;

        if ((l>(ll+2)) && (p[ll+2]!='='))
        {
            n |= B64INDEX[p[ll+2]] << 6;
            result.push_back(n >> 8 & 0xFF);
        }
    }
    return true;

}


//--------------------------------------------------------------------------------------------------
bool GetSystemEnvironment(SystemEnvironment &SE)
{
	SE.clear();
	
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
//#include <paths.h>
	char *pc;
	std::string s;
	size_t p;
	for (int i=0; ((pc=environ[i])!=nullptr); i++)
	{
		s=pc;
		if ((p=s.find('='))!=std::string::npos) { SE[s.substr(0, p)]=s.substr(p+1); }
	}

//#elif //assume Windows
	//todo..
	
#endif
	
	return !SE.empty();
}

size_t MaxPathLength()
{
	//(it's a mess!)
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	long n = pathconf(".", _PC_PATH_MAX);
	if (n<0) n=4097; //n==-1 effectively means unlimited size
	return size_t(n);
#elif defined(_WIN64)
	return size_t(255); //actually 260
	//if long names are used, prefix path with "\\?\" for 32767 byte length
#endif
}

//--------------------------------------------------------------------------------------------------

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
struct dirent *ReadDir(DIR *pd) //wrapper to discard '.' and '..' entries
{
	auto isdots=[](std::string s)->bool{ return ((s==".")||(s=="..")); };
	struct dirent *pde;
read_again: pde=readdir(pd);
	if (pde&&(isdots(pde->d_name))) goto read_again;
	return pde;
}
#endif

std::string FILESYS_ERROR;
inline void clear_fs_err() { FILESYS_ERROR.clear(); }
inline bool fs_err(const std::string &e) { FILESYS_ERROR=e; return false; }
std::string filesys_error() { return FILESYS_ERROR; }

FST direnttypetoFST(unsigned char uc)
{
	switch(uc)
	{
		case DT_BLK: return FST_BDEV;
		case DT_CHR: return FST_CDEV;
		case DT_DIR: return FST_DIR;
		case DT_FIFO: return FST_PIPE;
		case DT_LNK: return FST_LINK;
		case DT_REG: return FST_FILE;
		case DT_SOCK: return FST_SOCK;
		case DT_UNKNOWN: default: return FST_UNKNOWN;
	}
}

FST modetoFST(int stm)
{
	switch(stm&S_IFMT)
	{
		case S_IFDIR: return FST_DIR;
		case S_IFCHR: return FST_CDEV;
		case S_IFBLK: return FST_BDEV;
		case S_IFREG: return FST_FILE;
		case S_IFIFO: return FST_PIPE;
		case S_IFLNK: return FST_LINK;
		case S_IFSOCK: return FST_SOCK;
		default: return FST_UNKNOWN;
	}
}
FST FSYStoFST(FSYS::file_type ft)
{
	switch(ft)
	{
		case FSYS::file_type::directory:	return FST_DIR;
		case FSYS::file_type::regular:		return FST_FILE;
		case FSYS::file_type::fifo:			return FST_PIPE;
		case FSYS::file_type::symlink:		return FST_LINK;
		case FSYS::file_type::block:		return FST_BDEV;
		case FSYS::file_type::character:	return FST_CDEV;
		case FSYS::file_type::socket:		return FST_SOCK;
		default:							return FST_UNKNOWN;
	}
}

std::string FSTName(FST fst, bool bshort)
{
	switch(fst)
	{
		case FST_FILE: return((bshort)?"Fil":"File");
		case FST_DIR: return((bshort)?"Dir":"Directory");
		case FST_LINK: return((bshort)?"Lnk":"Link");
		case FST_PIPE: return((bshort)?"Ffo":"Pipe");
		case FST_SOCK: return((bshort)?"Sck":"Socket");
		case FST_BDEV: return((bshort)?"Blk":"BlockDevice");
		case FST_CDEV: return((bshort)?"Chr":"CharDevice");
		case FST_UNKNOWN: default: return((bshort)?"Unk":"Unknown");
	}
};

//-------------------------------------------------------MIME
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
#include <magic.h>
struct MimeInfo : NOCOPY
{
	magic_t M{nullptr};
	bool bmok{false};
	~MimeInfo() { if (bmok) magic_close(M); }
	MimeInfo() { if ((bmok=((M=magic_open(MAGIC_MIME_TYPE))))) magic_load(M, NULL); }
	bool IsOK() { return bmok; }
	std::string Get(const std::string &se)
		{
			std::string sm{};
			if (bmok) sm=magic_file(M, se.c_str());
			return sm;
		}

	std::string GetExts(const std::string &se) //returns CSV
		{
			std::string sx{};
			magic_t MM{nullptr};
			if ((MM=magic_open(MAGIC_EXTENSION))) //OR'ing with MAGIC_MIME_TYPE ignores MAGIC_MIME_TYPE values
			{
				magic_load(MM, NULL);
				const char *psz=magic_file(MM, se.c_str());
				if (psz) sx=psz;
				magic_close(MM);
				MM=nullptr;
				TRIM(sx, " ?");
				ReplaceChars(sx, "/", ",");
			}
			return sx;
		}
};
#elif defined(_WIN64)
struct MimeInfo : NOCOPY ///TODO ---
{
	~MimeInfo() {}
	MimeInfo() {}
	bool IsOK() return false; }
	std::string Get(const std::string &se) { return ""; }
	std::string GetExts(const std::string &se) { return ""; }
};
#endif

MimeInfo MIMEINF; //make available
inline std::string GetMimeType(const std::string &se) { return MIMEINF.Get(se); }
inline std::string GetMimeExtentions(const std::string &se) { return MIMEINF.GetExts(se); }

//-------------------------------------------------------statx
bool getstatx(FSInfo &FI, const std::string &sfe, MimeFlag mimeflag)
{
	std::string sf{sfe};
	TRIM(sf);
	if (sf[0]!='/')
	{
		//sf=path_append(getcurpath(), sf);
	 return sayerr("getstatx: full path required");
	}
	
	struct statx stx;
	unsigned int mask, flags;
	
	FI.clear();
	mask=STATX_ALL;
	flags=AT_NO_AUTOMOUNT|AT_SYMLINK_NOFOLLOW;
	int r=statx(AT_FDCWD, sf.c_str(), flags, mask, &stx);
	if (!r)
	{
		FI.path=sf;
		FI.owner_id=stx.stx_uid;
		FI.group_id=stx.stx_gid;

		switch (stx.stx_mode & S_IFMT)
		{
			case S_IFIFO:	FI.type=FST_PIPE; break;
			case S_IFCHR:	FI.type=FST_CDEV; break;
			case S_IFDIR:	FI.type=FST_DIR; break;
			case S_IFBLK:	FI.type=FST_BDEV; break;
			case S_IFREG:	FI.type=FST_FILE; break;
			case S_IFLNK:	{ FI.type=FST_LINK; FI.linktarget=getlinktarget(sf); } break;
			case S_IFSOCK:	FI.type=FST_SOCK; break;
			default: FI.type=FST_UNKNOWN; break;
		}
		
		FI.rights_owner=((stx.stx_mode&S_IRUSR)?4:0)+((stx.stx_mode&S_IWUSR)?2:0)+((stx.stx_mode&S_IXUSR)?1:0);
		FI.rights_group=((stx.stx_mode&S_IRGRP)?4:0)+((stx.stx_mode&S_IWGRP)?2:0)+((stx.stx_mode&S_IXGRP)?1:0);
		FI.rights_other=((stx.stx_mode&S_IROTH)?4:0)+((stx.stx_mode&S_IWOTH)?2:0)+((stx.stx_mode&S_IXOTH)?1:0);

		FI.bytesize=stx.stx_size;

		auto todts = [&](struct statx_timestamp &ts)->DTStamp
			{
				struct tm *ptm;
				time_t tt=ts.tv_sec;
				if ((ptm=localtime(&tt))) return make_dtstamp(ptm, ts.tv_nsec);
				return (DTStamp)0;
			};
		
		FI.dtaccess=todts(stx.stx_atime);
		FI.dtcreate=todts(stx.stx_btime);
		FI.dtmodify=todts(stx.stx_ctime);
		FI.dtstatus=todts(stx.stx_mtime);
		
		if (mimeflag!=MF_NONE)
		{
			std::string sm{}, sx{};
			sm=GetMimeType(sf);
			if (mimeflag==MF_MIMEEXT) { sx=GetMimeExtentions(sf); if (!sx.empty()) sayss(sm, " (", sx, ")"); }
			FI.mimetype=sm;
		}

	}
	//else - use errno etc...
	return (!r);
}


//------------------------------------------------
void DirTree::rec_read(const std::string &sdir, DirTree &dt)
{
	dt.clear();
	dt.dirpath=sdir;
	DirEntries de; de.clear();
	dir_read(sdir, de);
	auto isdots=[](std::string s)->bool{ return ((s==".")||(s=="..")); };
	if (!de.empty())
	{
		for (auto p:de)
		{
			if (isdirtype(p.second)) { if (!isdots(p.first)) rec_read(path_append(sdir, p.first), dt[p.first]); } //toss the dots
			else dt.content[p.first]=p.second;
		}
	}
}

DirTree::DirTree() : dirpath("") { clear(); }
DirTree::~DirTree() {}

bool DirTree::Read(const std::string &sdir)
{
	clear();
	if (!dir_exist(sdir)) return fs_err(says("DirTree: directory '", sdir, "' does not exist"));
	rec_read(sdir, *this);
	return true;
}

size_t DirTree::Size0(size_t &n, size_t &f)
{
	n+=size();
	f+=content.size();
	for (auto d:(*this)) d.second.Size0(n, f);
	return (n+f);
}

size_t DirTree::Size(size_t *pnsubs, size_t *pnfiles)
{
	size_t t=0, n=0, f=0;
	t=Size0(n, f);
	if (pnsubs) *pnsubs=n;
	if (pnfiles) *pnfiles=f;
	return t;
}

//------------------------------------------------
size_t fssize(const std::string &sfs)
{
	size_t n=0;
	FSInfo fi;
	if (getstatx(fi, sfs)) n=fi.bytesize;
	return n;
}

//------------------------------------------------
std::string path_append(const std::string &sPath, const std::string &sApp)
{
	if (sApp.empty()) return sPath;
	std::string sl=sPath, sr=sApp;
	std::string sep{};
	static std::string spa{};
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	sep="/";
#elif defined(_WIN64)
	sep="\\";
#endif
	RTRIM(sl, sep.c_str());
	LTRIM(sr, sep.c_str());
	spa=says(sl,sep,sr);
	return spa;
}

bool path_realize(const std::string &spath)
{
	if (dir_exist(spath)) return true;
	clear_fs_err();
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	VSTR vp{};
	VSTR vpp{};
	if (splitslist(spath, '/', vp, false)>0)
	{
		std::string sp{"/"};
		bool b{true};
		auto it=vp.begin();
		while (b&&(it!=vp.end())) { sp=path_append(sp, *it++); if (!dir_exist(sp)) { if ((b=dir_create(sp))) vpp.push_back(sp); }}
		if (!b) { for (auto s:vpp) dir_delete(s); return fs_err(says("path_realize: '", spath, "' - ", filesys_error())); }
	}
	else return fs_err(says("path_realize: '", spath, "' - invalid path"));
#elif defined(_WIN64)
	///todo: test &/ chg to use windows funcs...
	std::error_code ec;
	if (FSYS::create_directories(spath, ec)) return true;
	else return fs_err(says("path_realize: '", spath, "' - ", ec.message()));
#endif 
	return true;
}
	
std::string path_path(const std::string &spath) //returns all to Left of last dir-separator
{
	std::string r(spath);
	char sep{'/'};
	size_t p{0};
#if defined(_WIN64)
	sep='\\';
#endif
	if ((r.size()>1)&&(r[r.size()-1]==sep)) { RTRIM(r, std::string{sep}.c_str()); return r; }
	p=r.rfind(sep);
	if (p!=std::string::npos) r=r.substr(0, p); else r.clear();
	return r;
}

std::string path_name(const std::string &spath)
{
	std::string r(spath);
	char sep{'/'};
	size_t p{0};
#if defined(_WIN64)
	sep='\\';
#endif
	if (r.size()>1) RTRIM(r, std::string{sep}.c_str());
	p=r.rfind(sep);
	if ((p!=std::string::npos)&&(p<r.size())) r=r.substr(p+1);
	return r;
}

std::string path_time(const std::string &spath)
{
	std::string sdt{};
	FSInfo fi;
	if (getstatx(fi, spath)) sdt=ToDateStr(MAX(fi.dtaccess, fi.dtmodify), DS_SQUASH); //last read|write
	return sdt;
}

std::string path_time_h(const std::string &spath) ///todo remove this func
{
	std::string sdt{};
	FSInfo fi;
	if (getstatx(fi, spath)) sdt=ToDateStr(MAX(fi.dtaccess, fi.dtmodify), DS_COMPACT);
	return sdt;
}

bool path_max_free(const std::string spath, size_t &max, size_t &free)
{
	max=free=0;
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)

	struct statvfs stv;
	if (statvfs(spath.c_str(), &stv)!=0) return fs_err(says("path_max_free: ", std::strerror(errno)));
	max=stv.f_blocks*stv.f_frsize;
	free=stv.f_bavail*stv.f_frsize;
	return true;
#elif defined(_WIN64)
	max=free=0;
	std::error_code ec;
	if (FSYS::exists(spath, ec))
	{
		try { FSYS::space_info si=FSYS::space(spath); max=si.capacity; free=si.free; return true; }
		catch(FSYS::filesystem_error &e) { return fs_err(says("path_space: '", spath, "' - ", e.what())); }
	}
	return fs_err(says("path_max_free: invalid path '", spath, "'"));
#endif

}

//------------------------------------------------
std::string getcurpath()
{
	size_t MPS=MaxPathLength();
	std::string s{};
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	char buf[MPS];
	if (getcwd(buf, MPS)!=nullptr) s=buf;
#elif defined(_WIN64)
	s=(std::string)FSYS::current_path();
#endif
	return s;
}

FST getfsitemtype(const std::string &se)
{
	FST ft=FST_UNKNOWN;
	fsexist(se, &ft);
	return ft;
}

bool fsexist(const std::string &sfs, FST *ptype)
{
	bool b{false};
	FSInfo fi;
	if ((b=getstatx(fi, sfs))) if (ptype) *ptype=fi.type;
	return b;
}

bool fsdelete(const std::string fd)
{
	clear_fs_err();
	if (!canwrite(fd)) return fs_err(says("fsdelete: '", fd, "' - insufficient rights"));
	if (!fsexist(fd)) return true;
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	if (isdir(fd))
	{
		DIR *pD;
		struct dirent *pDE;
		bool b{true};
		pD=opendir(fd.c_str());
		if (pD)
		{
			while (b&&((pDE=ReadDir(pD))!=nullptr)) { b=fsdelete(path_append(fd, pDE->d_name)); }
			closedir(pD);
		}
		if (b) rmdir(fd.c_str());
	}
	else if (unlink(fd.c_str())!=0) return fs_err(says("fsdelete: ", std::strerror(errno)));
#elif defined(_WIN64)
	std::error_code ec;
	FSYS::remove_all(fd, ec);
	if (ec) return fs_err(says("fsdelete: '", fd, "' - ", ec.message()));
#endif
	return !fsexist(fd);
}

bool fsrename(const std::string &x, const std::string &n) //x=existing full path, n=new name only
{
	clear_fs_err();
	if (!fsexist(x)) return fs_err(says("fsrename: '", x, "' does not exist"));
	if (!isvalidname(n)) return fs_err(says("fsrename: invalid new name '", n, "'"));
	std::string t=path_append(path_path(x), n);
	if (fsexist(t)) return fs_err(says("fsrename: '", t, "' already exist"));
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	if (rename(x.c_str(), t.c_str())!=0) return fs_err(says("fsrename: ", std::strerror(errno)));
#elif defined(_WIN64)
	std::error_code e;
	FSYS::rename(sfile, snd, e);
	if (e) return fs_err(says("fsrename: '", x, "' to '", t, "' - ", e.message()));
#endif
	return (!fsexist(x)&&fsexist(t));
}

bool fscopy(const std::string &src, const std::string &dest, bool bBackup, bool bIgnoreSpecial)
{
	clear_fs_err();
	if (!fsexist(src)) return fs_err(says("fscopy: source '", src, "' does not exist"));
	bool bret{false};
	
	//TO_DO_MSG("copying links(soft/hard), pipes, sockets, devices ... use 'Clone' ")
//	TODO("copying links(soft/hard), pipes, sockets, devices ... use 'Clone' ");
	
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	
	if (isdir(src))
	{
		if (dir_exist(dest)) { if (bBackup) dir_backup(dest); }
		if (seqs(src, dest)) { if (bBackup) return true; return fs_err(says("fscopy: source & destination are same")); }
		if (issubdir(dest, src)) return fs_err(says("fscopy: destination is sub-directory of source"));
		if (!dir_create(dest)) return false;
		DIR *pD;
		struct dirent *pDE;
		if ((pD=opendir(src.c_str())))
		{
			std::string ss{}, sd{};
			bret=true;
			while (bret&&((pDE=ReadDir(pD))!=nullptr))
			{
				ss=path_append(src, pDE->d_name);
				sd=path_append(dest, pDE->d_name);
				bret=fscopy(ss, sd);
			}
			closedir(pD);
		}
		else bret=false;
	}
	else if (isfile(src))
	{
		if (bBackup&&fsexist(dest)) file_backup(dest);
		//if (!seqs(src, dest)) --- overwrite it if it exists
		//{
			std::ifstream ifs(src, std::ios::binary);
			std::ofstream ofs(dest, std::ios::binary);
			if (ifs.good()&&ofs.good()) { ofs << ifs.rdbuf(); ofs.close(); ifs.close(); }
		
/*alternatively...
		int ifd, ofd;
		size_t n=0, N=8192;
		char buf[N];
		bool b{true};
		if ((ifd=open(src, O_RDONLY))>=0)
		{
			if ((ofd=open(dest, O_WRONLY))>=0)
			{
				while (b)
				{
					if ((b=((n=read(ifd, buf, N))>0)))
					{
						b=(write(ofd, buf, n)!=n);
					}
					else break; //done
				}
				close(ofd);
			}
			close(ifd);
		}
*/

		//}
		bret=fsexist(dest);
	}
	else
	{
		//pipe/socket/device/link
		// - rather use System(tar...) for archiving
		// - or System(rsync...) for sync/mirror
		// - or System(cp...)
		bret=(bIgnoreSpecial|fs_err(says("fscopy: '", src, "' is a ", GetFSType(src), " - use {Tarball|Synchronize|Archive} to copy")));
	}
	
#elif defined(_WIN64)

	std::error_code ec;
	FSYS::copy(src, Dest, FSYS::copy_options::recursive|FSYS::copy_options::copy_symlinks, ec);
	if (ec) return fs_err(says("fscopy: '", src, "' to '", Dest, "' - ", ec.message()));
	bret=true;

#endif
	return bret;
}

bool issubdir(const std::string &Sub, const std::string &Dir)
{
	std::string sD, sS;
	if (!isdir(Dir)) sD=path_path(Dir); else sD=Dir;
	size_t ld=sD.size();
	if (Sub.size()<ld) return false; //==dir is sub of itself
	sS=Sub.substr(0, ld);
	return seqs(sS, sD);
/*
	if (!isdir(Dir)) return false; //Sub may be a file or dir
	VSTR vs{}, vd{};
	size_t i=0, ns, nd;
	bool b{true};
	ns=splitslist(Sub, '/', vs, false);
	nd=splitslist(Dir, '/', vd, false);
	if (ns>=nd) { while (b&&(i<nd))
	 {
	  b=seqs(vd[i], vs[i]);
	  i++;
	  }}
	return b;
*/
}

bool isdirempty(const std::string &D)
{
	if (!dir_exist(D)) return false; //??? not exist should imply empty ???
	DirEntries md;
	if (dir_read(D, md)) return md.empty();
	return false;
}

bool realizetree(const std::string &sdir, DirTree &tree)
{
	std::string sd("");
	bool b=true;
	auto pit=tree.begin();
	while (b&&(pit!=tree.end()))
	{
		sd=path_append(sdir, pit->first);
		if ((b=path_realize(sd))) b=realizetree(sd, pit->second);
	}
	return b;
}

std::string getrelativepathname(const std::string &sroot, const std::string &spath)
{
	size_t n=sroot.size();
	std::string srel("");
	if ((n<spath.size())&&(seqs(sroot, spath.substr(0,n)))) { srel=spath.substr(n+1); LTRIM(srel,"/"); }
	return srel;
}

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
std::string getlinktarget(const std::string &se) //was getsymlinktarget
{
	size_t MPS=MaxPathLength();
	std::string sl("");
	char buf[MPS];
	int i=0, n;
	n=readlink(se.c_str(), buf, MPS);
	for (;i<n;i++) sl+=buf[i];
	sl+='\0';
	if (sl[0]!='/') { sl=path_append(path_path(se), sl); }
	return sl;
}
#endif

bool isscriptfile(const std::string sF) //bash, perl, (checks #!)
{
	if (canexecute(sF))
	{
		std::ifstream inf(sF);
		std::string s;
		getline(inf, s);
		TRIM(s);
		return seqs(s.substr(0,2), "#!");
	}
	return false;
}

bool isnontextfile(const std::string sF)
{
	//crude check for null-byte in first 2000 bytes of file
	std::string d{};
	if (file_read(sF, d)&&!d.empty())
	{
		for (size_t i=0; i<std::min(d.size(),size_t(2000)); i++) { if (!d[i]) return true; }
	}
	return false;
}

bool istextfile(std::string sf)
{
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	return istxtfile(sf);
#else
	return is_text_file(sf);
#endif	
}

//todo...following is..() are q&d's, fix proper, check magic-bytes, sig's etc...
bool ispicturefile(std::string sf) //jpg/png/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" bmp gif jpg jpeg pcx ppm png svg tif xpm xps ").find(sx)!=std::string::npos);
}

bool isanimationfile(std::string sf) //gif/tif/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" gif tif ").find(sx)!=std::string::npos); //png etc ?...
}

bool issoundfile(std::string sf)
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" mp3 ").find(sx)!=std::string::npos);
}

bool isvideofile(std::string sf) //mpg/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" avi mkv mp4 mpg mpeg mov vob ").find(sx)!=std::string::npos);
}

bool isarchivefile(std::string sf) //zip/gz/bz2/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" z zip 7z gz bz2 ").find(sx)!=std::string::npos);
}

bool isdocumentfile(std::string sf) //doc/odf/ods/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" doc xls odf ods ").find(sx)!=std::string::npos);
}

bool isdatabasefile(std::string sf) //sqlite/xml/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" sqlite sqlite3 xml ").find(sx)!=std::string::npos);
}

bool issourcecodefile(std::string sf) //c/cpp/h/...
{
	if (isscriptfile(sf)) return true;
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" c h cpp hpp ").find(sx)!=std::string::npos);
}

bool iswebfile(std::string sf) //htm[l]/css/
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=says(" ", sx, " ");
	return (std::string(" htm html css js ").find(sx)!=std::string::npos);
}

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
#include <magic.h>
	bool check_magic_mimetype(std::string sf, std::string mt)
	{
		magic_t ck = magic_open(MAGIC_MIME_TYPE);
		magic_load(ck, 0);
		const char *psz = magic_file(ck, sf.c_str());
		std::string s{};
		if (psz) s=psz;
		magic_close(ck);
		return (sieqs(s, mt)||sieqs(s, mt.substr(0, s.size())));
	}
	bool get_mime_info(std::string sf, std::string &smime, std::string &scharset)
	{
		magic_t ck = magic_open(MAGIC_MIME);
		magic_load(ck, 0);
		const char *psz = magic_file(ck, sf.c_str());
		std::string s{};
		if (psz) s=psz;
		magic_close(ck);
		if (!s.empty())
		{
			size_t p=s.find(';');
			if (p!=std::string::npos)
			{
				smime=s.substr(0,p);
				if ((p=s.find('=', p+1))!=std::string::npos)
				{
					scharset=s.substr(p+1);
					return true;
				}
			}
		}
		return false;
	}
	bool istxtfile(std::string sf) { return check_magic_mimetype(sf, "text/plain"); }
	bool ishtmlfile(std::string sf) { return check_magic_mimetype(sf, "text/html"); }
	bool isxmlfile(std::string sf) { return check_magic_mimetype(sf, "text/xml"); }
	bool isodtfile(std::string sf) { return check_magic_mimetype(sf, "application/vnd.oasis.opendocument.text"); }
	bool isdocfile(std::string sf) { return check_magic_mimetype(sf, "application/msword"); }
	bool isdocxfile(std::string sf) { return check_magic_mimetype(sf, "application/vnd.openxmlformats-officedocument.wordprocessingml.document"); }
	bool isrtffile(std::string sf) { return check_magic_mimetype(sf, "text/rtf"); }
	bool ispdffile(std::string sf) { return check_magic_mimetype(sf, "application/pdf"); }
	
#elif defined(_WIN64)
	bool match_extension(std::string sf, std::string sxs)
	{
		std::string sx=file_extension(sf);
		if (sx.empty()) return false;
		tolcase(sx);
		sx=says(" ", sx, " ");
		return (sxs.find(sx)!=std::string::npos);
	}
	bool get_mime_info(std::string sf, std::string &smime, std::string &scharset) //todo...
	{
		return false();
		/*
		see: https://docs.microsoft.com/en-us/dotnet/api/system.web.mimemapping.getmimemapping?redirectedfrom=MSDN&view=netframework-4.8#System_Web_MimeMapping_GetMimeMapping_System_String_
		this (may?) get the mime-type
		still need charset: may have to scan the file bytes for ascii, \0=>binary, >\x80-utf8, ?wchar?, ...
		*/
	}
	bool istxtfile(std::string sf) { return istextfile(sf); }
	bool ishtmlfile(std::string sf) { return match_extension(sf, " htm html "); }
	bool isxmlfile(std::string sf) { return match_extension(sf, " xml "); }
	bool isodtfile(std::string sf) { return match_extension(sf, " odt "); }
	bool isdocfile(std::string sf) { return match_extension(sf, " doc "); }
	bool isdocxfile(std::string sf) { return match_extension(sf, " docx "); }
	bool isrtffile(std::string sf) { return match_extension(sf, " rtf "); }
	bool ispdffile(std::string sf) { return match_extension(sf, " pdf "); }

#endif

DTStamp get_dtmodified(const std::string sf)
{
	return ToDTStamp(path_time(sf));
}

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)

	std::string get_owner(const std::string sf)
	{
		std::string so("unknown");
		FSInfo fi;
		if (getstatx(fi, sf)) so=username(fi.owner_id);
		return so;
	}

	bool isexefile(const std::string sF) //check if sF is an elf-binary
	{
		if (canexecute(sF))
		{
			char buf[5];
			std::ifstream inf(sF);
			for (int i=0;i<5;i++) inf >> buf[i];
			return ((buf[0]==0x7F)&&(buf[1]=='E')&&(buf[2]=='L')&&(buf[3]=='F'));
		}
		return false;
	}

#elif defined(_WIN64)

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "accctrl.h"
#include "aclapi.h"
//#pragma comment(lib, "advapi32.lib")

	std::string get_owner(const std::string &sf)
	{
		HANDLE hFile;
		PSID pSidOwner=NULL;
		//BOOL b=TRUE;
		LPTSTR AcctName=NULL;
		LPTSTR DomainName=NULL;
		DWORD dwAcctName=1, dwDomainName=1;
		SID_NAME_USE eUse=SidTypeUnknown;
		PSECURITY_DESCRIPTOR pSD=NULL;
	
		if ((hFile=CreateFile(sf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE) return "unknown";
		if (GetSecurityInfo(hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pSidOwner, NULL, NULL, NULL, &pSD)!=ERROR_SUCCESS) return "unknown";
		//b=LookupAccountSid(NULL, pSidOwner, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse); //wtf's up with b? todo...
		LookupAccountSid(NULL, pSidOwner, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse); //get sizes
		if (!(AcctName=(LPTSTR)GlobalAlloc(GMEM_FIXED, dwAcctName))) return "unknown";
	    if (!(DomainName=(LPTSTR)GlobalAlloc(GMEM_FIXED, dwDomainName))) return "unknown";
	    if (!LookupAccountSid(NULL, pSidOwner, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse)) return "unknown";
	    return (std::string)AcctName;															//?need to return std::string...todo
	}

	bool isexefile(const std::string sF)
	{
		if (canexecute(sF))
		{
			std::string s=sF.substr(sF.size()-4);
			toucase(s);
			return seqs(s, ".EXE"); //cop-out q&d
		}
		return false;
	}
	
#endif


//------------------------------------------------
int getpermissions(const std::string sN)
{
//	FSInfo fi;
//	if (getstatx(fi,sN)) return fi.???
		
//	if (fsexist(sN)) return (int)FSYS::status(sN).permissions();
//	fs_err(says("getpermissions: '", sN, "' - not found"));
	sayerr("getpermissions - not yet implemented correctly"); //fixme todo
	return 0;
}

bool setpermissions(const std::string, int, bool)//(const std::string sN, int prms, bool bDeep) pedantic
{
	//todo...
	// must do this bottom-up
	// setting permission e.g.: read-only on parent will prevent setting permissions on sub-dirs/files
	// .....
	return false;
/*
	if (fsexist(sN))
	{
		std::error_code ec;
		bool b=true;
		//FSYS::permissions(sN, perms, ec); - does not seem to be implemented yet
		//if (ec) return fs_err(says("setpermissions: '", sN, "' - ", ec.message())); &fix for deep..
		try
		{
			FSYS::permissions(sN, (FSYS::perms)prms);
			if (bDeep&&isdir(sN))
			{
				DirEntries c;
				if (dir_read(sN, c)) { for (auto p:c) if (!(b=setpermissions(p.first, bDeep))) break; }
			}
			return b;
		}
		catch(FSYS::filesystem_error e) { return fs_err(says("setpermissions: '", sN, "' - ", e.what())); }
		catch(...) { return fs_err(says("setpermissions: '", sN, "' - failed with some exception..")); }
	}
	return fs_err(says("setpermissions: '", sN, "' - not found"));
*/
}

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)

	std::string getfullrights(const std::string sf) //ogo
	{
		std::string sr{"--- --- ---"};
		FSInfo fi;
		if (getstatx(fi, sf))
		{
			auto rwx=[&sr](uint8_t u){ sr+=(u&4)?'r':'-'; sr+=(u&2)?'w':'-'; sr+=(u&1)?'x':'-'; };
			sr.clear(); rwx(fi.rights_owner);
			sr+=' ';  rwx(fi.rights_group);
			sr+=' ';  rwx(fi.rights_other);
		}
		return sr;
	}

	int getrights(const std::string sfd) //o only, using effective uid
	{
		int n=0;
		if (!euidaccess(sfd.c_str(), F_OK))
		{
			if (!euidaccess(sfd.c_str(), R_OK)) n|=4;
			if (!euidaccess(sfd.c_str(), W_OK)) n|=2;
			if (!euidaccess(sfd.c_str(), X_OK)) n|=1;
		}
		return n;
	}
	
	bool canread(const std::string sfd)
	{
		if (euidaccess(sfd.c_str(), F_OK|R_OK)) return fs_err(says("canread: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}
	
	bool canwrite(const std::string sfd)
	{
		if (euidaccess(sfd.c_str(), F_OK|W_OK)) return fs_err(says("canwrite: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}
	
	bool canexecute(const std::string sfd)
	{
		if (euidaccess(sfd.c_str(), F_OK|X_OK)) return fs_err(says("canexecute: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}

#elif defined(_WIN64)

	std::string getfullrights(const std::string sfd) //ogo
	{
		return getrightsRWX(getrights(sfd));
	}
	
	int getrights(const std::string sfd)
	{
		int n=0, p=getpermissions(sfd);
		//std::string so=get_owner(sfd);
		n=((p&(int)(FSYS::perms::owner_read|FSYS::perms::group_read|FSYS::perms::others_read))!=(int)FSYS::perms::none)?4:0;
		n+=((p&(int)(FSYS::perms::owner_write|FSYS::perms::group_write|FSYS::perms::others_write))!=(int)FSYS::perms::none)?2:0;
		n+=((p&(int)(FSYS::perms::owner_exec|FSYS::perms::group_exec|FSYS::perms::others_exec))!=(int)FSYS::perms::none)?1:0;
		return n;
	}
	
	bool canread(const std::string sfd)
	{
		if (fsexist(sfd))
		{
			int p=getpermissions(sfd);
			std::string so=get_owner(sfd);
			
			return ((p&(int)(FSYS::perms::owner_read|FSYS::perms::group_read|FSYS::perms::others_read))!=(int)FSYS::perms::none);
		}
		return fs_err(says("canread: '", sfd, "' - not found"));
	}
	
	bool canwrite(const std::string sfd)
	{
		if (fsexist(sfd))
		{
			int p=getpermissions(sfd);
			return ((p&(int)(FSYS::perms::owner_write|FSYS::perms::group_write|FSYS::perms::others_write))!=(int)FSYS::perms::none);
		}
		return fs_err(says("canwrite: '", sfd, "' - not found"));
	}
	
	bool canexecute(const std::string sfd)
	{
		if (fsexist(sfd))
		{
			int p=getpermissions(sfd);
			return ((p&(int)(FSYS::perms::owner_exec|FSYS::perms::group_exec|FSYS::perms::others_exec))!=(int)FSYS::perms::none);
		}
		return fs_err(says("canexecute: '", sfd, "' - not found"));
	}

#endif

std::string getrightsRWX(size_t r) //using retval from getrights()...
{
	std::string s{};
	s=says(((r&4)==4)?"r":"_", ((r&2)==2)?"w":"_", ((r&1)==1)?"x":"_");
	return s;
}

std::string getrightsRWX(const std::string sN)
{
	return getrightsRWX(getrights(sN));
}

//------------------------------------------------
bool dir_exist(const std::string &sdir)
{
	FST t;
	return (fsexist(sdir, &t)&&isdirtype(t));
}

bool dir_create(const std::string &sdir)
{
	clear_fs_err();
	if (dir_exist(sdir)) return true;
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
/*
	User:	S_IRUSR (read), S_IWUSR (write), S_IXUSR (execute), S_IRWXU (read+write+execute)
	Group:	S_IRGRP (read), S_IWGRP (write), S_IXGRP (execute), S_IRWXG (read+write+execute)
	Others:	S_IROTH (read), S_IWOTH (write), S_IXOTH (execute), S_IRWXO (read+write+execute)
*/
	int rwxr_xr_x=(S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	if (mkdir(sdir.c_str(), rwxr_xr_x)!=0) return fs_err(says("dir_create: '", sdir, "' - ", strerror(errno)));
#elif defined(_WIN64)
	std::error_code ec;
	if (!FSYS::create_directory(sdir, ec)) return fs_err(says("dir_create: '", sdir, "' - ", ec.message()));
#endif
	return dir_exist(sdir);
}

bool dir_delete(const std::string &sdir)
{
	return fsdelete(sdir);
}

bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly)
{
	clear_fs_err();
	if (!dir_exist(l)) return fs_err(says("dir_compare: l-directory '", l, "' does not exist"));
	if (!dir_exist(r)) return fs_err(says("dir_compare: r-directory '", r, "' does not exist"));
	
	bool b;
	DirEntries m;
	std::map<std::string, int> mc;
	auto all2=[&mc]()->bool{ if (mc.size()>0) { for (auto p:mc) { if (p.second!=2) return false; }} return true; };
	
	mc.clear();
	m.clear();
	if ((b=dir_read(l, m)))
	{
		if (m.size()>0) for (auto p:m) mc[p.first]=1;
		m.clear();
		if ((b=dir_read(r, m)))
		{
			if (m.size()>0) for (auto p:m) mc[p.first]++;
			if ((b=all2()))
			{
				auto it=m.begin();
				while (b&&(it!=m.end()))
				{
					if (it->second==FST_DIR) { b=dir_compare(path_append(l, it->first), path_append(r, it->first)); }
					else if (!bNamesOnly&&(it->second==FST_FILE)) { b=file_compare(path_append(l, it->first), path_append(r, it->first)); }
					//if !b then stuff into some diff's list... means not bail-out when b==false... - todo - see alt-proto in header
					it++;
				}
			}
		}
	}
	return b;
}

bool dir_read(const std::string &sdir, DirEntries &content, bool bFullpath) //only entries directly under sdir
{
	content.clear();
	bool bret{true};
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	///this is ~3 x faster than std::filesystem
	DIR *pD;
	struct dirent *pDE;
	pD=opendir(sdir.c_str());
	if (pD)
	{
		std::string sfn{};
		while ((pDE=ReadDir(pD))!=nullptr)
		{
			if (bFullpath) sfn=path_append(sdir, pDE->d_name); else sfn=pDE->d_name;
			content[sfn]=direnttypetoFST(pDE->d_type);
		}
		closedir(pD);
	}
	else bret=false;
#elif defined(_WIN64)
	clear_fs_err();
	if (!dir_exist(sdir)) bret=fs_err(says("dir_read: directory '", sdir, "' does not exist"));
	try
	{
		auto DI=FSYS::directory_iterator(sdir);
		auto it=begin(DI);
		for (; it!=end(DI); ++it)
		{
			TO_DO_MSG("fix to discard '.' and '..' entries") //fixme
			if (bFullpath) sfn=it->path(); else sfn=path_name(it->path());
			content[sfn]=FSYStoFST(FSYS::symlink_status(it->path()).type());
		}
	}
	catch(FSYS::filesystem_error &e) { bret=fs_err(says("dir_read: '", sdir, "' - ", e.what())); }
#endif
	return bret;
}

bool dir_rename(const std::string &sdir, const std::string &sname)
{
	return fsrename(sdir, sname);
}

bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackup)
{
	//if destdir does not exist  then itself is the target and will be created
	//else if destdir exists then sdir will be copied as a sub-dir of destdir
	//(if you want to copy the _content_ of sdir to destdir directly then you
	//must select the content and copy that)
	std::string sd{};
	if (!fsexist(destdir)) { sd=destdir; path_realize(sd); }
	else sd=path_append(destdir, path_name(sdir));
	return fscopy(sdir, sd, bBackup);
}

bool dir_sync(const std::string &sourcedir, const std::string &syncdir)
{
	
	say("need to think 'dir_sync' through..."); return false; //.........................................todo...
	
	//want to have rsync here ... check if available in a lib, or check rsync-source and replicate here
	
	//only copy if source is new/er ..
	
	
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sourcedir)) return fs_err(says("dir_sync: source directory '", sourcedir, "' does not exist"));
	if (!path_realize(syncdir)) return fs_err(says("dir_sync: target directory '", syncdir, "' does not exist")); //created
	
	FSYS::copy(sourcedir, syncdir, FSYS::copy_options::recursive|FSYS::copy_options::copy_symlinks, ec);
	
	if (ec) return fs_err(says("dir_sync: '", sourcedir, "' to '", syncdir, "' - ", ec.message()));
	return true; //compare..?
}

bool dir_move(const std::string &sdir, const std::string &destdir)
{
	if (dir_copy(sdir, destdir)) return dir_delete(sdir);
	return false;
}

bool dir_backup(const std::string &sdir)
{
	if (dir_exist(sdir))
	{
		int i=1;
		std::string sd=sdir;
		while (dir_exist(sd)) { sd=says(sdir, ".~", i, '~'); i++; }
		return fscopy(sdir, sd, false);
	}
	return false;
}

size_t dir_size(const std::string sdir, bool)// bDeep) pedantic
{
	return path_size(sdir);
	
	//what u want here? - dir-entry-size or dir-content-size?, only first level or entire tree?
/*
	size_t used=0;
	if (!dir_exist(sdir)) { fs_err(says("dir_size: invalid directory '", sdir, "'")); return 0; }

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	//DirEntries de;
	struct stat st;
	used=(!stat(sdir.c_str(), &st))?st.st_size:0;
//	if (dir_read(sdir, de))
//	{
//		if (!de.empty())
//		{
//			std::string s;
//			auto it=de.begin();
//			while (it!=de.end())
//			{
//				s=path_append(sdir, it->first);
//				if (isdirtype(it->second)&&bDeep) used+=dir_size(s);
//				else used+=(!stat(s.c_str(), &st))?st.st_size:0;
//				it++;
//			}
//		}
//	}
#endif
	//windows dir-size always == 0
	return used;
*/
}

size_t dir_content_size(const std::string sdir)
{
	DirEntries de{};
	dir_read(sdir, de);
	return de.size();
}

//------------------------------------------------
bool file_exist(const std::string &sfile) //true iff existing reg-file
{
	FST t; //=getfsitemtype(sfile);
	return (fsexist(sfile, &t)&&isfiletype(t));
}

void file_name_ext(const std::string &sfpath, std::string &name, std::string &ext)
{
	name.clear();
	ext.clear();
	//if (isfile(sfpath))
	//{
		std::string s=path_name(sfpath);
		size_t p=s.find_last_of('.');
		if ((p!=std::string::npos)&&(p>0)) { name=s.substr(0, p); ext=s.substr(p+1); } else name=s;
	//}
}

std::string file_name_noext(const std::string &sfpath)
{
	std::string n, e;
	file_name_ext(sfpath, n, e);
	return n;
}

std::string file_extension(std::string sfpath)
{
	std::string n, e;
	file_name_ext(sfpath, n, e);
	return e;
}

bool file_delete(const std::string &sfile)
{
	return fsdelete(sfile);
/*
	clear_fs_err();
	if (!isfile(sfile)) return fs_err(says("file_delete: '", sfile, "' is not a file"));
	if (file_exist(sfile))
	{
		
	//xxx
		
		std::error_code ec;
		FSYS::remove(sfile, ec);
		if (ec) return fs_err(says("file_delete: '", sfile, "' - ", ec.message()));
	}
	return true;
*/
}

bool file_read(const std::string &sfile, std::string &sdata)
{
	clear_fs_err();
	if (!file_exist(sfile)) return fs_err(says("file_read: no such file: '", sfile, "'"));;
	sdata.clear();
	using itbuf = std::istreambuf_iterator<char>;
    std::ifstream inf(sfile);
    if (!inf.good()) return fs_err(says("file_read: cannot open file '", sfile, "'"));
    sdata = std::string(itbuf(inf.rdbuf()), itbuf());
    return true;
    
/*
	std::ifstream ifs(sfile.c_str());
	if (ifs.good())
	 {
	  std::stringstream ss; ss << ifs.rdbuf();
	   sdata=ss.str();
	    return true;
	     }
	return fs_err(says("file_read: cannot read file '", sfile, "'"));
*/

}

bool file_read_n(const std::string &sfile, size_t n, std::string &sdata)
{
	if (!file_exist(sfile)) return false;
	clear_fs_err();
	sdata.clear();
	std::ifstream ifs(sfile.c_str(), std::ios::binary);
	if (ifs.good())
	{
		char *buf= new char[n];
		ifs.read(buf, n);
		size_t mm, m=MAX((size_t)ifs.gcount(), n); // "negative values of std::streamsize are never used" - cppreference.com
		mm=m;
		while (m-->0) sdata+=buf[mm-m];
		delete [] buf;
		return true;
	}
	return fs_err(says("file_read: cannot read file '", sfile, "'"));
}

bool file_compare(const std::string &l, const std::string &r) //crude but ok
{
	if (file_exist(l)&&file_exist(r))
	{
		std::string sl, sr;
		if (file_read(l, sl)&&file_read(r, sr)) return seqs(sl, sr);
	}
	return false;
}

bool file_write(const std::string &sfile, const std::string &sdata, bool bappend)
{
	if (file_exist(sfile))
	{
		if (bappend) return file_append(sfile, sdata);
		else return file_overwrite(sfile, sdata);
	}
	clear_fs_err();
	std::ofstream ofs(sfile);
	if (ofs.good())
	{
		ofs << sdata;
		return true;
	}
	return fs_err(says("file_write: cannot write file '", sfile, "'"));
}

bool file_append(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile, std::ios_base::app);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(says("file_append: cannot write file '", sfile, "'"));
}

bool file_overwrite(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile, std::ios_base::out|std::ios_base::trunc);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(says("file_overwrite: cannot write file '", sfile, "'"));
}


bool file_rename(const std::string &sfile, const std::string &sname)
{
	return fsrename(sfile, sname);
/*
	clear_fs_err();
	if (!file_exist(sfile)) return fs_err(says("file_rename: '", sfile, "' does not exist"));
	if (!isvalidname(sname)) return fs_err(says("file_rename: invalid new name '", sname, "'"));
	std::string snd=path_append(path_path(sfile), sname);
	if (file_exist(snd)) return fs_err(says("file_rename: name '", sname, "' already exist"));
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	if (rename(sfile.c_str(), snd.c_str())!=0) return fs_err(says("file_rename: ", std::strerror(errno)));
#elif defined(_WIN64)
	std::error_code ec;
	FSYS::rename(sfile, snd, ec);
	if (ec) return fs_err(says("file_rename: '", sfile, "' to '", snd, "' - ", ec.message()));
#endif
	return (!file_exist(sfile)&&file_exist(snd));
*/
}



bool file_copy(const std::string &src, const std::string &dest, bool bBackup)
{
	std::string tgt{};
	if (isdir(dest)) { tgt=path_append(dest, path_name(src)); } else tgt=dest;
	return fscopy(src, tgt, bBackup);
}

bool file_move(const std::string &sfile, const std::string &dest, bool bBackup)
{
	if (file_copy(sfile, dest, bBackup)) return file_delete(sfile);
	return false;
}

bool file_backup(std::string sfile)
{
	if (file_exist(sfile))
	{
		int i=1;
		std::string sf=sfile;
		while (file_exist(sf)) { sf=says(sfile, ".~", i, '~'); i++; }
		return fscopy(sfile, sf);
	}
	return false;
}

/*
size_t file_size(const std::string &sfile)
{
	return path_size(sfile);
/...* ...USE FOR WINDOWS/NON-LINUX...
	size_t n=0;
	if (fsexist(sfile))
	{
		clear_fs_err();
		try { n=(size_t)FSYS::file_size(sfile); }
		catch(FSYS::filesystem_error &e) { n=0; fs_err(says("file_size: '", sfile, "' - ", e.what())); }
	}
	else { fs_err(says("file_size: '", sfile, "' - does not exist")); }
	return n;
*.../
}
*/
//const std::string file_extension(const std::string &sfile)
//{
//	size_t p=sfile.rfind('.');
//	std::string s("");
//	if (p!=std::string::npos) s=sfile.substr(p+1);
//	return s;
//}

bool file_crc32(const std::string &sfile, uint32_t &crc)
{
	crc=0;
	std::string sd;
	if (fsexist(sfile)&&file_read(sfile, sd))
	{
		const uint32_t P=0xedb88320;
		auto calc=[=](uint32_t c, const unsigned char *buf, size_t len)
		{
		    c=~c;
			while (len--) { c^=*buf++; for (int k=0; k<8; k++) c=((c&1)?((c>>1)^P):(c>>1)); }
			return ~c;
		};
		crc=calc(crc, (const unsigned char*)(sd.data()), sd.size());
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
bool findfile0(const DirTree &DT, std::string sdir, std::string sfile, std::string &sfound)
{
	if (DT.content.find(sfile)!=DT.content.end()) { sfound=path_append(sdir, sfile); return true; }
	for (auto p:DT) { if (findfile0(p.second, path_append(sdir, p.first), sfile, sfound)) return true; }
	return false;
}
bool findfile(std::string sdir, std::string sfile, std::string &sfound)
{
	DirTree DT;
	DT.Read(sdir);
	return findfile0(DT, sdir, sfile, sfound);
}

auto ffdnl=[](const std::string &h, const std::string &n, bool b)->bool{ if (b) return scontain(h, n); return sicontain(h, n); };
bool findfdnamelike0(const DirTree &DT, std::string sdir, std::string slike, VSTR &vfound, bool bsamecase)
{
	for (auto pft:DT.content) { if (ffdnl(pft.first, slike, bsamecase)) vfound.push_back(path_append(sdir, pft.first)); }
	for (auto p:DT)
	{
		std::string sd;
		sd=path_append(sdir, p.first);
		if (ffdnl(p.first, slike, bsamecase)) { vfound.push_back(sd); }
		if (findfdnamelike0(p.second, sd, slike, vfound, bsamecase)) return true; }
	return false;
}
bool findfdnamelike(std::string sdir, std::string slike, VSTR &vfound, bool bsamecase)
{
	DirTree DT;
	vfound.clear();
	if (ffdnl(sdir, slike, bsamecase)) { vfound.push_back(sdir); }
	DT.Read(sdir);
	findfdnamelike0(DT, sdir, slike, vfound, bsamecase);
	return (vfound.size()>0);
}

bool isencrypted(std::string sf)
{
	if (isfile(sf))
	{
		//todo .. check for sigs...
		
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
bool IsInteger(const std::string &s)
{
	std::string t=s;
	ReplaceChars(t,",.-+",""); //strip commas
	if (t.empty()) return false;
	bool b=true;
	for (size_t i=0; ((i<t.length())&&(b=IsDigit(t[i]))); i++);
	return b;
}

bool IsDecimal(const std::string &s) //assumes d , .d , d. , d.d , d,ddd.d
{
	std::string t=s,l,r;
	size_t p;
	ReplaceChars(t,",",""); //strip commas
	if (t.empty()) return false;
	size_t i=0, n=0;
	bool b=true;
	while (b&&(n<2)&&(i<t.length())) { if (t[i]=='.') n++; else b=IsDigit(t[i]); i++; }
	if (!b||(n>=2)) return false;
	if ((p=t.find('.'))!=std::string::npos)
	{
		l=t.substr(0,p); r=t.substr(p+1);
		return ((l.empty()&&IsInteger(r)) || (r.empty()&&IsInteger(l)) || (IsInteger(l)&&IsInteger(r)));
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
//explicit specialization to check if std::string contain a number
template<> bool is_numeric <std::string> (std::string s)
{
	return (IsInteger(s)||IsDecimal(s));
}

//--------------------------------------------------------------------------------------------------
//explicit specializations of ensv & desv for std::string
//template<> void ensv <std::string> (const std::vector<std::string> &v, char delim, std::string &list, bool bIncEmpty)
template<> void ensv <std::string> (const VSTR &v, char delim, std::string &list, bool bIncEmpty)
{
	std::ostringstream oss("");
	for (auto t:v) { if (!oss.str().empty()) oss << delim; oss << t; }
	list=oss.str();
}

//template<> size_t desv <std::string> (const std::string &list, char delim, std::vector<std::string> &v, bool bIncEmpty, bool bTrimWhitespace)
template<> size_t desv <std::string> (const std::string &list, char delim, VSTR &v, bool bIncEmpty, bool bTrimWhitespace)
{
	v.clear();
	if (!list.empty())
	{
		std::string s{};
		int i=0, n=list.size();
		while (i<n)
		{
			if (list[i]!=delim) { s+=list[i]; }
			else
			{
				if (bTrimWhitespace) TRIM(s);
				if (!s.empty()||bIncEmpty) { v.push_back(s); s.clear(); } s.clear();
			}
			i++;
		}
		//if (!s.empty()||(list[list.size()-1]==delim)) v.push_back(s);
		if (bTrimWhitespace) TRIM(s);
		if (!s.empty()||bIncEmpty) v.push_back(s);
	}
	return v.size();
}


//--------------------------------------------------------------------------------------------------
