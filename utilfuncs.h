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

#ifndef _utilfuncs_h_
#define _utilfuncs_h_

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <mutex>
#include <sys/time.h>
#include <fstream>
#include <functional>
#include <limits>
#include <any>
#include <cstdlib> //for system()-calls
#include <random>
#include <cmath>

//#include <concepts> //in c++20

#include <filesystem> //c++17! (in gcc v7.1)
namespace FSYS=std::filesystem;
//#include <experimental/filesystem> //NB: need to link with libstdc++fs
//namespace FSYS=std::experimental::filesystem;

//--------------------------------------------------------------------------------------------------Generic Vector
enum { SORT_NONE, SORT_ASC, SORT_DESC, };

template<typename L, typename R> int scmp(const L &l, const R &r);
template<typename L, typename R> int sicmp(const L &l, const R &r);
template<typename L, typename R=L> bool seqs(const L &l, const R &r);
template<typename L, typename R=L> bool sieqs(const L &l, const R &r);

template<typename T, bool Unique=false> struct VType : public std::vector<T>
{
	//generic vec to supply Add(), Set(), Has(), Drop(), use variadics & optional sort, optional unique
	
	using VT=std::vector<T>;
	bool bUniq{Unique}; //true=>strip dupes on add
	int sorder{SORT_NONE};
	void clear()								{ VT::clear(); }
	bool empty() const							{ return VT::empty(); }
	virtual ~VType()							{ clear(); }
	VType()										{ clear(); }
	VType& setsortorder(int order)
		{
			if ((order>=0)&&(order<=SORT_DESC)&&(order!=sorder))
			{
				sorder=order;
				if (!empty()) *this=VT(*this);
			}
			return *this;
		}
	VType& setunique(bool b=true)
		{
			bool bb=(b&&!bUniq);
			bUniq=b;
			if (bb&&!empty()) *this=VT(*this);
			return *this;
		}
	void push_back(const T &t)
		{
			if (VT::empty()||(!bUniq&&(sorder==SORT_NONE))) { VT::push_back(t); return; }
			auto cmp=[this](const T &l, const T &r)->int
				{
					if (bUniq&&(l==r)) return (-1);
					if (sorder==SORT_ASC) return (l>r)?1:0;
					return (l<r)?1:0;
				};
			int p{0};
			auto it=VT::begin();
			while (it!=VT::end()) { if (!(p=cmp((*it), t))) it++; else break; }
			if (p>=0) VT::insert(it, t);
		}
	bool Has(const T &t) const						{ for (auto e:(*this)) { if (e==t) return true; } return false; }
	void Add(const T &t)							{ this->push_back(t); }
	void Add(const VType<T> &V)						{ for (auto e:V) Add(e); }
	VType& operator=(const std::vector<T> &V)		{ clear(); Add(V); return *this; }
	VType& operator=(const VType<T> &V)				{ clear(); bUniq=V.bUniq; sorder=V.sorder; Add(V); return *this; }
	template<typename...P> void Add(P...p)			{ VT v{p...}; for (auto pp:v) Add(pp); }
	template<typename...P> VType<T>& Set(P...p)		{ clear(); (Add(p...)); return *this;}
	template<typename...P> VType(P...p)				{ clear(); Set(p...); } //variadic ctor
	VType(const VType<T> &V)						{ *this=V; }
	bool Drop(const T &t) //only first found
		{
			auto it=VT::begin();
			while (it!=VT::end()) { if ((*it)==t) { VT::erase(it); return true; } it++; }
			return false;
		}
	void RemoveDupes(const T &t)
		{
			bool b{false};
			auto it=VT::begin();
			while (it!=VT::end()) { if ((*it)==t) { b=true; VT::erase(it++); } else it++; } //remove all
			if (b) Add(t); //restore 1 entry
		}
	//fixme: this should only apply if T can stream(has operator<<() [& >>()] defined)
	friend std::ostream& operator<<(std::ostream &os, const VType &v)
		{
			os << "[";
			if (v.size()) os << v[0];
			if (v.size()>1) { for (size_t i=1; i<(v.size()-1); i++) os << ", " << v[i]; }
			os << ", " << v[(v.size()-1)] << "]";
			return os;
		}
};

typedef VType<std::string> VSTR;
typedef VType<std::string, true> VUSTR; //unique entries (no dupes)


//--------------------------------------------------------------------------------------------------Generic Map
template<typename K, typename V=K> struct MType : std::map<K, V>
{
	// Add() new only & NOT replace; Set() new & replace; rest are just aliases
	using MT=std::map<K, V>;
	void clear()		{ MT::clear(); }
	bool empty() const	{ return MT::empty(); }
	virtual ~MType()	{ clear(); }
	MType()				{ clear(); }
	inline bool HasKey(const K &k) const		{ return (MT::find(k)!=MT::end()); }
	inline bool HasValue(const V &v) const		{ for (auto p:(*this)) { if (p.second==v) return true; } return false; }
	inline void Add(const K &k, const V &v)		{ MT::emplace(k, v); } //alias for emplace() - only if key does not exist
	inline void Drop(const K &k)				{ MT::erase(k); }
	inline const V Get(const K &k) const		{ V v{}; if (HasKey(k)) v=MT::at(k); return v; }
	inline void Set(const K &k, const V &v)		{ (*this)[k]=v; } //add/replace value
	//fixme: this should only apply if K & V can stream(has operator<<() [& >>()] defined)
	friend std::ostream& operator<<(std::ostream &os, const MType &m)
		{
			for (auto p:m) { os << p.first << "=" << p.second << "\n"; }
			return os;
		}
};

typedef MType<std::string> MSTR;
typedef MType<std::string, VSTR> MVSTR;

/*
struct MVSTR : std::map<std::string, VSTR >
{
	virtual ~MVSTR() { clear(); }
	MVSTR()  { clear(); }
	bool HasKey(const std::string &sk) const				{ return (find(sk)!=end()); }
	bool HasValue(const std::string &sv) const				{ for (auto p:(*this)) { if (p.second.Has(sv)) return true; } return false; }
	void Add(const std::string &sk, const std::string &sv)	{ VSTR &V=(*this)[sk]; V.Add(sv); }
	void Drop(const std::string &sk)						{ if (HasKey(sk)) { (*this)[sk].clear(); erase(sk); }}
	VSTR Get(const std::string &sk) const					{ VSTR V{}; if (HasKey(sk)) V=at(sk); return V; }
	void Set(const std::string &sk, const VSTR &V)			{ if (!sk.empty()) (*this)[sk]=V; } //add/replace value
};
*/


//--------------------------------------------------------------------------------------------------
struct NOCOPY { private: NOCOPY(const NOCOPY&)=delete; NOCOPY& operator=(const NOCOPY&)=delete; public: NOCOPY(){}};

//--------------------------------------------------------------------------------------------------system-error-logging-etc
std::string get_error_report();
void clear_error_report();
bool report_error(const std::string &serr, bool btell=true);//=false);
bool log_report_error(const std::string &serr, bool btell=true);//=false);

//--------------------------------------------------------------------------------------------------user-errors
std::string get_utilfuncs_error(); //only last error set is kept
void clear_utilfuncs_error();
bool set_utilfuncs_error(const std::string &serr, bool btell=false);

//--------------------------------------------------------------------------------------------------
void WRITESTRING(const std::string &s);
std::string READSTRING(const std::string &sprompt);

#if defined(flagGUI)
void MsgOK(std::string msg, std::string title="Message");
#endif

//--------------------------------------------------------------------------------------------------
std::string askuser(const std::string &sprompt);
bool askok(const std::string &smsg);
void waitenter(const std::string &msg="");
bool checkkeypress(int k);
bool askpass(std::string &pw); //cli-only

//--------------------------------------------------------------------------------------------------
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
bool validate_app_path(std::string sFP, std::string &sRet);
#endif

//--------------------------------------------------------------------------------------------------
std::string thisapp();
std::string username(); //current user
std::string username(int uid);
std::string homedir();
std::string hostname();

//--------------------------------------------------------------------------------------------------
//process (using /proc, etc)
pid_t find_pid(const std::string &spath);
bool kill_pid(pid_t pid);
void kill_self();
bool kill_app(const std::string &spath);

//--------------------------------------------------------------------------------------------------
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	int realuid();
	int effectiveuid();
	bool has_root_access();
#endif

//--------------------------------------------------------------------------------------------------
template<typename...P> void say(P...p) { std::string r{}; std::stringstream ss(""); (ss<<...<<p); r=ss.str(); WRITESTRING(r); }
template<typename...P> std::string says(P...p) { std::string r{}; std::stringstream ss(""); (ss<<...<<p); r=ss.str(); return r; }
template<typename...P> void sayss(std::string &s, P...p) { std::string r{}; std::stringstream ss(""); (ss<<...<<p); r=ss.str();  s+=r; } //APPENDS! to s!
template<typename...P> bool sayerr(P...p) { std::string r{}; std::stringstream ss(""); ss<<"error: "; (ss<<...<<p); r=ss.str(); WRITESTRING(r); return false; }
template<typename...P> bool sayfail(P...p) { std::string r{}; std::stringstream ss(""); ss<<"fail: "; (ss<<...<<p); r=ss.str(); WRITESTRING(r); return false; }

//--------------------------------------------------------------------------------------------------
//unpacks variadic into csv-string (found this somewhere, dunno how or why (or if) it works!?!?)
template<typename H, typename...T> std::string unpack2csv(H h, T...t)
{
	std::stringstream ss("");
	ss << h;
	(int[]){ 0, (ss<<","<<t, void(), 0)... };
	return ss.str();
}

//--------------------------------------------------------------------------------------------------
void write_to_log(const std::string &slogname, const std::string &sinfo); //create/append to ~/LOGFILES/slogname
template<typename...T> void logger(T...t) { std::ofstream ofs(homedir()+"/LOGGER.LOG", std::ios::app); (ofs<<...<<t)<<"\n"; ofs.close(); }
template<typename...T> void logger(const std::string &slogfile, T...t) { std::ofstream ofs(slogfile.c_str(), std::ios::app); (ofs<<...<<t)<<"\n"; ofs.close(); }

//===================================================================================================
//for debugging
inline int trace_sequence() { static int ts{0}; ++ts; return ts; }
//WhereAmI - WAI
#define WAI says("*", trace_sequence(), "* TRACE: [", __FILE__, " : ", long(__LINE__), " : ", __func__, "] ")
template<typename...P> void trace(P...p)
	{
		//call with: "trace( WAI, rest, of, info);" -- will give location via wai-data
		// - trace_to_log() - without the WRITESTRING() is better -
		std::string r{};
		std::stringstream ss("");
		(ss<<...<<p);
		r=ss.str();
		write_to_log("TRACE.log", r);
		WRITESTRING(r);
	}
template<typename...P> void trace_to_log(P...p) //VERY helpful debugging function
	{
		//*** call with: "trace_to_log(WAI, rest, of, info);" -- will give location via wai-data ***
		std::string r{};
		std::stringstream ss("");
		(ss<<...<<p);
		r=ss.str();
		write_to_log("TRACE.log", r);
	}
//===================================================================================================

//housekeeping
auto to_do=[](std::string s=""){ say("(To do) ", s); };
#define TODO(m) say("To Do: '", __func__, "', in ", __FILE__, " [", long(__LINE__), "] ", #m)

//#define DEBUG telluser

//--------------------------------------------------------------------------------------------------
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif
template<typename T=int> const T MAX() { return std::numeric_limits<T>::max(); }
template<typename T> T MAX(T t) { return t; }
template<typename T> T MAX(T h, T t) { return (h>t)?h:t; }
template<typename R, typename...T> R MAX(R h, R v, T...t) { R r=(h>v)?h:v; r=MAX(r, t...); return r; }
template<typename R, typename V, typename...T> auto MAX(R h, V v, T...t) { auto r=(h>v)?h:v; r=MAX(r, t...); return r; }
template<typename T=int> T MIN() { return std::numeric_limits<T>::min(); }
template<typename T> T MIN(T t) { return t; }
template<typename T> T MIN(T h, T t) { return (h<t)?h:t; }
template<typename R, typename...T> R MIN(R h, R v, T...t) { R r=(h<v)?h:v; r=MIN(r, t...); return r; }
template<typename R, typename V, typename...T> auto MIN(R h, V v, T...t) { auto r=(h<v)?h:v; r=MIN(r, t...); return r; }

#ifdef ABS
#undef ABS
#endif
template<typename N> N ABS(N n) { return ((n<0)?-n:n); }
//#define ABS(n) ((n<0)?-n:n)

//--------------------------------------------------------------------------------------------------
//double - floating-point funcs:
const double D_EPS=1.0e-11L;//15 //epsilon=1.0e-8L; //-11L; //-15L; ///epsilon-nearness
auto dbl_is_zero	= [](double d)->bool{ return (d<0.0)?(d>=-D_EPS):(d<=D_EPS); };
auto dbl_is_equal	= [](double l, double r)->bool{ return dbl_is_zero(l-r); };
auto dbl_ceil		= [](double d)->int{ double D=ABS(d); int I=int(D); if (((D-double(I))>0)&&(d>0)) I++; if (d<0) I*=(-1); return I; };
auto dbl_floor		= [](double d)->int{ double D=ABS(d); int I=int(D); if (((D-double(I))>0)&&(d<0)) I++; if (d<0) I*=(-1); return I; };
auto round_nearest	= [](double d)->double{ return double((dbl_is_zero(d))?0.0:(d>0.0)?int(d+0.5):int(d-0.5)); };
auto round_lesser	= [](double d)->double{ return double((dbl_is_zero(d))?0.0:(d>0.0)?int(d):int(d-0.5)); };
auto round_greater	= [](double d)->double{ return double((dbl_is_zero(d))?0.0:(d>0.0)?(((d-int(d))>0)?(int(d)+1):int(d)):int(d)); };

//--------------------------------------------------------------------------------------------------
const double PI=3.1415926535897; //180
const double PIx2=(PI*2.0); //360
const double PId2=(PI/2.0); //90
const double PId4=(PI/4.0); //45
const double PIx2d3=(PI*2.0/3.0); //120
const double PIx3d2=(PI*3.0/2.0); //=(PI+Pd2);  //270

inline double degtorad(double deg) { return (deg*PI/180); }
inline double radtodeg(double rad) { return (rad*180/PI); }

//--------------------------------------------------------------------------------------------------
std::string to_sKMGT(size_t n); //1024, Kilo/Mega/Giga/Tera/Peta/Exa/Zetta/Yotta

//--------------------------------------------------------------------------------------------------
__attribute__((always_inline)) inline void kips(int sec) { std::this_thread::sleep_for(std::chrono::seconds(sec)); } //1.0
__attribute__((always_inline)) inline void kipm(int milsec) { std::this_thread::sleep_for(std::chrono::milliseconds(milsec)); } //0.001 (x/1 000)
__attribute__((always_inline)) inline void kipu(int microsec) { std::this_thread::sleep_for(std::chrono::microseconds(microsec)); } //0.000 001 (x/1 000 000)
__attribute__((always_inline)) inline void kipn(int nanosec) { std::this_thread::sleep_for(std::chrono::nanoseconds(nanosec)); } //0.000 000 001 (x/1 000 000 000)

__attribute__((always_inline)) inline auto getclock() { return std::chrono::steady_clock::now(); }
//inline auto getelapsed(auto b, auto e) { return std::chrono::duration_cast<std::chrono::nanoseconds>(e-b).count(); }
template<typename B, typename E> __attribute__((always_inline)) inline auto getelapsed(B b, E e) { return std::chrono::duration_cast<std::chrono::nanoseconds>(e-b).count(); }
//USAGE: auto b=getclock(); some_func_to_be_timed(...); auto duration=getelapsed(b, getclock()); ..

//--------------------------------------------------------------------------------------------------
template<typename T> T randominrange(T least, T most)
{
	int M;
	int R;
	T t;
	M=(most<10000)?10000:10000000;
	std::uniform_int_distribution<int> uid(0, M);
	std::random_device rd;
	R=uid(rd);
	t=((T)R*(most-least)/(T)M);
	return (least+t);
}

//--------------------------------------------------------------------------------------------------
typedef uint64_t DTStamp;
DTStamp			make_dtstamp(struct tm *ptm, uint64_t usecs=0);
DTStamp			dt_stamp(); //now
std::string		ymdhms_stamp();
std::string		h_dt_stamp(); //human-readable: "0y0m0d0H0M0Sxx" (e.g.: "181123111051A3"=>2018 Nov 23 11:10:51 (xx/A3 = reduced-microsec-suffix)
DTStamp			ToDTStamp(const std::string &yyyymmddHHMMSS); //HHMMSS-optional
std::string		month_name(int m, bool bfull=true);

enum { DS_FULL=0, DS_SHORT, DS_COMPACT, DS_SQUASH };
std::string		ToDateStr(DTStamp dts, int ds=DS_SHORT, bool btime=true, bool bmsec=false);
/*
	DS_FULL			ccyy month dd[ HH:MM:SS[.u...]]
	DS_SHORT		ccyy mon dd[ HH:MM:SS[.u...]]
	DS_COMPACT		ccyy mm dd[ HH:MM:SS[.u...]]
	DS_SQUASH		yymmdd[HHMMSS[.u...]]
*/

inline bool is_leap_year(int y) { return (((y%4==0)&&(y%100!= 0))||(y%400==0)); }

//--------------------------------------------------------------------------------------------------
uint64_t get_unique_index();
std::string get_unique_name(const std::string sprefix="U", const std::string ssuffix="");
inline bool isvalidname(const std::string &sN) { return (sN.find('/')==std::string::npos); } //applies to bash
inline bool is_name_char(int c)	{ return (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')) || ((c>='a')&&(c<='z')) || (c=='_')); }

//--------------------------------------------------------------------------------------------------
// bash/linux...
std::string bash_escape_name(const std::string &name);
inline std::string bash_safe_name(const std::string &name) { return bash_escape_name(name); }

//--------------------------------------------------------------------------------------------------
template<typename TypePtr> size_t p2t(TypePtr p) { union { TypePtr u; size_t n; }; u=p; return n; } //??todo...risky...test where used...
template<typename TypePtr> TypePtr t2p(size_t t) { union { TypePtr u; size_t n; }; n=t; return u; }

//--------------------------------------------------------------------------------------------------string-related
///todo: whitespace chars - beep, para.., backspace, etc
#define WHITESPACE_CHARS (const char*)" \t\n\r"
void LTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void RTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void TRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void ReplacePhrase(std::string &sTarget, const std::string &sPhrase, const std::string &sReplacement); //each occ of phrase with rep
void ReplaceChars(std::string &sTarget, const std::string &sChars, const std::string &sReplacement); //each occ of each char from chars with rep
std::string SanitizeName(const std::string &sp); //replaces non-alphanums with underscores: "a, b\t c" -> "a_b_c"
std::string EscapeChars(const std::string &raw, const std::string &sChars); //insert '\' before each occurrence of each char in sChars and '\' itself
std::string UnescapeChars(const std::string &sesc); //remove '\' from escaped chars, retain escaped '\' itself

//--------------------------------------------------------------------------------------------------
std::string ucase(const char *sz); //ascii-only todo: also do unicode upper/lower cases
std::string lcase(const char *sz);
// ***** See also: uucase(), ulcase(), touucase(), toulcase() in uul.h/cpp *****
inline std::string ucase(const std::string &s) { return ucase(s.c_str()); }
inline void toucase(std::string& s) { s=ucase(s.c_str()); }
inline std::string lcase(const std::string &s) { return lcase(s.c_str()); }
inline void tolcase(std::string &s) { s=lcase(s.c_str()); }

//--------------------------------------------------------------------------------------------------
template<typename L, typename R> int scmp(const L &l, const R &r) { std::string ll(l), rr(r); return ll.compare(rr); }
template<typename L, typename R> int sicmp(const L &l, const R &r) { std::string ll(l), rr(r); return ucase(ll).compare(ucase(rr)); }
template<typename L, typename R=L> bool seqs(const L &l, const R &r) { return (scmp(l,r)==0); }
template<typename L, typename R=L> bool sieqs(const L &l, const R &r) { return (sicmp(l,r)==0); }

bool scontain(const std::string &data, const std::string &fragment); //case-sensitive
bool sicontain(const std::string &data, const std::string &fragment); //not case-sensitive
bool scontainany(const std::string &s, const std::string &charlist); //checks for each char in list

//--------------------------------------------------------------------------------------------------
inline bool is_digit(unsigned char c) { return ((c>='0')&&(c<='9')); }
inline bool is_letter(unsigned char c) { return (((c>='A')&&(c<='Z'))||((c>='a')&&(c<='z'))); }
inline bool is_ascii(unsigned char c) { return (c<=127); }

//--------------------------------------------------------------------------------------------------
//see also 'tohex'-template below
bool hextoch(const std::string &shex, char &ch); //expect shex <- { "00", "01", ... , "fe", "ff" }
int shextodec(const std::string &shex); //shex contain hexdigits (eg "1E23A" or "1e23a" or "0x1E23A" or "0x1e23a" or "0X1E23A" or "0X1e23a")
void fshex(const std::string &sraw, std::string &shex, int rawlen=0); //format rawlen(0=>all) chars to hex
std::string ashex(const std::string &sraw); //format raw chars to "HE XH EX HE ..."
std::string ashex(unsigned char u);
std::string asbin(unsigned char u); //0's& 1's...

//--------------------------------------------------------------------------------------------------
std::string en_quote(const std::string &s, char Q='"'); //use any char to 'quote-enclose' string
std::string de_quote(const std::string &s);

//--------------------------------------------------------------------------------------------------
std::string schop(const std::string &s, int nmaxlen, bool bdots=true); //appends ".." to string
std::string spad(const std::string &s, int nmaxlen, char c=' ', bool bfront=true, int tabsize=4);
//fixed-column Tab-positions..
std::string spadct(const std::string &s, int nmaxlen, char c=' ', bool bfront=true, int tabcols=4, int startpos=0);

template<typename T> std::string pads(T t, int nmaxlen, char c=' ', bool bfront=true, int tabsize=4) //todo ?tabsize?
{
	std::string s{};
	int nt=0;
	s=says(t);
	int n=nmaxlen-s.length();
	if (n>0) s.insert((bfront?0:s.length()), n, c);
	return s;
}

//set fixed-length string, cater for dots front, mid, back, padding front/back, centering ...
enum { FIT_NONE=0, FIT_L=1, FIT_M=2, FIT_R=4, FIT_DOT=8, FIT_PAD=32, };
std::string sfit(const std::string &s, int nmaxlen, int fitbit=FIT_NONE, char cpad=' ');

std::string slcut(std::string &S, size_t n); //resizes S, returns cut from front (cutting chunks off S until S is empty)
std::string srcut(std::string &S, size_t n); //resizes S, returns cut from back

bool slike(const std::string &sfront, const std::string &sall); //case-sensitive compare sfront with start of sall
bool silike(const std::string &sfront, const std::string &sall); //non-case-sensitive compare sfront with start of sall
/* todo---
bool sendlike(const std::string &send, const std::string &sall); //case-sensitive compare send with end of sall
bool siendlike(const std::string &send, const std::string &sall); //non-case-sensitive compare send with end of sall
bool scontainlike(const std::string &scon, const std::string &sall); //case-sensitive compare/check if scon appears within sall
bool sicontainlike(const std::string &scon, const std::string &sall); //non-case-sensitive compare/check if scon appears within sall
*/

//--------------------------------------------------------------------------------------------------
void RTFtostring(const std::string &srtf, std::string &stext); //...cannot remember why I would need this...

//--------------------------------------------------------------------------------------------------UTF8
//some crude utf8-related funcs...
typedef uint32_t unicodepoint;
const unicodepoint INVALID_UCP=0xfffd; //'�'
const std::string INVALID_UTF8="\ufffd";

unicodepoint	u8toU(std::string u8);
std::string		Utou8(unicodepoint U);
bool			isvalidu8(const std::string &s);
std::string		u8charat(const std::string &s, size_t &upos); //inc's upos to next utf8-char
size_t			u8charcount(const std::string &s); //u8 char can be 1 to 4 bytes in size
std::string		u8substr(const std::string &s, size_t pos, size_t len=0);
size_t			u8charpostobytepos(const std::string &s, size_t pos); //pos=char-pos, ret-val is byte-count to start of char at charpos
void			u8insert(std::string &su8, const std::string &sins, size_t pos); //inserts or appends

//--------------------------------------------------------------------------------------------------
enum //GReeK-chars
{
	GRK_ALPHA,		GRK_BETA,	GRK_GAMMA,	GRK_DELTA,	GRK_EPSILON,
	GRK_ZETA,		GRK_ETA,	GRK_THETA,	GRK_IOTA,	GRK_KAPPA,
	GRK_LAMBDA,		GRK_MU,		GRK_NU,		GRK_XI,		GRK_OMICRON,
	GRK_PI,			GRK_RHO,	GRK_STIGMA,	GRK_SIGMA,	GRK_TAU,
	GRK_UPSILON,	GRK_PHI,	GRK_CHI,	GRK_PSI,	GRK_OMEGA,
};

inline std::string greekletter(int g, bool bUcase=false)
{
	std::string s=(bUcase)?"ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡςΣΤΥΦΧΨΩ":"αβγδεζηθικλμνξοπρςστυφχψω";
	std::string l{}; //fix: set to default invalid char
	if ((g>=0)&&(g<25)) { size_t p; p=g; l=u8charat(s, p); }
	return l;
}

//--------------------------------------------------------------------------------------------------simple encryption
void encs(const std::string &raw, std::string &enc, const std::string &pw="");
void decs(const std::string &enc, std::string &raw, const std::string &pw="");
std::string encs(const std::string &raw, const std::string &pw="");
std::string decs(const std::string &enc, const std::string &pw="");

//--------------------------------------------------------------------------------------------------
size_t splitslist(const std::string &list, char delim, VSTR &vs, bool bIncEmpty=true);
size_t splitslist(const std::string &list, const std::string &delim, VSTR &vs, bool bIncEmpty=true);
//size_t splitqslist(const std::string &list, char delim, VSTR &vs, bool bIncEmpty); todo quoted strings
void splitslen(const std::string &src, size_t len, VSTR &vs); //copy len-sized substrings from s to vs
void splitslr(std::string s, char cdiv, std::string &l, std::string &r); //split s on first cdiv into l & r
void splitslr(std::string s, const std::string &sdiv, std::string &l, std::string &r); //split s on first sdiv into l & r

//--------------------------------------------------------------------------------------------------
bool encode_b64(const std::string &sfrom, std::string &result);
bool decode_b64(const std::string &sfrom, std::string &result);
inline std::string encode_b64(const std::string &sfrom) { std::string s; if (!encode_b64(sfrom, s)) s.clear(); return s; }
inline std::string decode_b64(const std::string &sfrom) { std::string s; if (!decode_b64(sfrom, s)) s.clear(); return s; }

//===================================================================================================filesystem-related
std::string filesys_error(); //applies to bool functions below

//--------------------------------------------------------------------------------------------------
typedef std::map<std::string, std::string> SystemEnvironment; //[key-name]=value
bool GetSystemEnvironment(SystemEnvironment &SE);

#define FST int
enum //FST - easier as generic int type
{
	FST_UNKNOWN = 0,
	FST_FILE = 1,
	FST_DIR = 2,
	FST_LINK = 4,
	FST_PIPE = 8,
	FST_SOCK = 16,
	FST_BDEV = 32,
	FST_CDEV = 64,
};

FST direnttypetoFST(unsigned char uc);
FST modetoFST(int stm);
FST FSYStoFST(FSYS::file_type ft);
std::string FSTName(FST fst, bool bshort=false);

//-------------------------------------------------------MIME
std::string GetMimeType(const std::string &se);
std::string GetMimeExtentions(const std::string &se);

//-------------------------------------------------------statx
struct FSInfo //File-System-Entity-..
{
	std::string path;
	uint32_t owner_id;
	uint32_t group_id;
	FST type;
	uint8_t rights_owner; //4|2|1: r=(rights_owner&4), ...
	uint8_t rights_group;
	uint8_t rights_other;
	uint64_t bytesize;
	DTStamp dtaccess; //last read from
	DTStamp dtcreate; //immutable creation date-time
	DTStamp dtmodify; //last written to
	DTStamp dtstatus; //e.g. rights changed
	std::string linktarget; //extras---
	std::string mimetype;
	//---

	void clear()
		{
			path.clear();
			owner_id=group_id=0;
			type=0;
			rights_owner=rights_group=rights_other=0;
			bytesize=0;
			dtaccess=dtcreate=dtmodify=dtstatus=0;
			linktarget.clear();
			mimetype.clear();
			//---
		}
	~FSInfo() {}
	FSInfo() { clear(); }
};


/*
	TODO: whereever file & path names are used and it is linux -> bash_safe_name(..)
*/



enum MimeFlag { MF_NONE=0, MF_MIME, MF_MIMEEXT, };
bool getstatx(FSInfo &FI, const std::string &sfe, MimeFlag mimeflag=MF_NONE);

//-------------------------------------------------------
typedef std::map<std::string, FST> DirEntries;
struct DirTree : public std::map<std::string, DirTree> //[RELATIVE!!-to-dirpath-name]=..whatever-it-contains..
{
	/*
		DTree[sub_path_name] = (sub-path_subdir_names-DTree)
		 - full_sub_path
		 content - ([file_name]=type)*
	*/
	std::string dirpath;
	DirEntries content; //files etc (not subdirs)
	void rec_read(const std::string &sdir, DirTree &dt); //recursive read dir; NB: . & .. discarded
	DirTree();
	virtual ~DirTree();
	bool Read(const std::string &sdir);
	size_t Size0(size_t &n, size_t &f);
	size_t Size(size_t *pnsubs=nullptr, size_t *pnfiles=nullptr);
};

size_t fssize(const std::string &sfs);

//path..
std::string path_append(const std::string &spath, const std::string &sapp); // (/a/b/c, d) => /a/b/c/d
bool path_realize(const std::string &sfullpath); //full PATH only!, a file-name will create dir with that name!
std::string path_path(const std::string &spath); //excludes last name (/a/b/c/d => /a/b/c)
std::string path_name(const std::string &spath); //only last name (/a/b/c/d => d)
std::string path_time(const std::string &spath); //content_change date-time: return format=>"yyyymmddHHMMSS"
std::string path_time_h(const std::string &spath); //human-readable content_change date-time: return format=>"yyyy-mm-dd HH:MM:SS"
inline size_t path_size(std::string spath) { return fssize(spath); }
bool path_max_free(const std::string spath, size_t &max, size_t &free); //space on device on which path exist

//utility..
std::string getcurpath(); //current working directory

FST getfsitemtype(const std::string &se);
inline std::string GetFSType(const std::string &se, bool bshort=false) { return FSTName(getfsitemtype(se), bshort); }

//isdirtype() checks type directly, if symlink then getsymlinktarget() must be used if needed ...
inline bool isdirtype(FST ft) { return (ft==FST_DIR); }
inline bool isfiletype(FST ft) { return (ft==FST_FILE); }
inline bool islinktype(FST ft) { return (ft==FST_LINK); } ///was: issymlinktype
//ispipetype..
//issockettype..
//isdevicetype..


bool fsexist(const std::string &sfs, FST *ptype=nullptr);
bool fsdelete(const std::string fd);
bool fsrename(const std::string &x, const std::string &n); //x=existing full path&name, n=new name only
bool fscopy(const std::string &src, const std::string &dest, bool bBackupFiles=false, bool bIgnoreSpecial=false);


bool issubdir(const std::string &Sub, const std::string &Dir); //is S a subdir of D; check when Dir is a link---todo
bool isdirempty(const std::string &D); //also false if D does not exist, or cannot be read
bool realizetree(const std::string &sdir, DirTree &tree); //creates dirs-only (?? cwd? or sdir must be abs?)
std::string getrelativepathname(const std::string &sroot, const std::string &spath); //empty if spath not subdir of sroot

#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
	std::string getlinktarget(const std::string &slnk);
#endif

//libmagic(3)...
bool get_mime_info(std::string sf, std::string &smime, std::string &scharset);

bool isexefile(const std::string sF); //check if sF is an executable (.EXE or elf-binary)
bool isscriptfile(const std::string sF); //bash, perl, (checks #!)
bool isnontextfile(const std::string sF); //checks if file contains a null-byte
inline bool isbinaryfile(std::string sf) { return isnontextfile(sf); }
inline bool is_text_file(std::string sf) { return !isbinaryfile(sf); }
bool istextfile(std::string sf);
bool ispicturefile(std::string sf); //jpg/png/...
bool isanimationfile(std::string sf); //gif/tif/...
bool issoundfile(std::string sf); //mp3/...
bool isvideofile(std::string sf); //mpg/...
bool isarchivefile(std::string sf); //zip/gz/bz2/...
bool isdocumentfile(std::string sf); //doc/odf/ods/...
bool isdatabasefile(std::string sf); //sqlite/xml/...
bool issourcecodefile(std::string sf); //c/cpp/h/...
bool istxtfile(std::string sf);
bool iswebfile(std::string sf); //htm[l]/css/
bool ishtmlfile(std::string sf);
bool isxmlfile(std::string sf);
bool isodtfile(std::string sf); //ODF-type
bool isdocfile(std::string sf); //ms-type
bool isdocxfile(std::string sf); //ms-type
bool isrtffile(std::string sf);
bool ispdffile(std::string sf); //pdf


DTStamp get_dtmodified(const std::string sf);
std::string get_owner(const std::string sf);

//permissions & rights..
int getpermissions(const std::string sN); //format:777; see getstatx
bool setpermissions(const std::string sN, int prms, bool bDeep=false); //deep=>recursively if sN is a directory
//set_uid
//set_gid
//set_sticky

std::string getfullrights(const std::string sfd); //ogo
int getrights(const std::string sN); //for current user on object
bool canread(const std::string sN);
bool canwrite(const std::string sN);
bool canexecute(const std::string sN);

std::string getrightsRWX(size_t r);
std::string getrightsRWX(const std::string sN);

//dir..
bool dir_exist(const std::string &sdir);
inline bool isdir(const std::string &sdir) { return dir_exist(sdir); }
inline std::string dir_path(const std::string &sdpath) { return path_path(sdpath); }
inline std::string dir_name(const std::string &sdpath) { return path_name(sdpath); }
bool dir_create(const std::string &sdir);
bool dir_delete(const std::string &sdir);

bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly=false); //deep; bNamesOnly=>no file-sizes; true=>same

//bool dir_diff(const std::string &l, const std::string &r, bool bNamesOnly=false, std::map<std::string, int> *pdiffs); ...todo
	// pdiffs==[relative-path-to-entity]={0==access_error, 1==content_diff, 2==meta_diff, 3==not_exist_in_l, 4==not_exist_in_r}
		// enum COMPARE_STATUS { CS_ACCESS=0, CS_DIFF=1, CS_META, CS_NOTL, CS_NOTR, };
		// so that: l+relative-path-to-entity <-compared to-> r+relative-path-to-entity
	// root paths of l and r not compared, only sub-entities
	

bool dir_read(const std::string &sdir, DirEntries &content, bool bFullpath=false); //ALL local entries only, [name(not path)]=type
bool dir_rename(const std::string &sdir, const std::string &sname);
bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackupFiles=true);
bool dir_sync(const std::string &sourcedir, const std::string &syncdir); //copy only new & changed
bool dir_move(const std::string &sdir, const std::string &destdir);
bool dir_backup(const std::string &sdir); //makes ".~nn~" copy of dir

size_t dir_size(const std::string sdir, bool bDeep=false); //deep-size (use with to_sKMGT(..) to display e.g.: "nn.nn MB")
size_t dir_content_size(const std::string sdir); //count of files & sub-dirs

//bool dir_sync(const std::string &sdir, const std::string &destrootdir); //sdir treated as subdir of destrootdir ...todo:
	// destrootdir updated with content of sdir after sync

//bool dir_mirror(const std::string &l, const std::string &r); // ...todo:
	// both l and r will be mutually updated to be exact same after call
	// equivalent to: dir_sync(l, r); dir_sync(r, l);


//file..
bool file_exist(const std::string &sfile);
inline bool isfile(const std::string &sfile) { return file_exist(sfile); }
inline std::string file_path(const std::string &sfpath) { return path_path(sfpath); } //("/a/b/c.d") -> "/a/b"
inline std::string file_name(const std::string &sfpath) { return path_name(sfpath); } //("/a/b/c.d") -> "c.d"
void file_name_ext(const std::string &sfpath, std::string &name, std::string &ext);
std::string file_name_noext(const std::string &sfpath); //(/a/b/c.d) -> c
std::string file_extension(std::string sfpath); //(/a/b/c.d) -> d
bool file_delete(const std::string &sfile);
bool file_read(const std::string &sfile, std::string &sdata);
bool file_read_n(const std::string &sfile, size_t n, std::string &sdata); //check sdata.size() for actual size read
bool file_compare(const std::string &l, const std::string &r); //true=>same
bool file_write(const std::string &sfile, const std::string &sdata, bool bappend=false);
bool file_append(const std::string &sfile, const std::string &sdata);
bool file_overwrite(const std::string &sfile, const std::string &sdata);
bool file_rename(const std::string &sfile, const std::string &sname);
bool file_copy(const std::string &sfile, const std::string &dest, bool bBackup=true);
bool file_move(const std::string &sfile, const std::string &dest, bool bBackup=true);
bool file_backup(std::string sfile); //makes ".~nn~" copy of file
inline size_t file_size(const std::string &sfile) { return fssize(sfile); }
bool file_crc32(const std::string &sfile, uint32_t &crc);

bool findfile(std::string sdir, std::string sfile, std::string &sfound);

bool findfdnamelike(std::string sdir, std::string slike, VSTR &vfound, bool bsamecase=true);

bool isencrypted(std::string sf);


//--------------------------------------------------------------------------------------------------
//explicitly specialized to check if a string-content is numeric
template<typename T> bool is_numeric(T t)
{
	return std::is_arithmetic_v<T>;
}

template<typename N, typename I=int> N cmod(N v, N b) //clock-modulus
{
	N r=0, d, av=((v<0)?(-v):v), ab=((b<0)?(-b):b);
	if ((av>0)&&(ab>0))
	{
		d=(av-(ab*I(av/ab)));
		r=(v>0)?((b>0)?(d):(d-ab)):((b>0)?(ab-d):(-d));
		r=(r==b)?0:r;
	}
	return r;
}

template<typename N> N fitrange(N &v, N MIN, N MAX) { if (v<MIN) v=MIN; else if (v>MAX) v=MAX; return v; }


//NB: use absolute value of N-param...
//template<typename N> std::string tohex(N NB_UseAbsoluteValue, unsigned int leftpadtosize=1, char pad='0')
//...concepts...
template<typename N> std::string tohex(N n, unsigned int leftpadtosize=1, char pad='0')
{
	if (!is_numeric(n)) return "NaN";
	std::string H="";
	size_t U=size_t(n);
	while (U) { H.insert(0, 1, "0123456789abcdef"[(U&0xf)]); U>>=4; }
	while (H.size()<leftpadtosize) H.insert(0, 1, pad);
	return H;
}

template<typename T> std::string ttos(const T &v) //TypeTOString
{
	//if streamable(T)
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template<typename T> const T stot(const std::string &str) //StringTOType
{
	//if streamable(T)
    std::istringstream iss(str); //NB: FAILS with T is std::string
    T ret;
    iss >> ret;
    return ret;
}

//explicitly specialized for T==std::string
template<typename T> void ensv(const std::vector<T> &v, char delim, std::string &list, bool bIncEmpty=true) //EN-[delimiter-]Separated-Values
{ //					en-string [from] vector
	list.clear();
	std::ostringstream oss("");
	for (auto t:v) { if (!oss.str().empty()) oss << delim; oss << t; }
	list=oss.str();
}

//explicitly specialized for T==std::string
template<typename T> size_t desv(const std::string &list, char delim, std::vector<T> &v, bool bIncEmpty=true, bool bTrimWhitespace=false) //DE-[delimiter-]Separated-Values
{ //					de-string [to] vector
	std::istringstream iss(list);
	std::string s;
	v.clear();
	s.clear();
	auto stt=[](const std::string &str)->const T{ std::istringstream ss(str); T t; ss >> t; return t; };
	int i=0, n=list.size();
	while (i<n)
	{
		if (list[i]!=delim) { s+=list[i]; }
		else
		{
			if (bTrimWhitespace) TRIM(s);
			if (!s.empty()||bIncEmpty) { v.push_back(stt(s)); } s.clear();
		}
		i++;
	}
	if (bTrimWhitespace) TRIM(s);
	if (!s.empty()||bIncEmpty) v.push_back(stt(s));
	return v.size();
}

#define VTOS ensv<std::string>
#define STOV desv<std::string>


template<typename T> struct Stack : private std::vector<T> //LIFO=>push & pop; FIFO=>pushbot & pop()
{
	typedef std::vector<T> VT;
	typedef typename VT::iterator iterator;
	typedef typename VT::const_iterator const_iterator;
	typedef typename VT::reverse_iterator reverse_iterator;
	typedef typename VT::const_reverse_iterator const_reverse_iterator;
	
	void clear() { VT::clear(); } //if (!VT::empty())
	bool empty() { return VT::empty(); }
	size_t size() { return VT::size(); }
	
	Stack() { clear(); }
	virtual ~Stack() { clear(); }

	//top-of-stack...
	void push(const T &t) { VT::insert(VT::begin(), t); }
	T pop() { T t; if (!empty()) { t=VT::front(); VT::erase(VT::begin()); } return t; }
	T peek() { T t; if (!empty()) t=VT::front(); return t; }

	//bottom-of-stack...
	void pushbot(const T &t) { VT::push_back(t); }
	T popbot() { T t; if (!empty()) { t=VT::back(); VT::pop_back(); } return t; }
	T peekbot() { T t; if (!empty()) t=VT::back(); return t; }

	//DIY...
	iterator begin() { return VT::begin(); } //top-of-stack
	const_iterator begin() const { return VT::begin(); } //top-of-stack
	iterator end() { return VT::end(); }
	const_iterator end() const { return VT::end(); }
	reverse_iterator rbegin() { return VT::rbegin(); } //bottom-of-stack
	const_reverse_iterator rbegin() const { return VT::rbegin(); } //bottom-of-stack
	reverse_iterator rend() { return VT::rend(); }
	const_reverse_iterator rend() const { return VT::rend(); }

	//iterator based access...
	iterator find(const T &t) { size_t n=VT::size(); while (n>0) { if (VT::at(n-1)==t) return (begin()+(n-1)); n--; } return end(); }
	void push(iterator pos, const T &t) { if ((pos>=begin())&&(pos<end())) VT::insert(pos, t); else if (pos<begin()) push(t); else if (pos>=end()) pushbot(t); }
	void insert(iterator pos, const T &t) { push(pos, t); } //for consistency
	T pop(iterator pos) { T t=T(); if ((pos>=begin())&&(pos<end())) { t=(*pos); VT::erase(pos); } return t; }
	T peek(iterator pos) { T t=T(); if ((pos>=begin())&&(pos<end())) t=(*pos); return t; }
	void erase(iterator pos) { pop(pos); } //for consistency
	void replace(iterator pos, const T &t) { if ((pos>=begin())&&(pos<end())) (*pos)=t; }

	//index-based access...
	bool find(const T &t, size_t &idx) { if (size()>0) { idx=0; while (idx<size()) { if (VT::at(idx)==t) return true; idx++; }} return false; }
	void push(size_t pos, const T &t) { push(begin()+pos, t); }
	bool pop(size_t pos, T &t) { if (pos<size()) { t=VT::at(pos); erase(begin()+pos); return true; } return false; }
	bool peek(size_t pos, T &t) { if (pos<size()) { t=VT::at(pos); return true; } return false; }
	
	T& operator[](size_t pos) { return VT::at(pos); } //throws out-of-range
	const T& operator[](size_t pos) const { return VT::at(pos); } //throws out-of-range
	
};

typedef Stack<std::string> SStack;

//===================================================================================================
template<typename T> struct FilterFIFO
{
	typedef std::function<bool(T &t)> FilterFunction;
	
	std::mutex Q_LOCK;
	const size_t TO_PUT=43; //TimeOut's
	const size_t TO_GET=97;
	//const size_t TO_PEEK=350;
	
	std::vector<T> Q{};
	
	void clear() { Q.clear(); }
	bool empty() { return Q.empty(); }
	
	virtual ~FilterFIFO() {} // clear(); }
	FilterFIFO() { clear(); }
	FilterFIFO(const FilterFIFO &F) { *this=F; }
	
	FilterFIFO& operator=(const FilterFIFO &F) { Q=F.Q; return *this; }

	bool put(const T &t) { while (!Q_LOCK.try_lock()) kipu(TO_PUT); Q.push_back(t); Q_LOCK.unlock(); return true; }
//	bool put(const T &t) { Q.push_back(t); return true; }
	
	bool get(T &t, FilterFunction ff=nullptr)
	{
		while (!Q_LOCK.try_lock()) kipu(TO_GET);
		bool b=false;
		if (!Q.empty())
		{
			if (ff)
			{
				auto it=Q.begin();
				while (!b&&(it!=Q.end())) { if ((b=ff((*it)))) { t=(*it); Q.erase(it); break; } else it++; }
			}
			else { t=*(Q.begin()); Q.erase(Q.begin()); b=true; }
		}
		Q_LOCK.unlock();
		return b;
	}
	
};


//===================================================================================================tokenizer
struct TokPos
{
	int x,y; //w(idth in bytes) at line y column x
	void clear() { x=y=0; }
	TokPos() : x{}, y{} {}
	TokPos(int X,int Y) : x{X}, y{Y} {}
	TokPos(const TokPos &tp) { *this=tp; }
	TokPos& operator=(const TokPos &tp) { x=tp.x; y=tp.y; return *this; }
	friend std::ostream& operator<<(std::ostream &os, const TokPos &tp) { os << "(" << tp.x << "," << tp.y << ")"; return os; }
};

enum //token-type
{
	//ALL tokens are strings
	TT_NOISE=0,	//whitespace
	TT_SCHAR=1,	//single character (includes multi-byte chars)
	TT_TEXT=2,	//string
	TT_QSTR=4,	//quoted string
	TT_MAX=4,
};

enum //CHARACTER_TYPE
{
	CT_NULL=0,
	CT_SPACE,		// (space) 32
	CT_TAB,			// (tab) 9
	CT_LF,			// (line-feed) 10
	CT_CR,			// (carriage-return) 13 - should be xlated to 10
	CT_COMMA,		// ,
	CT_PERIOD,		// .
	CT_COLON,		// :
	CT_SEMICOLON,	// ;
	CT_DASH,		// -
	CT_BAR,			// |
	CT_FSLASH,		// /
	CT_BSLASH,		// (backslash) 92
	CT_USCORE,		// _ (underscore)
	CT_SQUOTE,		// ' (single-quote) 39
	CT_DQUOTE,		// " (double-quote) 34
	CT_BQUOTE,		// ` (grave-quote) 96
	CT_GQUOTE=CT_BQUOTE,
	CT_OCURVE,		// (
	CT_CCURVE,		// )
	CT_OSTAPLE,		// [
	CT_CSTAPLE,		// ]
	CT_OBRACE,		// {
	CT_CBRACE,		// }
	CT_OANGLE,		// <
	CT_CANGLE,		// >
	CT_DIGIT,		// 0..9
	CT_ALPHA,		// A..Z a..z
	CT_OPERATOR,	// + - * /
	
};

inline int ct_type(int c)
{
	if ((c>='0')&&(c<='9')) return CT_DIGIT;
	if (((c>='A')&&(c<='Z'))||((c>='a')&&(c<='z'))) return CT_ALPHA;
	if ((c=='-')||(c=='*')||(c=='+')||(c=='/')) return CT_OPERATOR;
	switch(c)
	{
		case 32: return CT_SPACE;
		case 9: return CT_TAB;			// (tab) 9
		case 10: return CT_LF;			// (line-feed) 10
		case 13: return CT_CR;			// (carriage-return) 13
		case ',': return CT_COMMA;		// ,
		case '.': return CT_PERIOD;		// .
		case ':': return CT_COLON;		// :
		case ';': return CT_SEMICOLON;	// ;
		case '-': return CT_DASH;		// -
		case '|': return CT_BAR;		// |
		case '/': return CT_FSLASH;		// /
		case 92: return CT_BSLASH;		// (backslash) 92
		case '_': return CT_USCORE;		// _
		case 0x2018: case 0x2019: case 0x201a: case 0x201b: //‘’‚‛ unicode chars
		case 39: return CT_SQUOTE;		// ' (single-quote) 39
		case 0x201c: case 0x201d: case 0x201e: case 0x201f: //“”„‟ unicode chars
		case '"': return CT_DQUOTE;		// "
		case 96: return CT_BQUOTE;		// ` (grave-quote/back-quote/back-tick) 96
		case '(': return CT_OCURVE;		// (
		case ')': return CT_CCURVE;		// )
		case '[': return CT_OSTAPLE;	// [
		case ']': return CT_CSTAPLE;	// ]
		case '{': return CT_OBRACE;		// {
		case '}': return CT_CBRACE;		// }
		case '<': return CT_OANGLE;		// <
		case '>': return CT_CANGLE;		// >
		
	}
	return CT_NULL;
}

template<typename T=std::string> struct Token
{
	int Type; //token-type
	T Tok;
	TokPos P;
	void clear() { Type=CT_NULL; Tok={}; P.clear(); }
	Token() : Type(CT_NULL), Tok{}, P{} {}
	Token(const T &tok, TokPos p, int type) : Type(type), Tok(tok), P(p) {}
	Token(const T &tok, int x, int y, int type) : Type(type), Tok(tok), P(x,y) {}
	Token(const Token &token) { *this=token; }
	Token& operator=(const Token &token) { Type=token.Type; Tok=token.Tok; P=token.P; return *this; }
	friend std::ostream& operator<<(std::ostream &os, const Token<T> &token) { os << token.Tok; return os; }
};

//using AToken=Token<std::string>;

//template<typename T=AToken> struct Tokens
template<typename T=Token<>> struct Tokens
{
	using Token_Type=T;
private:
	std::vector<T> vtoks;
public:
	
	using type=T;
	
	using iterator=typename std::vector<T>::iterator;
	using const_iterator=typename std::vector<T>::const_iterator;
	
	iterator begin() { return vtoks.begin(); }
	iterator end() { return vtoks.end(); }
	const_iterator begin() const { return vtoks.begin(); }
	const_iterator end() const { return vtoks.end(); }
	
	const T T_NO_THROW; //avoid exceptions? wise?
	size_t index; //always points to next token
	void clear()			{ vtoks.clear(); index=0; }
	void push(const T &t)	{ vtoks.push_back(t); }
	virtual ~Tokens()		{ clear(); }
	Tokens()				{ clear(); }
	Tokens(const Tokens &tokens) { clear();  *this=tokens; }

	Tokens& operator=(const Tokens &tokens) { vtoks=tokens.vtoks;  return *this; }

	size_t Index()			{ return index; }
	size_t size() const		{ return vtoks.size(); }
	void back()				{ if (index>0) index--; }
	void reset_index()		{ index=0; }
	
	bool next(T &t)
	{
		t.clear();
		if (index<size()) { t=vtoks.at(index); index++; return true; }
		return false;
	}

	bool peek_next(T &t, int dist=1) const
	{
		if (((dist<0)&&(size_t(abs(dist))>index))||((dist>0)&&((dist+index)>=size()))) return false;
		t=vtoks.at(index+dist);
		return true;
	}
	bool peek_prev(T &t, int dist=1) const { return peeknext(t, -dist); } //placebo for a pedant
	
	const T& peek(int dist=0) const //throws! (if you don't want to throw use peek_next)
	{
		//if (((dist<0)&&(size_t(abs(dist))>index))||((dist>0)&&((dist+index)>=size()))) throw std::out_of_range("Tokens<>::peek");
		if (((dist<0)&&(size_t(abs(dist))>index))||((dist>0)&&((dist+index)>=size()))) return T_NO_THROW;
		return vtoks.at(index+dist);
	}
	
	const T& peek_at(size_t idx) const //throws!
	{
		//if (idx>=size()) throw std::out_of_range("Tokens<>::peek_at");
		if (idx>=size()) return T_NO_THROW;
		return vtoks.at(idx);
	}
	
	bool is_val(const std::string val, int dist) const
	{
		if (((dist<0)&&(size_t(abs(dist))>index))||((dist>0)&&((dist+index)>=size()))) return false;
		return sieqs(val, vtoks.at(index+dist).Tok);
	}

	bool is_val_at(const std::string val, size_t idx) const
	{
		if (idx>=size()) return false;
		return sieqs(val, vtoks.at(idx).Tok);
	}

	bool is_type(int type, int dist) const
	{
		if (((dist<0)&&(size_t(abs(dist))>index))||((dist>0)&&((dist+index)>=size()))) return false;
		return (vtoks.at(index+dist).Type==type);
	}

	bool is_type_at(int type, size_t idx) const
	{
		if (idx>=size()) return false;
		return (vtoks.at(idx).Type==type);
	}

};

//using ATokens=Tokens<AToken>;
//using UTokens=Tokens<UToken>;
/*
template<typename C> bool IsNoise(C c)		{ return ((c==32)||(c==9)||(c==10)||(c==13)); }
template<typename C> bool IsSpace(C c)		{ return (c==32); }
template<typename C> bool IsTab(C c)		{ return (c==9); }
template<typename C> bool IsNewline(C c)	{ return ((c==10)||(c==13)); }
template<typename C> bool IsQuote(C c)		{ return ((c=='"')||(c=='\'')||(c=='`')); }
template<typename C> bool IsDigit(C c)		{ return ((c>='0')&&(c<='9')); }
template<typename C> bool IsOperator(C c)	{ return (std::string("+-*&/|^%~!<>?:=").find(c)!=std::string::npos); }
template<typename C> bool IsAlpha(C c)		{ return ((c=='_')||((c>='A')&&(c<='Z'))||((c>='a')&&(c<='z'))); } //underscore assumed to be alpha
template<typename C> bool IsAlphaNum(C c)	{ return (IsAlpha(c)||IsDigit(c)); }
template<typename C> bool IsBelowASCII(C c)	{ return (c<32); }
template<typename C> bool IsASCII(C c)		{ return ((c>=32)&&(c<=127)); }
template<typename C> bool IsAboveASCII(C c)	{ return (c>127); }
template<typename C> bool IsOtherASCII(C c)	{ return (IsASCII(c)&&!IsAlphaNum(c)); }
*/

template<typename C> bool IsNoise(C c)		{ int ct=ct_type(c); return ((ct==CT_SPACE)||(ct==CT_TAB)||(ct==CT_CR)||(ct==CT_LF)); }
template<typename C> bool IsSpace(C c)		{ return (ct_type(c)==CT_SPACE); }
template<typename C> bool IsTab(C c)		{ return (ct_type(c)==CT_TAB); }
template<typename C> bool IsNewline(C c)	{ int ct=ct_type(c); return ((ct==CT_LF)||(ct==CT_CR)); }
template<typename C> bool IsQuote(C c)		{ int ct=ct_type(c); return ((ct==CT_SQUOTE)||(ct==CT_DQUOTE)||(ct==CT_BQUOTE)); }
template<typename C> bool IsDigit(C c)		{ return (ct_type(c)==CT_DIGIT); }
template<typename C> bool IsOperator(C c)	{ return (ct_type(c)==CT_OPERATOR); }
template<typename C> bool IsAlpha(C c)		{ int ct=ct_type(c); return ((ct==CT_USCORE)||(ct==CT_ALPHA)); } //underscore assumed to be alpha
template<typename C> bool IsAlphaNum(C c)	{ return (IsAlpha(c)||IsDigit(c)); }
template<typename C> bool IsBelowASCII(C c)	{ return (((int)c)<32); }
template<typename C> bool IsControlChar(C c){ return (((int)c)<32); }
template<typename C> bool IsASCII(C c)		{ return ((((int)c)>=32)&&(((int)c)<=127)); }
template<typename C> bool IsAboveASCII(C c)	{ return (((int)c)>127); }
template<typename C> bool IsOtherASCII(C c)	{ return (IsASCII(c)&&!IsAlphaNum(c)); }


bool IsInteger(const std::string &s);
bool IsDecimal(const std::string &s);

enum //WHITESPACE_DISPOSITION === space/tab/lf/cr/?
{
	WSD_KEEP=0, //keep all
	WSD_DISCARD, //discard all
	WSD_REDUCE, //reduce to single space
	WSD_REDUCE_NL, //reduces to single space|newline
};

inline bool isWSD(int v, int w) { return ((v&w)==w); }

template<typename CHAR=char, typename STRING=std::string, typename TOKEN=Token<STRING>, typename TOKENLIST=Tokens<TOKEN> >
size_t tokenize(TOKENLIST &TL, const STRING &S, int wsdisp=WSD_REDUCE, bool bHasQuotedStrings=false)
{
	if (!S.size()) return 0;
	STRING t;
	CHAR c;
	size_t pos=0, line=1, col=1, X, Y;
	auto Get=[&pos, &S](CHAR &u)->bool{ if (pos<S.size()) { u=S.at(pos++); return true; } return false; };
	//auto UnGet=[&pos](){ --pos; };
	auto Peek=[&pos, &S](CHAR &u)->bool{ if (pos<S.size()) { u=S.at(pos); return true; } return false; };
	auto Prev=[&pos, &S]()->CHAR{ CHAR u{}; if (pos>1) u=S.at(pos-2); return u; };

	TL.clear();
	while (Get(c))
	{
		col++;
		//--------------------
		if (IsNoise(c))
		{
			if (!t.empty()) { TL.push(TOKEN(t, (col-t.length()-1), line, TT_TEXT)); t.clear(); }
			if (IsNewline(c)) { line++; col=1; } else col++;
			if (wsdisp==WSD_DISCARD) continue;
			else if (wsdisp==WSD_REDUCE) //..to a single space
			{
				t=32;
				while (Peek(c)&&IsNoise(c)) { Get(c); if (IsNewline(c)) { line++; col=1; } else col++; }
			}
			else if (wsdisp==WSD_REDUCE_NL) //..to a single space or single newline
			{
				while (Peek(c)&&IsNoise(c))
				{
					Get(c);
					if (IsNewline(c)) { line++; col=1; t='\n'; }
					else { if (t!="\n") { t=32; col++; }}
				}
			}
			else t=c; //default: assume (wsdisp==WSD_KEEP)
			X=col-1; Y=line;
			TL.push(TOKEN(t, X, Y, TT_SCHAR));
			t.clear();
		}
		//--------------------
		else if (IsQuote(c)&&(Prev()!=(CHAR)'\\'))
		{
			//if (!t.empty()) { TL.push(TOKEN(t, (col-t.length()-1), line, TT_TEXT)); t.clear(); } ///??? eg.: it's
			if (bHasQuotedStrings)
			{
				size_t p=0;
				CHAR cq=c; //double, single or grave
				t+=cq;
				while (Get(c))
				{
					col++;
					if ((c==cq) && !p) { t+=cq; break; }
					if (c==(CHAR)'\\') p=c; else p=0; //to check for escaped (\) quote
					if (IsNewline(c)) { line++; col=1; } //don't apply wsdisp
					t+=c;
				}
				TL.push(TOKEN(t, (col-t.size()-1), line, TT_QSTR)); //quotes included
			}
			else TL.push(TOKEN(t, X, Y, TT_SCHAR));
			t.clear();
		}
		//--------------------
		else if (IsOtherASCII(c)||IsControlChar(c)) //||IsQuote(c))
		{
			if (!t.empty()) TL.push(TOKEN(t, (col-t.size()-1), line, TT_TEXT));
			X=col-1; Y=line;
			t=c;
			TL.push(TOKEN(t, X, Y, TT_SCHAR));
			t.clear();
		}
		//--------------------
		else { t+=c; } //..accumulate into single token
	}
	if (!t.empty()) TL.push(TOKEN(t, (col-t.length()-1), line, TT_TEXT));
	
/*
//scan for quotes etc here...
//---- MAYBE... quotes need to be handled elsewhere---------------- by caller
	
	input: ab \" cd ' "ef g" h's ij 3.1415 rads
	currently parsed as: (ab) ( ) (") ( ) (cd) ( ) (') ( ) (") (ef) ( ) (g) (") ( ) (h) (') (s) ( ) (ij) ( ) (3) (.) (1415) ( ) (rads)
	((this is to be handled ('elsewhere') by caller))
	
	or exmpl:		[...] pi=3.1415 end of para 3. 4. Para four [...]
	parsed as:		[...] (pi) (=) (3) (.) (1415) (end) (of) (para) (3) (.) (4) (.) (Para) (four) [...]
	with REDUCE:	[...] (pi) (=) (3) (.) (1415) ( ) (end) ( ) (of) ( ) (para) ( ) (3) (.) ( ) (4) (.) ( ) (Para) ( ) (four) [...]
	
	
	should parse as 1?: (ab) (") (cd) (' "ef g" h') (s) (ij)
	................2?: (ab) (") (cd) (') ("ef g") (h) (') (s) (ij)
	
// to scan thru vtoks for matching quotes, & to find apostrophes, misplaced quotes, etc
// maybe: use temp list to collect on first pass above, then filter the temp-list for quotes
// and strings contained within strings, or single quoted strings containing dbl-quoted strings & vice versa
// & fill the param-list with tokens for valid quoted strings, and apostrophe-words

			if (IsQuote(c))
			{
				if (bHasQuotedStrings)
				{
					p=0; cq=c;
					ins.get(c); col++;
					if (Get(c)) t+=c;
					while (ins.good() && ((c!=cq) || (p=='\\')))
					{
						if (IsNewline(c)) { line++; col=1; } don't apply wsdisp
						p=c; to check for escaped (\) quote
						ins.get(c); col++;
						if (ins.good()) t+=c;
					}
					TL.push_back(TToken(t, X, Y, TT_QSTR)); quotes included
				}
				else TL.push_back(TToken(t, X, Y, TT_SCHAR));
				t="";
			}
*/
//--------------------------------------------------------------

	
	return TL.size();
}


//===================================================================================================
//NB: add compiler flag -fconcepts
template<typename T> concept bool CpCon() { return std::is_copy_constructible<T>::value; }
template<typename T> concept bool IsCon() { return (std::is_convertible<T, std::string>::value || std::is_pod<T>::value); }

template<CpCon T, IsCon U=std::string> struct Share
{
	using Share_Type=T*;
	typedef U Key_Type;
	std::mutex MUX_SHARE;

private:
	struct Item
	{
	private:
		Key_Type k{};
		Share_Type p{};
		int u{}; //use-count
	public:
		Key_Type Key() { return k; }
		int Usecount() { return u; }
		Share_Type Sharetype() { return p; }
		~Item() {}
		Item(Key_Type K, Share_Type P) : k(K), p(P), u(1) {}
		friend class Share;
	};

	typedef std::vector<Item> VI;

	Share(const Share&); //no copying because pointers
	Share& operator=(const Share&);
	VI vi{};

public:
	typedef typename VI::iterator iterator;
	typedef typename VI::const_iterator const_iterator;
	typedef typename VI::reverse_iterator reverse_iterator;
	typedef typename VI::const_reverse_iterator const_reverse_iterator;

	iterator				begin()			{ return vi.begin(); }
	const_iterator			cbegin() const	{ return vi.cbegin(); }
	iterator				end()			{ return vi.end(); }
	const_iterator			cend() const	{ return vi.cend(); }
	reverse_iterator		rbegin()		{ return vi.rbegin(); }
	const_reverse_iterator	crbegin() const	{ return vi.crbegin(); }
	reverse_iterator		rend()			{ return vi.rend(); }
	const_reverse_iterator	crend() const	{ return vi.crend(); }

	bool empty() { return vi.empty(); }
	size_t size() { return vi.size(); }
	void destroy() { if (!empty()) { for (auto& I:vi) delete I.p; } vi.clear(); }

	Share_Type add(Key_Type K, const T &t)
	{
		std::lock_guard<std::mutex> g(MUX_SHARE);
		for (auto& I:vi) { if (I.k==K) { I.u++; return I.p; }}
		return vi.emplace_back(K, (Share_Type)(new T(t))).p;
	}

	Share_Type copy(Share_Type pt) { return add(Key(pt), *pt); }

	void remove(Share_Type *pt)
	{
		std::lock_guard<std::mutex> g(MUX_SHARE);
		if (*pt)
		{
			auto it=begin();
			while (it!=end())
			{
				if ((*it).p==(*pt))
				{
					if (--(*it).u<=0) { delete (*it).p; vi.erase(it); }
					*pt=nullptr;
					return;
				}
				else it++;
			}
		} //else ? throw or ignore ?
	}

	void clear() { destroy(); }

	Key_Type Key(Share_Type pt)
	{
		if (pt) { for (auto I:vi) { if (I.p==pt) { return I.Key(); }}}
		return Key_Type();
	}

	int Usecount(Share_Type pt)
	{
		if (pt) { for (auto I:vi) { if (I.p==pt) { return I.Usecount(); }}}
		return 0;
	}

	~Share() { destroy(); }
	Share() { vi.clear(); }

};


//===================================================================================================
//template<std::equality_comparable T> struct TList /* when c++20 & concepts.h is available */
template<typename T> struct TList
{
private:
	using VecType=std::vector<T>;
	VecType VT{};
	bool bDelete{false};
	
public:
	using iterator=typename VecType::iterator;
	using const_iterator=typename VecType::const_iterator;
	using reverse_iterator=typename VecType::reverse_iterator;
	using const_reverse_iterator=typename VecType::const_reverse_iterator;
	
	iterator				begin()			{ return VT.begin(); }
	iterator				end()			{ return VT.end(); }
	const_iterator			cbegin() const	{ return VT.cbegin(); }
	const_iterator			cend() const	{ return VT.cend(); }
	reverse_iterator		rbegin()		{ return VT.rbegin(); }
	reverse_iterator		rend()			{ return VT.rend(); }
	const_reverse_iterator	crbegin() const	{ return VT.crbegin(); }
	const_reverse_iterator	crend() const	{ return VT.crend(); }

	void clear() { if (bDelete) for (auto t:VT) delete t; VT.clear(); }
	bool empty() const { return VT.empty(); }
	size_t size() const { return VT.size(); }
	virtual ~TList() { clear(); }
	TList(bool bdel=false) { clear(); bDelete=bdel; }
	TList(const TList &L) { *this=L; }
	TList operator=(const TList &L) { clear(); bDelete=L.bDelete; for (auto t:L) Add(t); return *this; }
	void set_delete(bool bdel=true) { bDelete=bdel; }
	bool has(const T &t) const
	{
		for (auto tt:VT)
		{
			if (bDelete) { if (*tt==*t) return true; }
			else if (t==tt) return true;
		}
		return false;
	}
	bool Add(T t) { if (!has(t)) { VT.push_back(t); return true; } return false; }
	void Remove(T t) { auto it=VT.begin(); while (it!=VT.end()) { if ((*it)==t) { if (bDelete) delete (*it); VT.erase(it); return; } else it++; }}
	
};


//===================================================================================================
struct Storage //container for mixed types
{
	using MIS=std::map<size_t, std::any>;
	using iterator=MIS::iterator;
	using const_iterator=MIS::const_iterator;
	using reverse_iterator=MIS::reverse_iterator;
	using const_reverse_iterator=MIS::const_reverse_iterator;
	MIS mis;
	
	void	clear()			{ mis.clear(); }
	size_t	size()			{ return mis.size(); }
	bool	has(size_t si)	{ return !(mis.find(si)==mis.end()); }
	void	del(size_t si)	{ if (has(si)) mis.erase(si); }
	
	virtual ~Storage()						{ clear(); }
	Storage()								{ clear(); }
	Storage(const Storage &S)				{ *this=S; }
	Storage& operator=(const Storage &S)	{ clear(); for (auto p:S.mis) mis[p.first]=p.second; return *this; }
	
	template<typename T> bool	istype(size_t si)	{ try { std::any_cast<T>(mis[si]); return true; } catch(...) { return false; }};
	template<typename T> size_t	hastype()			{ for (auto p:mis) if (istype<T>(p.first)) return p.first; return 0; }
	
	template<typename T> int	store(T t)			{ size_t si=1; while (has(si)) si++; mis[si]=t; return si; }
	
	template<typename T> bool	fetch(size_t si, T &t)
	{
		if (has(si)) { t=std::any_cast<T>(mis[si]); del(si); return true; }
		return false;
	}
	
	template<typename T> auto& use(size_t si)
	{
		if (has(si))
		{
			try { return std::any_cast<T&>(mis.at(si)); }
			catch(...) { throw std::logic_error("invalid stored type"); }
		}
		else throw std::out_of_range("invalid storage index");
	}
	
	iterator begin()							{ return mis.begin(); }
	iterator end()								{ return mis.end(); }
	const_iterator cbegin() const				{ return mis.cbegin(); }
	const_iterator cend() const					{ return mis.cend(); }
	reverse_iterator rbegin()					{ return mis.rbegin(); }
	reverse_iterator rend()						{ return mis.rend(); }
	const_reverse_iterator crbegin() const		{ return mis.crbegin(); }
	const_reverse_iterator crend() const		{ return mis.crend(); }
	
};


//===================================================================================================
#if defined(PLATFORM_POSIX) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix)
template<typename...T> void _S_R_A_(const std::string &sc)
	{
		std::string s{};
		s=says("nohup ", sc, " 1&2>/dev/null &");
		std::system(s.c_str());
	}
#elif defined(_WIN64)
template<typename...T> void _S_R_A_(const std::string &sc) //assuming windows - NOT TESTED!
	{
		std::string s{};
		s=says("start ", sc);
		std::system(s.c_str());
	}
#endif

template<typename...T> void SysRunApp(const std::string &sApp, T...t)
	{
		std::string sa{};
		sa=says(sa, sApp, " ", t...);
		std::thread(_S_R_A_, sa).detach();
	}


//===================================================================================================
template<class X> struct LinkList
{
	struct LLNode
	{
		LLNode *p{nullptr};
		LLNode *n{nullptr};
		X x; //X must cctor, =()
		LLNode() : p{nullptr}, n{nullptr} {}
	};
	LLNode *h{nullptr};
	LLNode *t{nullptr};
	size_t llcount{0};

	void _drop_node_(LLNode *p)
		{
			if (p)
			{
				if (p->p) { p->p->n=p->n; }
				if (p->n) { p->n->p=p->p; }
				if (p==h) { h=p->n; }
				if (p==t) { t=p->p; }
				if (llcount>0) --llcount;
				delete p;
			}
		}

	void clear() { while (h) _drop_node_(h); h=t=nullptr; llcount=0; }
	bool empty() const { return (h==nullptr); }
	size_t size() const { return llcount; }

	~LinkList() { clear(); }
	LinkList() : h{nullptr}, t{nullptr}, llcount{0} {}
	LinkList(const LinkList &LL) { *this=LL; }

	LinkList& operator=(const LinkList &LL) { clear(); LLNode *c=LL.h; while (c) { add(c->x); c=c->n; } return *this; }
	
	X* add(X x)
		{
			X *pX=nullptr;
			LLNode *p=new LLNode;
			if (p)
			{
				if (!h) { h=p; t=h; }
				else { p->p=t; t->n=p; t=p; }
				++llcount;
				p->x=x;
				pX=&p->x;
			}
			return (pX);
		}

	X* addat(X x, size_t idx)
		{
			X *pX=nullptr;
			size_t i=0;
			auto *pN=h;
			while (pN&&(i!=idx)) { pN=pN->n; i++; }
			if (pN)
			{
				LLNode *p=new LLNode;
				if (p)
				{
					p->x=x;
					if (pN->p) pN->p->n=p;
					p->p=pN->p;
					pN->p=p;
					p->n=pN;
					if (h==pN) h=p;
					++llcount;
					pX=&p->x;
				}
			}
			else return add(x); //if empty or idx>size
			return (pX);
		}

	bool remove(X *pX)
		{
			auto *p=h;
			while (p&&(pX!=&p->x)) p=p->n;
			if (p) { _drop_node_(p); return true; }
			return false;
		}

	bool remove(size_t idx)
		{
			if (idx<llcount) //throw std::out_of_range("LinkList::remove");
			{
				size_t i=0;
				auto *p=h; while (p&&(i!=idx)) { i++; p=p->n; }
				if (i==idx) { _drop_node_(p); return true; }
			}
			return false;
		}

	X* operator[](size_t idx)
		{
			X *pX=nullptr;
			size_t i=0, bug=0;
			auto *p=h;
			while (p&&(i!=idx)) { p=p->n; i++; }
			if (p) pX=&p->x;
			return pX;
		}

/*
	const X& operator[](size_t idx) const
		{
			if (idx>=llcount) throw std::out_of_range("LinkList::operator[] const"); //return X();
			size_t i=0;
			LLNode *p=h;
			while (i!=idx) { p=p->n; i++; }
			return p->x;
		}

	X& operator[](size_t idx)
		{
			if (idx>=llcount) throw std::out_of_range("LinkList::operator[]"); //return X();
			size_t i=0;
			LLNode *p=h;
			while (i!=idx) { p=p->n; i++; }
			return p->x;
		}
*/


};

//===================================================================================================
//general D-dimensional point of type N
template<typename N, int D=2> struct NDPoint //default 2D
{
	std::vector<N> VN{};
	int DIM;

	void clear() { VN.clear(); for (int i=0; i<D; i++) VN.push_back(0); }
	~NDPoint() {}
	NDPoint() { DIM=D; clear(); if (DIM<2) throw std::logic_error("invalid dimension"); }
	NDPoint(const NDPoint &P) { *this=P; }
	template<typename...A> NDPoint(A...a) : VN{(N)a...} { DIM=VN.size(); if (DIM<2) throw std::logic_error("invalid dimension"); }
	NDPoint(const std::vector<N> &vn) : VN{vn} { DIM=vn.size(); if (DIM<2) throw std::logic_error("invalid dimension"); }

	NDPoint& operator=(const NDPoint &P) { VN.clear(); DIM=P.DIM; for (auto p:P.VN) VN.push_back(p); return *this; }
	template<typename M, int C> NDPoint& operator=(const NDPoint<M,C> &P) { VN.clear(); DIM=P.DIM; for (auto p:P.VN) VN.push_back(p); return *this; }

	bool operator==(const NDPoint &P) const { for (size_t i=0;i<DIM;i++) { if (VN[i]!=P.VN[i]) return false; } return true; }
	bool operator!=(const NDPoint &P) const { return !(*this==P); }

	int dimension() const { return DIM; }

	NDPoint& operator+=(const NDPoint &P) { throw::std::logic_error("NDPoint: addition undefined"); } //?bullshit - you get another point!
///	NDPoint& operator+=(const NDPoint &P) { for (size_t i=0;i<DIM;i++) (VN[i]+=P.VN[i]); return *this; }
	NDPoint& operator*=(double d) { for (auto& p:VN) p=(N)((double)p*d); return *this; }
	NDPoint& operator*=(int n) { return ((*this)*=(double)n); }
	NDPoint& operator/=(double d) { for (auto& p:VN) p=(N)((double)p/d);  return *this; }
	NDPoint& operator/=(int n) { return ((*this)/=(double)n); }

	N& operator[](size_t idx) { if (idx<DIM) return VN[idx]; throw std::out_of_range("NDPoint: index out of range"); }
	N operator[](size_t idx) const { if (idx<DIM) return VN[idx]; throw std::out_of_range("NDPoint: index out of range"); }

	//convenience
	N& x() { return (*this)[0]; }
	N x() const { return (*this)[0]; }
	N& y() { return (*this)[1]; }
	N y() const { return (*this)[1]; }
	N& z() { if (DIM<3) std::out_of_range("NDPoint: dimension-error"); return (*this)[2]; }
	N z() const { if (DIM<3) std::out_of_range("NDPoint: dimension-error"); return (*this)[2]; }
	N& w() { if (DIM<4) std::out_of_range("NDPoint: dimension-error"); return (*this)[3]; }
	N w() const { if (DIM<4) std::out_of_range("NDPoint: dimension-error"); return (*this)[3]; }

	friend std::ostream& operator<<(std::ostream &os, const NDPoint &P) { os << "(" << P[0]; for (int i=1; i<P.DIM; i++) os << ", " << P[i]; os << ")"; return os; }

};

template<typename N, int D=2> NDPoint<N, D> operator*(const NDPoint<N, D> &l, double d) { NDPoint<N, D> p(l); p*=d; return p; }
template<typename N, int D=2> NDPoint<N, D> operator*( double d, const NDPoint<N, D> &l) { return (l*d); }
template<typename N, int D=2> NDPoint<N, D> operator*(const NDPoint<N, D> &l, int n) { return (l*double(n)); }
template<typename N, int D=2> NDPoint<N, D> operator*(int n, const NDPoint<N, D> &l) { return (l*double(n)); }
template<typename N, int D=2> NDPoint<N, D> operator/(const NDPoint<N, D> &l, double d) { NDPoint<N, D> p(l); p/=d; return p; }
template<typename N, int D=2> NDPoint<N, D> operator/(const NDPoint<N, D> &l, int n) { return (l/double(n)); }

//general D-dimensional vector of type N
template<typename N, int D=2> struct NDVector : public NDPoint<N, D>
{
	using NDP=NDPoint<N, D>;
	~NDVector() {}
	NDVector() {}
	NDVector(const NDVector &V) { *this=V; }
	NDVector(const NDPoint<N,D> &P) { *this=P; }
	template<typename...A> NDVector(A...a) : NDP{(N)a...} { } //NDP::DIM=NDP::VN.size(); }
	NDVector(const std::vector<N> &vn) : NDP{vn} { }

	NDVector& operator=(const NDVector &V) { NDP::VN.clear(); NDP::DIM=V.DIM; for (auto p:V.VN) NDP::VN.push_back(p); return *this; }
	NDVector& operator=(const NDPoint<N,D> &P) { NDP::VN.clear(); NDP::DIM=P.DIM; for (auto p:P.VN) NDP::VN.push_back(p); return *this; }

	template<typename M, int C> NDVector& operator=(const NDVector<M,C> &V) { NDP::VN.clear(); NDP::DIM=V.DIM; for (auto p:V.VN) NDP::VN.push_back((N)p); return *this; }

	bool operator==(const NDVector &V) const
		{
			if (NDP::DIM!=V.DIM) return false;
			for (size_t i=0; i<NDP::DIM; i++) { if (NDP::VN[i]!=V.VN[i]) return false; }
			return true;
		}
	bool operator!=(const NDVector &V) const { return !(*this==V); }

	NDVector& operator+=(const NDVector &V) { for (size_t i=0; i<NDP::DIM; i++) NDP::VN[i]+=V.VN[i]; return *this; }

	//??
	NDVector& operator+=(const NDPoint<N,D> &P) { NDVector<N,D> V(P); return ((*this)+=V); }
	//??
	
	NDVector& operator-=(const NDVector &V) { NDVector v(V); v*=(-1); (*this)+=v; return *this; }
	NDVector& operator-=(const NDPoint<N,D> &P) { NDVector<N,D> V(P); return ((*this)-=V); }
	NDVector& operator*=(double d) { for (auto& p:NDP::VN) p=(N)((double)p*d); return *this; }
	NDVector& operator*=(int n) { return ((*this)*=(double)n); }
	NDVector& operator/=(double d) { for (auto& p:NDP::VN) p=(N)((double)p/d);  return *this; }
	NDVector& operator/=(int n) { return ((*this)/=(double)n); }

	double dotproduct() const { double d{0.0}; for (size_t i=0; i<NDP::DIM; i++) d+=(NDP::VN[i]*NDP::VN[i]); return d; } //convenience
	double dotproduct(const NDVector &V) const { double d{0.0}; for (size_t i=0; i<NDP::DIM; i++) d+=(NDP::VN[i]*V.VN[i]); return d; }
	double length() const { return sqrt(dotproduct()); }

	NDVector crossproduct(const NDVector &V) const
		{
			if (NDP::DIM!=3) throw std::logic_error("NDVector: crossproduct vectors not 3D");
			//R-hand-rule: index along 'this'
			//	fingers thru smallest angle to 'V'
			//	thumb will be in direction of 'R'
			//if == 0 => parallel
			NDVector R;
			R.x()=((this->y()*V.z())-(this->z()*V.y()));
			R.y()=((this->z()*V.x())-(this->x()*V.z()));
			R.z()=((this->x()*V.y())-(this->y()*V.x()));
			return R;
		}

	operator NDPoint<N,D>() { return NDPoint<N,D>(NDP::VN); }

	NDVector& normalize() { (*this)/=length(); return *this; }
	NDVector normalize() const { NDVector P(*this); P/=P.length(); return P; }
	NDVector direction() const { return NDVector(*this).normalize(); }
	
	double angle_with(const NDPoint<N,D> &P) { double a; NDVector l=((const NDVector)*this).normalize(), r(P); r.normalize(); r-=l; a=acosf(l.dotproduct(r)); return a; }
	double angle_with(const NDVector &V) { double a; NDVector l=((const NDVector)*this).normalize(), r=V.normalize(); a=acosf(l.dotproduct(r)); return a; }
	

};

template<typename N, int D=2> double dotproduct(const NDVector<N,D> &l, const NDVector<N,D> &r)				{ return (l.dotproduct(r)); }
template<typename N, int D=2> NDVector<N,D> crossproduct(const NDVector<N,D> &l, const NDVector<N,D> &r)	{ NDVector<N,D> V(l); return V.crossproduct(r); }
template<typename N, int D=2> NDVector<N,D> direction(const NDPoint<N,D> &F, const NDPoint<N,D> &T)			{ return (T-F).normalize(); }

//??
template<typename N, int D=2> NDVector<N,D> operator+(const NDPoint<N,D> &l, const NDVector<N,D> &r)	{ NDVector<N,D> V(l); V+=r; return V; } //(NDPoint<N,D>)V; }
template<typename N, int D=2> NDVector<N,D> operator+(const NDVector<N,D> &l, const NDPoint<N,D> &r)	{ NDVector<N,D> V(l); V+=r; return V; } //(NDPoint<N,D>)V; }
//??

template<typename N, int D=2> NDVector<N,D> operator+(const NDVector<N,D> &l, const NDVector<N,D> &r)	{ NDVector<N,D> V(l); V+=r; return V; }
template<typename N, int D=2> NDVector<N,D> operator-(const NDPoint<N,D> &l, const NDPoint<N,D> &r)		{ NDVector<N,D> V(l); V-=r; return V; }
template<typename N, int D=2> NDVector<N,D> operator-(const NDVector<N,D> &vl, const NDPoint<N,D> &pr)	{ NDVector<N,D> V(vl); V-=pr; return V; }
template<typename N, int D=2> NDVector<N,D> operator-(const NDVector<N,D> &l, const NDVector<N,D> &r)	{ NDVector<N,D> V(l); V-=r; return V; }

template<typename N, int D=2> NDVector<N, D> operator*(const NDVector<N, D> &l, double d)	{ NDVector<N, D> p(l); p*=d; return p; }
template<typename N, int D=2> NDVector<N, D> operator*( double d, const NDVector<N, D> &l)	{ return (l*d); }
template<typename N, int D=2> NDVector<N, D> operator*(const NDVector<N, D> &l, int n)		{ return (l*double(n)); }
template<typename N, int D=2> NDVector<N, D> operator*(int n, const NDVector<N, D> &l)		{ return (l*double(n)); }
template<typename N, int D=2> NDVector<N, D> operator/(const NDVector<N, D> &l, double d)	{ NDVector<N, D> p(l); p/=d; return p; }
template<typename N, int D=2> NDVector<N, D> operator/(const NDVector<N, D> &l, int n)		{ return (l/double(n)); }





#endif







