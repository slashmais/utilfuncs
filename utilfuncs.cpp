/*
    This file is part of utilfuncs comprising utilfuncs.h and utilfuncs.cpp

    utilfuncs - Copyright 2018 Marius Albertyn

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


#include <utilfuncs/utilfuncs.h>
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

#if defined(PLATFORM_POSIX) || defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#include <cerrno>
#include <cstring>
#endif

////#include <filesystem> //c++17! (in gcc v7.1)
////namespace FSYS=std::filesystem;
//#include <experimental/filesystem> //NB: need to link with libstdc++fs
//namespace FSYS=std::experimental::filesystem;

//--------------------------------------------------------------------------------------------------

std::string s_error_report="";
const std::string get_error_report() { return s_error_report; }
void clear_error_report() { s_error_report.clear(); }
bool report_error(std::string serr, bool btell) { s_error_report=serr; if (btell) telluser("ERROR: ", serr); return false; }

//--------------------------------------------------------------------------------------------------

///todo 
/*
	change this to use X directly (or Wayland?!?)
	does X have "msgbox()"?
	
	if X is running ...
	
*/
#if defined(flagGUI)
#include <CtrlLib/CtrlLib.h>
using namespace Upp;
	void PRINTSTRING(const std::string &s)
	{
		std::string S{s};
		ReplaceChars(S, "\t", "    ");
		PromptOK(DeQtf(S.c_str()));
	}
	const std::string READSTRING(const std::string &sprompt)
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
	void waitenter(const std::string &msg) { PromptOK(DeQtf(msg.c_str())); }

	void MsgOK(std::string msg, std::string title)
	{
		Prompt(title.c_str(), Image(), msg.c_str(), "OK");
	}

#else
	void PRINTSTRING(const std::string &s) { std::cout << s << std::flush; }
	const std::string READSTRING(const std::string &sprompt)
	{
		std::string s{};
		std::cout << sprompt << ": ";
		std::cin >> s;
		return s;
	}
	void waitenter(const std::string &msg) { std::cout << msg << "\nPress Enter to continue..\n"; std::cin.get(); }
#endif

const std::string askuser(const std::string &sprompt)
{
	return READSTRING(sprompt);
}

#if defined(PLATFORM_POSIX) || defined(__linux__)
	const std::string username() { return getpwuid(getuid())->pw_name; }
	const std::string username(int uid)
	{
		struct passwd *p=getpwuid(uid);
		std::string s{"?"};
		if (p) s=p->pw_name;
		return s;
	}
	const std::string homedir() { return getpwuid(getuid())->pw_dir; }
	const std::string hostname() { char buf[1024]; /*int n=*/gethostname(buf, 1024); return std::string((char*)buf); }
#else
	const std::string username() { return "wtfru?"; }											//todo...
	const std::string username(int uid) { return "wtfisthis?"; }
	const std::string homedir() { return "wtfuhome?"; }
	const std::string hostname() { return "wtfthishost?"; }
#endif

#if defined(PLATFORM_POSIX) || defined(__linux__)
	int realuid() { return (int)getuid(); }
	int effectiveuid() { return (int)geteuid(); }
	bool has_root_access() { return seqs(username(effectiveuid()), "root"); }
#endif


// bash/linux...
const std::string bash_escape_name(const std::string &name)
{
	 //see: https://unix.stackexchange.com/questions/347332/what-characters-need-to-be-escaped-in-files-without-quotes
	std::string ename="", ec=" \t!\"'#$&()*,;<>=?[]\\^`{}|~";
	for (auto c:name)
	{
		if (ec.find(c)!=std::string::npos) ename+="\\";
		ename+=c;
	}
	if (ename[0]=='-') ename.insert(0, "./");
	return ename;
}

#if defined(PLATFORM_POSIX) || defined(__linux__)
#include <termios.h>
#include <fcntl.h>
	bool checkkeypress(int k)
	{
		struct termios keep, temp;
		int curflags;
		int K=0;
		tcgetattr(STDIN_FILENO, &keep);
		curflags=fcntl(STDIN_FILENO, F_GETFL, 0);
		temp=keep;
		temp.c_lflag&=~(ICANON|ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &temp);
//		curflags=fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, curflags|O_NONBLOCK);
		K=getchar();
		tcsetattr(STDIN_FILENO, TCSANOW, &keep);
		fcntl(STDIN_FILENO, F_SETFL, curflags);
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
	
#else
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
#define MAX_SHORT 65535L
DTStamp dt_stamp()
{
	static uint16_t prev=0;
	time_t t;
	struct tm tmcur;
	struct timeval tv;
	DTStamp dts=0;
//	uint8_t y,m,d,H,M,S;
	uint16_t ms;
	time(&t);
	tmcur = *localtime(&t);
	dts=tmcur.tm_year;
	dts<<=8; dts+=tmcur.tm_mon+1;
	dts<<=8; dts+=tmcur.tm_mday;
	dts<<=8; dts+=tmcur.tm_hour;
	dts<<=8; dts+=tmcur.tm_min;
	dts<<=8; dts+=tmcur.tm_sec;
	gettimeofday(&tv, NULL);
	ms=(uint16_t)(tv.tv_usec%MAX_SHORT); //difference in 4-digit microsecond-tail (if need can do % 65535L)
	if (ms==prev) { prev++; ms=prev; } prev=ms;
	dts<<=16; dts+=ms;
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
	dts=spf(zp(y), zp(m), zp(tmcur.tm_mday), zp(tmcur.tm_hour), zp(tmcur.tm_min), zp(tmcur.tm_sec));
	return dts;
}

const std::string h_dt_stamp()
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
	y=((tmcur.tm_year+1900)%100); //just century (0..99)
	m=tmcur.tm_mon+1;
	gettimeofday(&tv, NULL);
	ms=(uint8_t)((double(tv.tv_usec)/1000000.0)*255.0); //reduce to range 0..255 *** NB! (if call-freq > 255, will get dupes!)
	if (ms==prev) { prev=(prev<255)?(prev+1):0; ms=prev; } prev=ms; // *** try to prevent dupes
	hdts=spf(zp(y), zp(m), zp(tmcur.tm_mday), zp(tmcur.tm_hour), zp(tmcur.tm_min), zp(tmcur.tm_sec), tohex(ms));
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

const std::string ToDateStr(DTStamp dts)
{
	std::stringstream ss("");
	uint64_t n;
	n=(dts>>56); ss << (int)(1900+n); //y
	n=((dts>>48)&0xff); ss << "-" << (int)n; //m
	n=((dts>>40)&0xff); ss << "-" << (int)n; //d
	n=((dts>>32)&0xff); ss << "-" << (int)n; //H
	n=((dts>>24)&0xff); ss << ":" << (int)n; //M
	n=((dts>>16)&0xff); ss << ":" << (int)n; //S
	n=(dts&0xffff); ss  << "." << (int)n; //ms
	return ss.str();
}

uint64_t get_unique_index() { return dt_stamp(); }

std::string get_unique_name(const std::string sprefix, const std::string ssuffix) { return spf(sprefix, dt_stamp(), ssuffix); }

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
{ //replaces EACH occurrence of sPhrase with sNew
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
{ //replaces EACH occurrence of EACH single char in sCharList with sReplacement
    std::string::size_type pos = 0;
    while ((pos=s.find_first_of(sCharList, pos))!= std::string::npos)
    {
        s.replace(pos, 1, sReplacement);
        pos += sReplacement.length();
    }
}

const std::string SanitizeName(const std::string &sp)
{
	std::string s=sp;
	ReplaceChars(s, " \\+-=~`!@#$%^&*()[]{}:;\"|'<>,.?/", "_");
	TRIM(s, "_");
	std::string t;
	do { t=s; ReplacePhrase(s, "__", "_"); } while (t.compare(s)!=0);
	return s;
}

const std::string ucase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::toupper(static_cast<unsigned char>(c));
	return w;
}

const std::string ucase(const std::string &s) { return ucase(s.c_str()); }

void toucase(std::string& s) { s=ucase(s.c_str()); }

const std::string lcase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::tolower(static_cast<unsigned char>(c));
	return w;
}

const std::string lcase(const std::string &s) { return lcase(s.c_str()); }

void tolcase(std::string &s) { s=lcase(s.c_str()); }

int scmp(const std::string &s1, const std::string &s2) { return s1.compare(s2); }
int sicmp(const std::string &s1, const std::string &s2)
{
	std::string t1=s1, t2=s2;
	return ucase(t1.c_str()).compare(ucase(t2.c_str()));
}
bool seqs(const std::string &s1, const std::string &s2) { return (scmp(s1,s2)==0); }
bool sieqs(const std::string &s1, const std::string &s2) { return (sicmp(s1,s2)==0); }

bool scontain(const std::string &data, const std::string &fragment) { return (data.find(fragment)!=std::string::npos); }
bool sicontain(const std::string &data, const std::string &fragment) { return scontain(lcase(data), lcase(fragment)); }

bool hextoch(const std::string &shex, char &ch) //shex <- { "00", "01", ... , "fe", "ff" }
{
	if (shex.length()==2)
	{
		std::string sh=shex;
        tolcase(sh);
		int n=0;
		if		((sh[0]>='0')&&(sh[0]<='9')) n=(16*(sh[0]-'0'));
		else if ((sh[0]>='a')&&(sh[0]<='f')) n=(16*(sh[0]-'a'+10));
		else return false;
		if		((sh[1]>='0')&&(sh[1]<='9')) n+=(sh[1]-'0');
		else if ((sh[1]>='a')&&(sh[1]<='f')) n+=(sh[1]-'a'+10);
		else return false;
		ch=char(n);
	}
	return true;
}

void fshex(const std::string &sraw, std::string &shex, int rawlen)
{
	//output: hex-values + tab + {char|.}'s, e.g.: "Hello" ->  "48 65 6C 6C 6F    Hello"
	//										 e.g.: "Hel\tlo" ->  "48 65 6C 09 6C 6F    Hel.lo"
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
	//shex+="\t";
	shex+="    ";//simulate 4-space 'tab'
	unsigned char u;
	for (i=0; i<l; i++) { u=sraw[i]; if (((u>=32)&&(u<=126))||((u>159)&&(u<255))) shex+=c2s(u); else shex+='.'; }
}

const std::string ashex(const std::string &sraw)
{
	std::string s("");
	unsigned int i;
	for (i=0; i<sraw.length(); i++) { s+=tohex<unsigned char>(sraw[i], 2); if (i<(sraw.length()-1)) s+=" "; }
	return s;
}

const std::string asbin(unsigned char u)
{
	std::string s("");
	int i=7;
	while (i>=0) { s+=((u>>i)&1)?"1":"0"; i--; }
	return s;
}

const std::string esc_quotes(const std::string &s)
{
	std::stringstream ss;
	ss << std::quoted(s);
	return ss.str();
}

const std::string unesc_quotes(const std::string &s)
{
	std::stringstream ss(s);
	std::string r;
	ss >> std::quoted(r);
	return r;
}

const std::string schop(const std::string &s, int nmaxlen, bool bdots)
{
	std::string w;
	if (bdots) { nmaxlen-=2; if (nmaxlen<=2) { w=".."; return w; }}
	if (nmaxlen>=(int)s.length()) { w=s; return w; }
	w=s.substr(0,nmaxlen);
	if (bdots) w+="..";
	return w;
}

const std::string spad(const std::string &s, int nmaxlen, char c, bool bfront, int tabsize)
{
	std::string w=s;
	int nt=0;
	for (size_t i=0; i<w.size(); i++) { if (w[i]=='\t') nt++; }
	int n=nmaxlen-w.length()-(nt*tabsize)+nt;
	if (n>0) w.insert((bfront?0:w.length()), n, c);
	return w;
}

const std::string spadct(const std::string &s, int nmaxlen, char c, bool bfront, int tabcols, int startpos)
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

const std::string sfit(const std::string &s, int nmaxlen, int fitbit, char c)
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
						else if (bpad) return spad(s, nmaxlen, c, false);
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
						else if (bpad) return spad(s, nmaxlen, c);
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
							sf=s.substr(0,l); spad(sf, nmaxlen-ns, c); sf+=s.substr(s.size()-r);
							return sf;
						}
					} break;
	}
	return s;
}

const std::string slcut(std::string &S, size_t n)
{
	std::string sc{};
	if (!S.empty())
	{
		sc=S.substr(0,n);
		if (S.size()>n) S=S.substr(n); else S.clear();
	}
	return sc;
}

const std::string srcut(std::string &S, size_t n)
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

//--------------------------------------------------------------------------------------------------
//simple encryption
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

const std::string encs(const std::string &raw, const std::string &pw) { std::string enc; encs(raw, enc, pw); return enc; }
const std::string decs(const std::string &enc, const std::string &pw) { std::string raw; decs(enc, raw, pw); return raw; }


//--------------------------------------------------------------------------------------------------
size_t splitslist(const std::string &list, char delim, std::vector<std::string> &vs, bool bIncEmpty)
{
	std::stringstream ss(list);
	std::string s;
	//NB: do not clear vs (preset-values) - caller's task
	while (std::getline(ss, s, delim))
	{
		TRIM(s);
		if (!s.empty()||bIncEmpty) vs.push_back(s);
	}
	return vs.size();
}

size_t splitsslist(const std::string &list, const std::string &delim, std::vector<std::string> &vs, bool bIncEmpty)
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

//size_t splitqslist(const std::string &list, char delim, std::vector<std::string> &vs, bool bIncEmpty)
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

void splitslen(const std::string &src, size_t len, std::vector<std::string> &vs)
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

void splitslr(std::string s, std::string sdiv, std::string &l, std::string &r)
{
	size_t p;
	r.clear();
	if ((p=s.find(sdiv.c_str()))!=std::string::npos) { l=s.substr(0, p); TRIM(l); r=s.substr(p+sdiv.size()); TRIM(r); }
	else l=s;
}

//--------------------------------------------------------------------------------------------------
bool GetSystemEnvironment(SystemEnvironment &SE)
{
	SE.clear();
	
#if defined(PLATFORM_POSIX) || defined(__linux__)
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

//--------------------------------------------------------------------------------------------------
const std::string to_sKMGT(size_t n)
{
	std::string s("");
	int i=0,t;
	float f=n;
	while (f>1024.0) { i++; f/=1024; }
	t=(int)(f*100.0); f=((float)t/100.0);
	return spf(f, i["BKMGTPEZY"]);
}

//--------------------------------------------------------------------------------------------------
std::string FILESYS_ERROR;
inline void clear_fs_err() { FILESYS_ERROR.clear(); }
inline bool fs_err(const std::string &e) { FILESYS_ERROR=e; return false; }
const std::string filesys_error() { return FILESYS_ERROR; }

//------------------------------------------------
void DirTree::rec_read(const std::string &sdir, DirTree &dt)
{
	dt.clear();
	dt.dirpath=sdir;
	DirEntries de; de.clear();
	dir_read(sdir, de);
	if (!de.empty())
	{
		for (auto p:de)
		{
			if (isdirtype(p.second)) rec_read(path_append(sdir, p.first), dt[p.first]);
			else dt.content[p.first]=p.second;
		}
	}
}

DirTree::DirTree() : dirpath("") { clear(); }
DirTree::~DirTree() {}

bool DirTree::Read(const std::string &sdir)
{
	clear();
	if (!dir_exist(sdir)) return fs_err(spf("DirTree: directory '", sdir, "' does not exist"));
	rec_read(sdir, *this);
	return true;
}

//bool DirTree::Prune_Dir(const std::string &dirname)
//{
//	auto it=find(dirname);
//	if (it!=end()) { erase(it); return true; }
//	return false;
//}
//
//bool DirTree::Prune_File(const std::string &filename)
//{
//	auto it=content.find(filename);
//	if (it!=content.end()) { content.erase(it); return true; }
//	return false;
//}



const std::string path_append(const std::string &sPath, const std::string &sApp)
{
	if (sApp.empty()) return sPath;
	std::string sl=sPath, sr=sApp; //(sPath.empty()?"/":sPath)
	//sl=((sl[sl.length()-1]=='/')?"":"/");
	RTRIM(sl, "/");
	LTRIM(sr, "/");
	return (sl+"/"+sr);
	//sl+="/"+sr;
	//return sl;
}

bool path_realize(const std::string &spath)
{
	if (dir_exist(spath)) return true;
	clear_fs_err();
	std::error_code ec;
	if (FSYS::create_directories(spath, ec)) return true;
	else return fs_err(spf("path_realize: '", spath, "' - ", ec.message()));

//retain below code until sure above works as it should !
//	if (dir_exist(spath)) return true;
//	clear_fs_err();
//	std::vector<std::string> v;
//	if (desv(spath, '/', v, false)>0)
//	{
//		std::string p("/");
//		bool b=true;
//		auto it=v.begin();
//		while (b&&(it!=v.end())) { p=path_append(p, *it++); if (!dir_exist(p)) b=dir_create(p); }
//		if (!b) return fs_err(spf("path_realize: '", spath, "' - ", filesys_error()));
//	}
//	else return fs_err(spf("path_realize: '", spath, "' - invalid path"));
//	return true;

}
	
const std::string path_path(const std::string &spath)
{
	std::string r(spath);
	//RTRIM(r, "/");
	size_t p=r.rfind('/');
	if (p!=std::string::npos) r=r.substr(0, p);
	return r;
}

const std::string path_name(const std::string &spath)
{
	std::string r(spath);
	if (r.size()>1) RTRIM(r, "/");
	size_t p=r.rfind('/');
	if ((p!=std::string::npos)&&(p<r.size())) r=r.substr(p+1);
	return r;
}

#if defined(PLATFORM_POSIX) || defined(__linux__)
	std::string path_time(const std::string &spath)
	{
		std::string sdt{};
		struct stat st;
		if (stat(spath.c_str(), &st)==0)
		{
			auto tts=[](const time_t *t, std::string &sD)
				{
					struct tm *ptm;
					std::stringstream ss;
					ptm = localtime(t);
					ss << (ptm->tm_year+1900) //a #define or variable somewhere?
						<< (((ptm->tm_mon+1)<=9)?"0":"") << (ptm->tm_mon+1)
						<< ((ptm->tm_mday<=9)?"0":"") << ptm->tm_mday
						<< ((ptm->tm_hour<=9)?"0":"") << ptm->tm_hour
						<< ((ptm->tm_min<=9)?"0":"") << ptm->tm_min
						<< ((ptm->tm_sec<=9)?"0":"") << ptm->tm_sec;
					sD=ss.str(); ss.str(""); ss.flush();
				};
		
			//tts(&st.st_atime, sdt);
			tts(&st.st_mtime, sdt);
			//tts(&st.st_ctime, sdt);
		}
		return sdt; //formatted as yyyymmddHHMMSS
	}
	
	std::string path_time_h(const std::string &spath)
	{
		std::string sdt{};
		struct stat st;
		if (stat(spath.c_str(), &st)==0)
		{
			auto tts=[](const time_t *t, std::string &sD)
				{
					struct tm *ptm;
					std::stringstream ss;
					ptm = localtime(t);
					ss << (ptm->tm_year+1900) //a #define or variable somewhere?
						<< "-" << (((ptm->tm_mon+1)<=9)?"0":"") << (ptm->tm_mon+1)
						<< "-" << ((ptm->tm_mday<=9)?"0":"") << ptm->tm_mday
						<< " " << ((ptm->tm_hour<=9)?"0":"") << ptm->tm_hour
						<< ":" << ((ptm->tm_min<=9)?"0":"") << ptm->tm_min
						<< ":" << ((ptm->tm_sec<=9)?"0":"") << ptm->tm_sec;
					sD=ss.str(); ss.str(""); ss.flush();
				};
		
			//tts(&st.st_atime, sdt);
			tts(&st.st_mtime, sdt);
			//tts(&st.st_ctime, sdt);
		}
		return sdt; //formatted as "yyyy-mm-dd HH:MM:SS"
	}
#else //windows..
	std::string path_time(const std::string &spath)
	{
		std::string sdt("");
		std::error_code ec;
		if (FSYS::exists(spath, ec))
		{
			auto ftime = FSYS::last_write_time(spath);
			std::time_t cftime=decltype(ftime)::clock::to_time_t(ftime);
			std::tm *pdt=localtime(&cftime);
			sdt=spf((pdt->tm_year+1900),
					(pdt->tm_mon<9)?"0":"", (pdt->tm_mon+1),
					(pdt->tm_mday<10)?"0":"", (pdt->tm_mday),
					(pdt->tm_hour<10)?"0":"", (pdt->tm_hour),
					(pdt->tm_min<10)?"0":"", (pdt->tm_min),
					(pdt->tm_sec<10)?"0":"", (pdt->tm_sec));
		}
		return sdt; //formatted as yyyymmddHHMMSS
	}
	
	std::string path_time_h(const std::string &spath)
	{
		std::string sr, sdt=path_time(spath);
		auto dsd=[](const std::string &sdt)->const std::string
			{
				std::string s=sdt.substr(0,4); //y
				s+="-"; s+=sdt.substr(4,2); //m
				s+="-"; s+=sdt.substr(6,2); //d
				s+=" "; s+=sdt.substr(8,2); //H
				s+=":"; s+=sdt.substr(10,2); //M
				s+=":"; s+=sdt.substr(12,2); //S
				return s;
			};
		if (!sdt.empty()) sr=dsd(sdt); else sr="0000-00-00 00:00:00";
		return sr; //formatted as "yyyy-mm-dd HH:MM:SS"
	}
#endif

bool path_max_free(const std::string spath, size_t &max, size_t &free)
{
	max=free=0;
	std::error_code ec;
	if (FSYS::exists(spath, ec))
	{
		try { FSYS::space_info si=FSYS::space(spath); max=si.capacity; free=si.free; return true; }
		catch(FSYS::filesystem_error &e) { return fs_err(spf("path_space: '", spath, "' - ", e.what())); }
	}
	return fs_err(spf("path_space: invalid path '", spath, "'"));
}


//------------------------------------------------
std::string getcurpath()
{
	return (std::string)FSYS::current_path();
}

//int gettype(FSITEMTYPE tn)
//{
//	int t=(-1);
//	switch(tn)
//	{
//		case TN_DIR:		return (int)FSYS::file_type::directory;
//		case TN_REGFILE:	return (int)FSYS::file_type::regular;
//		case TN_PIPE:		return (int)FSYS::file_type::fifo;
//		case TN_LINK:		return (int)FSYS::file_type::symlink;
//		case TN_BLOCKDEV:	return (int)FSYS::file_type::block;
//		case TN_CHARDEV:	return (int)FSYS::file_type::character;
//        case TN_SOCKET:		return (int)FSYS::file_type::socket;
//	}
//	return t;
//}
//
//int getitemtype(FSYS::file_type ft)
//{
//	int t=(-1);
//	//if (
//	switch(ft)
//	{
//		case FSYS::file_type::directory:	return TN_DIR;
//		case FSYS::file_type::regular:		return TN_REGFILE;
//		case FSYS::file_type::fifo:			return TN_PIPE;
//		case FSYS::file_type::symlink:		return TN_LINK;
//		case FSYS::file_type::block:		return TN_BLOCKDEV;
//		case FSYS::file_type::character:	return TN_CHARDEV;
//		case FSYS::file_type::socket:		return TN_SOCKET;
//	}
//	return (-1);
//}

FSYS::file_type getfsfiletype(const std::string &se)
{
	FSYS::file_type ft=FSYS::file_type::unknown;
	if (fsexist(se)) ft=FSYS::symlink_status(FSYS::path(se)).type();
	return ft;
}

const std::string gettypename(FSYS::file_type ft, bool bshort)
{
	switch(ft)
	{
		case FSYS::file_type::directory:	return (bshort)?"dir":"directory";
		case FSYS::file_type::regular:		return (bshort)?"fil":"regular file";
		case FSYS::file_type::fifo:			return (bshort)?"ffo":"fifo pipe";
		case FSYS::file_type::symlink:		return (bshort)?"lnk":"symlink";
		case FSYS::file_type::block:		return (bshort)?"blk":"block device";
		case FSYS::file_type::character:	return (bshort)?"chr":"character device";
		case FSYS::file_type::socket:		return (bshort)?"sck":"socket";
		case FSYS::file_type::none: //fall-thru
		case FSYS::file_type::not_found: //fall-thru
		case FSYS::file_type::unknown:			return (bshort)?"unk":"unknown";
	}
	return (bshort)?"unk":"unknown";
}

//isdirtype() checks type directly, if symlink then getsymlinktarget() must be used if needed ...
bool isdirtype(FSYS::file_type ft) { return (ft==FSYS::file_type::directory); }
bool isfiletype(FSYS::file_type ft) { return (ft==FSYS::file_type::regular); }
bool issymlinktype(FSYS::file_type ft) { return (ft==FSYS::file_type::symlink); }

bool fsexist(const std::string &sfs)
{
//	clear_fs_err();
//	std::error_code ec;
//	if (FSYS::exists(sfs, ec)) return true;
//	return fs_err(spf("fsexist: '", sfs, "' - ", ec.message()));

	struct stat buffer;
	return (stat (sfs.c_str(), &buffer) == 0);
}

bool issubdir(const std::string &Sub, const std::string &Dir)
{
	//
	if (!isdir(Dir)) return false; //Sub may be a file or dir
	if (Dir.size()&&(Dir.size()<Sub.size())) return seqs(Sub.substr(0, Dir.size()), Dir);
	return false;
}

bool isdirempty(const std::string &D)
{
	if (!dir_exist(D)) return false; //??? non-exist should imply empty ???
	DirEntries md;
	if (dir_read(D, md)) return md.empty();
	return false;
}

size_t fssize(const std::string &sfs)
{
	size_t n=0;
	//if (dir_exist(sfs)) n=dir_size(sfs);
	//else n=file_size(sfs);
	
	if (!fsexist(sfs)) { fs_err(spf("fssize: invalid entity '", sfs, "'")); }
	else
	{
		struct stat st;
		n=(!stat(sfs.c_str(), &st))?st.st_size:0;
	}
	return n;
}

//bool getfulltree(const std::string &sdir, std::map<std::string, int> &tree)
//{
//	clear_fs_err();
//	tree.clear();
//	if (!dir_exist(sdir)) return fs_err(spf("dir_read: directory '", sdir, "' does not exist"));
//	try
//	{
//		auto DID=FSYS::recursive_directory_iterator(sdir);
//		auto it=begin(DID);
//		for (; it!=end(DID); ++it) tree[getrelativepathname(sdir, it->path())]=(int)(FSYS::symlink_status(it->path()).type());
//	}
//	catch(FSYS::filesystem_error e) { return fs_err(spf("dir_read: '", sdir, "' - ", e.what())); }
//	return true;
//
//
//
//
///*
// //NB recursive
//	std::map<std::string, int> md;
//	const std::string &sdir=path_append(srootdir, ssubdir);
//	bool b=true;
//	if ((b=dir_read(sdir, md)))
//	{
//		if (md.size()>0)
//		{
//			for (auto p:md)
//			{
//				std::string s=path_append(sdir, p.first);
//				tree[s]=p.second;
//				if (isdirtype(p.second)) b=getfulltree(s, tree);
//			}
//		}
//	}
//	return b;
//*/
//
//}

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

const std::string getrelativepathname(const std::string &sroot, const std::string &spath)
{
	size_t n=sroot.size();
	std::string srel("");
	if ((n<spath.size())&&(seqs(sroot, spath.substr(0,n)))) { srel=spath.substr(n+1); LTRIM(srel,"/"); }
	return srel;
}

const std::string getsymlinktarget(const std::string &se)
{
	std::string sl("");
	if (issymlinktype(getfsfiletype(se)))
	{
		char buf[4096];
		int i=0, n=readlink(se.c_str(), buf, 4096);
		for (;i<n;i++) sl+=buf[i];
		sl+='\0';

//this filesystem shite doesn't work...
//		std::error_code ec;
//		FSYS::path fsp;
//		fsp=FSYS::read_symlink(se, ec);
//		if (ec) //fs_err(spf("setpermissions: '", sN, "' - ", ec.message()));
//		{
//			fs_err(spf("getsymlinktarget: '", se, "' - ", ec.message()));
//			sl=ec.message();
//		}
//		else sl=(std::string)fsp;

		
//			try
//			{
//				FSYS::permissions(sN, (FSYS::perms)prms);
//				if (bDeep&&isdir(sN))
//				{
//					DirEntries c;
//					if (dir_read(sN, c)) { for (auto p:c) if (!(b=setpermissions(p.first, bDeep))) break; }
//				}
//				return b;
//			}
//			catch(FSYS::filesystem_error e) { return fs_err(spf("setpermissions: '", sN, "' - ", e.what())); }
//			catch(...) { return fs_err(spf("setpermissions: '", sN, "' - failed with some exception..")); }
//		}
//		return fs_err(spf("setpermissions: '", sN, "' - not found"));

	}
	return sl;
}

bool isscript(const std::string sF) //bash, perl, (checks #!)
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
	std::string d{};
	if (file_read(sF, d)&&!d.empty())
	{
		for (size_t i=0; i<std::min(d.size(),size_t(2000)); i++) { if (!d[i]) return true; }
	}
	return false;
}

//todo...following is..() are q&d's, fix proper
bool ispicture(std::string sf) //jpg/png/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" bmp jpg jpeg png xpm svg xps gif ").find(sx)!=std::string::npos);
}

bool issound(std::string sf)
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" mp3 ").find(sx)!=std::string::npos);
}

bool isvideo(std::string sf) //mpg/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" avi mkv mp4 mpg mpeg mov ").find(sx)!=std::string::npos);
}

bool isarchive(std::string sf) //zip/gz/bz2/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" z zip 7z gz bz2 ").find(sx)!=std::string::npos);
}

bool ispdf(std::string sf) //adobe.. //pdf
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" pdf ").find(sx)!=std::string::npos);
}

bool isdocument(std::string sf) //doc/odf/ods/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" doc xls odf ods ").find(sx)!=std::string::npos);
}

bool isdatabase(std::string sf) //sqlite/xml/...
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" sqlite sqlite3 xml ").find(sx)!=std::string::npos);
}

bool issourcecode(std::string sf) //c/cpp/h/...
{
	if (isscript(sf)) return true;
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" c h cpp hpp ").find(sx)!=std::string::npos);
}

bool iswebfile(std::string sf) //htm[l]/css/
{
	std::string sx=file_extension(sf);
	if (sx.empty()) return false;
	tolcase(sx);
	sx=spf(" ", sx, " ");
	return (std::string(" htm html css js ").find(sx)!=std::string::npos);
}


DTStamp get_dtmodified(const std::string sN) { return 0; } //todo...
//	std::string get_sfdtime(const std::string sfd)
//	{
//		std::string sdt{};
//		auto ft=FSYS::last_write_time(sN);
//		time_t tt = decltype(ft)::clock::to_time_t(ft);
//		struct tm tl=*localtime(&tt);
//		sdt=spf(tl.tm_year+1900, "\\", tl.tm_mon+1, "\\", tl.tm_mday, "~", tl.tm_hour, ":", tl.tm_min, ":", tl.tm_sec);
//		return sdt;
//	}
	


//#if defined(__linux__)
#if defined(PLATFORM_POSIX) || defined(__linux__)
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

	const std::string get_owner(const std::string sN)
	{
		std::string so("unknown");
		if (fsexist(sN))
		{
			struct stat st;
			stat(sN.c_str(), &st);  // Error check omitted
			struct passwd *pw=getpwuid(st.st_uid);
			if (pw) so=pw->pw_name;
		}
		return so;
	}

	bool isexe(const std::string sF) //check if sF is an elf-binary
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
	

#else
//assuming Windows...

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "accctrl.h"
#include "aclapi.h"
//#pragma comment(lib, "advapi32.lib")

	const std::string get_owner(const std::string &sf)
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

	bool isexe(const std::string sF)
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
	if (fsexist(sN)) return (int)FSYS::status(sN).permissions();
	fs_err(spf("getpermissions: '", sN, "' - not found"));
	return 0;
}

bool setpermissions(const std::string, int, bool)//(const std::string sN, int prms, bool bDeep) pedantic
{
	//todo...
	// must do this bottom-up
	// setting permission e.g.: read-only on parent will prevent
	// setting permissions on sub-dirs/files
	// .....
	return false;
/*
	if (fsexist(sN))
	{
		std::error_code ec;
		bool b=true;
		//FSYS::permissions(sN, perms, ec); - does not seem to be implemented yet
		//if (ec) return fs_err(spf("setpermissions: '", sN, "' - ", ec.message())); &fix for deep..
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
		catch(FSYS::filesystem_error e) { return fs_err(spf("setpermissions: '", sN, "' - ", e.what())); }
		catch(...) { return fs_err(spf("setpermissions: '", sN, "' - failed with some exception..")); }
	}
	return fs_err(spf("setpermissions: '", sN, "' - not found"));
*/
}

#if defined(PLATFORM_POSIX) || defined(__linux__)
//#include <unistd.h>
//#include <cerrno>
//#include <cstring>

	std::string getfullrights(const std::string sfd) //ogo
	{
		struct stat STAT;
		lstat(sfd.c_str(), &STAT);
		std::string sr{};
		sr=((STAT.st_mode&S_IRUSR)==S_IRUSR)?"r":"-";
		sr+=((STAT.st_mode&S_IWUSR)==S_IWUSR)?"w":"-";
		sr+=((STAT.st_mode&S_IXUSR)==S_IXUSR)?"x":"-";
		sr+=" ";
		sr+=((STAT.st_mode&S_IRGRP)==S_IRGRP)?"r":"-";
		sr+=((STAT.st_mode&S_IWGRP)==S_IWGRP)?"w":"-";
		sr+=((STAT.st_mode&S_IXGRP)==S_IXGRP)?"x":"-";
		sr+=" ";
		sr+=((STAT.st_mode&S_IROTH)==S_IROTH)?"r":"-";
		sr+=((STAT.st_mode&S_IWOTH)==S_IWOTH)?"w":"-";
		sr+=((STAT.st_mode&S_IXOTH)==S_IXOTH)?"x":"-";
		return sr;
	}

	int getrights(const std::string sfd) //o only
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
		if (euidaccess(sfd.c_str(), F_OK|R_OK)) return fs_err(spf("canread: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}
	
	bool canwrite(const std::string sfd)
	{
		if (euidaccess(sfd.c_str(), F_OK|W_OK)) return fs_err(spf("canwrite: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}
	
	bool canexecute(const std::string sfd)
	{
		if (euidaccess(sfd.c_str(), F_OK|X_OK)) return fs_err(spf("canexecute: '", sfd, "' - ", std::strerror(errno)));
		return true;
	}

#else

	int getrights(const std::string sfd)
	{
		//int n=getpermissions(sfd); //need r=4, w=2, x=1 (rw==6, rwx==7, r_x==5)
		//n=(((n>>6)&7)|((n>>3)&7)|(n&7));
		
																									//todo:...fix this shit...
		
		int n=0;
		if (canread(sfd)) n|=4;
		if (canwrite(sfd)) n|=2;
		if (canexecute(sfd)) n|=1;
		
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
		return fs_err(spf("canread: '", sfd, "' - not found"));
	}
	
	bool canwrite(const std::string sfd)
	{
		if (fsexist(sfd))
		{
			int p=getpermissions(sfd);
			return ((p&(int)(FSYS::perms::owner_write|FSYS::perms::group_write|FSYS::perms::others_write))!=(int)FSYS::perms::none);
		}
		return fs_err(spf("canwrite: '", sfd, "' - not found"));
	}
	
	bool canexecute(const std::string sfd)
	{
		if (fsexist(sfd))
		{
			int p=getpermissions(sfd);
			return ((p&(int)(FSYS::perms::owner_exec|FSYS::perms::group_exec|FSYS::perms::others_exec))!=(int)FSYS::perms::none);
		}
		return fs_err(spf("canexecute: '", sfd, "' - not found"));
	}

#endif

const std::string getrightsRWX(size_t r) //use retval from getrights()...
{
																								//todo...
	if (r) r=0;
	return "todo";
}

const std::string getrightsRWX(const std::string sN)
{
	std::string s("");
	s=(canread(sN))?"r":"-";
	s+=(canwrite(sN))?"w":"-";
	s+=(canexecute(sN))?"x":"-";
	return s;
}

//------------------------------------------------
bool dir_exist(const std::string &sdir)
{
	std::error_code ec;
	if (FSYS::exists(sdir, ec)) { if (FSYS::is_directory(sdir)) return true; return false; }
	return fs_err(spf("dir_exist: '", sdir, "' - ", ec.message()));
}

bool dir_create(const std::string &sdir)
{
	clear_fs_err();
	std::error_code ec;
	if (dir_exist(sdir)) return true;
	if (FSYS::create_directory(sdir, ec)) return true;
	return fs_err(spf("dir_create: '", sdir, "' - ", ec.message()));
}

bool dir_delete(const std::string &sdir)
{
	clear_fs_err();
	std::error_code ec;
	FSYS::remove_all(sdir, ec);
	if (ec) return fs_err(spf("dir_delete: '", sdir, "' - ", ec.message()));
	return !dir_exist(sdir);
	
}

bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly)
{
	clear_fs_err();
	if (!dir_exist(l)) return fs_err(spf("dir_compare: l-directory '", l, "' does not exist"));
	if (!dir_exist(r)) return fs_err(spf("dir_compare: r-directory '", r, "' does not exist"));
	
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
					if ((FSYS::file_type)it->second==FSYS::file_type::directory)
						{ b=dir_compare(path_append(l, it->first), path_append(r, it->first)); }
					else if (!bNamesOnly&&((FSYS::file_type)it->second==FSYS::file_type::regular))
						{ b=file_compare(path_append(l, it->first), path_append(r, it->first)); }
						//if !b then stuff into some diff's list... means not bail-out when b==false... - todo - see alt-proto in header
					it++;
				}
			}
		}
	}
	return b;
}

bool dir_read(const std::string &sdir, DirEntries &content)
{
	clear_fs_err();
	content.clear();
	if (!dir_exist(sdir)) return fs_err(spf("dir_read: directory '", sdir, "' does not exist"));
	try
	{
		auto DI=FSYS::directory_iterator(sdir);
		auto it=begin(DI);
		for (; it!=end(DI); ++it) { content[path_name(it->path())]=FSYS::symlink_status(it->path()).type(); }
	}
	catch(FSYS::filesystem_error &e) { return fs_err(spf("dir_read: '", sdir, "' - ", e.what())); }
	return true;
}

bool dir_read_deep(const std::string &sdir, DirTree &dtree) { return dtree.Read(sdir); }

bool dir_rename(const std::string &sdir, const std::string &sname)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_rename: directory '", sdir, "' does not exist"));
	std::string snd=path_append(path_path(sdir), sname);
	if (dir_exist(snd))  return fs_err(spf("dir_rename: name '", sname, "' already exist"));
	FSYS::rename(sdir, snd, ec);
	if (ec) return fs_err(spf("dir_rename: '", sdir, "' to '", snd, "' - ", ec.message()));
	return (!dir_exist(sdir)&&dir_exist(snd));
}

bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_copy: source directory '", sdir, "' does not exist"));
	if (!dir_exist(destdir)) return fs_err(spf("dir_copy: target directory '", destdir, "' does not exist"));
	std::string sd=path_append(destdir, path_name(sdir));
	if (bBackup&&dir_exist(sd)) { if (!dir_backup(sd)) return fs_err(spf("dir_copy: failed to backup '", sd, "'")); }
	FSYS::copy(sdir, sd, FSYS::copy_options::recursive|FSYS::copy_options::copy_symlinks, ec);
	if (ec) return fs_err(spf("dir_copy: '", sdir, "' to '", sd, "' - ", ec.message()));
	return dir_exist(sd);
}

bool dir_sync(const std::string &sourcedir, const std::string &syncdir)
{
	
	telluser("need to think 'dir_sync' through..."); return false; //.........................................todo...
	
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sourcedir)) return fs_err(spf("dir_sync: source directory '", sourcedir, "' does not exist"));
	if (!path_realize(syncdir)) return fs_err(spf("dir_sync: target directory '", syncdir, "' does not exist")); //created
	
	FSYS::copy(sourcedir, syncdir, FSYS::copy_options::recursive|FSYS::copy_options::copy_symlinks, ec);
	
	if (ec) return fs_err(spf("dir_sync: '", sourcedir, "' to '", syncdir, "' - ", ec.message()));
	return true; //compare..?
}

bool dir_move(const std::string &sdir, const std::string &destdir, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_move: source directory '", sdir, "' does not exist"));
	if (!dir_exist(destdir)) return fs_err(spf("dir_move: target directory '", destdir, "' does not exist"));
	std::string sd=path_append(destdir, path_name(sdir));
	//if (dir_exist(sd)) return fs_err(spf("dir_move: target '", sd, "' already exist"));
	if (bBackup&&dir_exist(sd)) { if (!dir_backup(sd)) return fs_err(spf("dir_move: failed to backup '", sd, "'")); }
	FSYS::rename(sdir, sd, ec);
	if (ec) return fs_err(spf("dir_move: '", sdir, "' to '", sd, "' - ", ec.message()));
	return (!dir_exist(sdir)&&dir_exist(sd));
}

bool dir_backup(const std::string &sdir)
{
	if (dir_exist(sdir))
	{
		int i=1;
		std::string sd=sdir;
		while (dir_exist(sd)) { sd=spf(sdir, ".~", i, '~'); i++; }
		return dir_copy(sdir, sd, false);
	}
	return false;
}

size_t dir_size(const std::string sdir, bool)// bDeep) pedantic
{
	size_t used=0;
	if (!dir_exist(sdir)) { fs_err(spf("dir_size: invalid directory '", sdir, "'")); }
	else
	{
		DirEntries de;
		struct stat st;
		used=(!stat(sdir.c_str(), &st))?st.st_size:0;
//		if (dir_read(sdir, de))
//		{
//			if (!de.empty())
//			{
//				std::string s;
//				auto it=de.begin();
//				while (it!=de.end())
//				{
//					s=path_append(sdir, it->first);
//					if (isdirtype(it->second)&&bDeep) used+=dir_size(s);
//					else used+=(!stat(s.c_str(), &st))?st.st_size:0;
//					it++;
//				}
//			}
//		}
	}
	return used;
}


//------------------------------------------------
bool file_exist(const std::string &sfile) //true iff existing reg-file
{
	clear_fs_err();
	std::error_code ec;
	if (FSYS::exists(sfile, ec))
	{
		return isfile(sfile);
	}
	if (ec) return fs_err(spf("file_exist: '", sfile, "' - ", ec.message()));
	return false;
}

//bool priv_is_file(const std::string &sfile) //internal use: true if reg-file or does not exist
bool isfile(const std::string &sfile) //internal use: true if reg-file
{
	clear_fs_err();
	std::error_code ec;
	if (FSYS::exists(sfile, ec))
	{
		return isfiletype(getfsfiletype(sfile));
	}
	if (ec) return fs_err(spf("isfile: '", sfile, "' - ", ec.message()));
	return false;
}

void file_name_ext(const std::string &sfpath, std::string &name, std::string &ext)
{
	name.clear();
	ext.clear();
	if (isfile(sfpath))
	{
		std::string s=path_name(sfpath);
		size_t p=s.find_last_of('.');
		if ((p!=std::string::npos)&&(p>0)) { name=s.substr(0, p); ext=s.substr(p+1); } else name=s;
	}
}

std::string file_name_noext(const std::string &sfpath)
{
	std::string n, e;
	file_name_ext(sfpath, n, e);
	return n;
}

std::string file_extension(const std::string &sfpath)
{
	std::string n, e;
	file_name_ext(sfpath, n, e);
	return e;
}

bool file_delete(const std::string &sfile)
{
	clear_fs_err();
	std::error_code ec;
	if (!isfile(sfile)) return fs_err(spf("file_delete: '", sfile, "' is not a file"));
	if (file_exist(sfile))
	{
		FSYS::remove(sfile, ec);
		if (ec) return fs_err(spf("file_delete: '", sfile, "' - ", ec.message()));
	}
	return true;
}

bool file_read(const std::string &sfile, std::string &sdata)
{
	clear_fs_err();
	sdata.clear();
	std::ifstream ifs(sfile.c_str());
	if (ifs.good()) { std::stringstream ss; ss << ifs.rdbuf(); sdata=ss.str(); return true; }
	return fs_err(spf("file_read: cannot read file '", sfile, "'"));
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

bool file_write(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(spf("file_write: cannot write file '", sfile, "'"));
}

bool file_append(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile, std::ios_base::app);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(spf("file_append: cannot write file '", sfile, "'"));
}

bool file_overwrite(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile, std::ios_base::out|std::ios_base::trunc);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(spf("file_overwrite: cannot write file '", sfile, "'"));
}

bool file_rename(const std::string &sfile, const std::string &sname)
{
	clear_fs_err();
	std::error_code ec;
	if (!file_exist(sfile)) return fs_err(spf("file_rename: old file '", sfile, "' does not exist"));
	std::string sd=path_append(path_path(sfile), sname);
	if (file_exist(sd)) return fs_err(spf("file_rename: name '", sname, "' already exist"));
	FSYS::rename(sfile, sd, ec);
	if (ec) return fs_err(spf("file_rename: '", sfile, "' to '", sd, "' - ", ec.message()));
	return (!file_exist(sfile)&&file_exist(sd));
}

bool file_copy(const std::string &src, const std::string &dest, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	std::string tgt{};
	if (!file_exist(src)) return fs_err(spf("file_copy: source file '", src, "' does not exist"));
	if (isdir(dest)) { tgt=path_append(dest, path_name(src)); } else tgt=dest;
	if (bBackup&&file_exist(tgt)) { if (!file_backup(tgt)) return fs_err(spf("file_copy: failed to backup '", tgt, "' - ", filesys_error())); }
	FSYS::copy_file(src, tgt, FSYS::copy_options::overwrite_existing, ec);
	if (ec) return fs_err(spf("file_copy: '", src, "' to '", tgt, "' - ", ec.message()));
	return file_exist(tgt);
}

bool file_move(const std::string &sfile, const std::string &dest, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	std::string sd=dest;
	if (!file_exist(sfile)) return fs_err(spf("file_move: source file '", sfile, "' does not exist"));
	//if (isdir(sd)) { std::string sd=path_append(sd, path_name(sfile)); } else return fs_err(spf("file_move: target directory '", sd, "' does not exist"));
	if (isdir(sd)) sd=path_append(sd, path_name(sfile));
	//if (file_exist(sd)) return fs_err(spf("file_move: file '", sd, "' already exist"));
	if (bBackup&&file_exist(sd)) { if (!file_backup(sd)) return fs_err(spf("file_move: failed to backup '", sd, "' - ", filesys_error())); }
	FSYS::rename(sfile, sd, ec);
	if (ec) return fs_err(spf("file_move: '", sfile, "' to '", sd, "' - ", ec.message()));
	return (!file_exist(sfile)&&file_exist(sd));
}

bool file_backup(std::string sfile)
{
	if (file_exist(sfile))
	{
		clear_fs_err();
		std::error_code ec;
		int i=1;
		std::string sf=sfile;
		while (file_exist(sf)) { sf=spf(sfile, ".~", i, '~'); i++; }
		FSYS::copy_file(sfile, sf, ec);
		if (ec) return fs_err(spf("file_backup: '", sfile, "' to '", sf, "' - ", ec.message()));
		return file_exist(sf);
	}
	return false;
}

size_t file_size(const std::string &sfile)
{
	size_t n=0;
	if (fsexist(sfile))
	{
		clear_fs_err();
		try { n=(size_t)FSYS::file_size(sfile); }
		catch(FSYS::filesystem_error &e) { n=0; fs_err(spf("file_size: '", sfile, "' - ", e.what())); }
	}
	else { fs_err(spf("file_size: '", sfile, "' - does not exist")); }
	return n;
}

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


bool isencrypted(std::string sf)
{
	if (isfile(sf))
	{
		//todo .. check for sigs...
		
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
