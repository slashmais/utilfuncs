/*
    This file is part of utilfuncs-utility comprising utilfuncs.h and utilfuncs.cpp

    utilfuncs-utility - Copyright 2018 Marius Albertyn

    utilfuncs-utility is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    utilfuncs-utility is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with utilfuncs-utility in a file named COPYING.
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

#include <filesystem> //c++17! (in gcc v7.1)
namespace FSYS=std::filesystem;
//#include <experimental/filesystem> //NB: need to link with libstdc++fs
//namespace FSYS=std::experimental::filesystem;


//--------------------------------------------------------------------------------------------------

const std::string get_error_report();
void clear_error_report();
bool report_error(const std::string serr, bool btell=true);//=false);

//--------------------------------------------------------------------------------------------------

void PRINTSTRING(const std::string &s);
const std::string READSTRING(const std::string &sprompt);

#if defined(flagGUI)
void MsgOK(std::string msg, std::string title="Message");
#endif

const std::string askuser(const std::string &sprompt);

void waitenter(const std::string &msg="");
bool checkkeypress(int k);

bool askpass(std::string &pw); //cli-only

const std::string username();
const std::string username(int uid);
const std::string homedir();
const std::string hostname();

#if defined(PLATFORM_POSIX) || defined(__linux__)
	int realuid();
	int effectiveuid();
	bool has_root_access();
#endif

template<typename...P> std::string spf(P...p) { std::stringstream ss(""); (ss<<...<<p); return ss.str(); }
template<typename...P> void spfs(std::string &s, P...p) { s+=spf(p...); } //APPENDS! to s!
template<typename...P> void telluser(P...p) { PRINTSTRING(spf(p...)); }
template<typename...P> bool tellerror(P...p) { PRINTSTRING(spf("ERROR: ", p...)); return false; }

#define TO_DO telluser("To Do: '", __func__, "', in ", __FILE__, " [", long(__LINE__), "]");
#define to_do TO_DO

//#define DEBUG telluser

//--------------------------------------------------------------------------------------------------
//template<typename...T> void logger(T...t) { std::ofstream ofs("LOGGER.LOG", std::ios::app); (ofs<<...<<t)<<"\n"; ofs.close(); }
template<typename...T> void logger(T...t) { std::ofstream ofs(homedir()+"/LOGGER.LOG", std::ios::app); (ofs<<...<<t)<<"\n"; ofs.close(); }

//--------------------------------------------------------------------------------------------------
inline bool is_name_char(int c)	{ return (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')) || ((c>='a')&&(c<='z')) || (c=='_')); }
// bash/linux...
const std::string bash_escape_name(const std::string &name);

//--------------------------------------------------------------------------------------------------
__attribute__((always_inline)) inline void kips(int sec) { std::this_thread::sleep_for(std::chrono::seconds(sec)); } //1.0
__attribute__((always_inline)) inline void kipm(int milsec) { std::this_thread::sleep_for(std::chrono::milliseconds(milsec)); } //0.001 (x/1000)
__attribute__((always_inline)) inline void kipu(int microsec) { std::this_thread::sleep_for(std::chrono::microseconds(microsec)); } //0.000001 (x/1000000)
__attribute__((always_inline)) inline void kipn(int nanosec) { std::this_thread::sleep_for(std::chrono::nanoseconds(nanosec)); } //0.000 000 001 (x/1000000000)

inline auto getclock() { return std::chrono::steady_clock::now(); }
//inline auto getelapsed(auto b, auto e) { return std::chrono::duration_cast<std::chrono::nanoseconds>(e-b).count(); }
template<typename B, typename E> auto getelapsed(B b, E e) { return std::chrono::duration_cast<std::chrono::nanoseconds>(e-b).count(); }
//USAGE: auto b=getclock(); some_func_to_be_timed(...); auto duration=getelapsed(b, getclock()); ..

typedef uint64_t DTStamp;
DTStamp dt_stamp();
std::string ymdhms_stamp();
const std::string h_dt_stamp(); //human-readable: "0y0m0d0H0M0Sxx" (e.g.: "181123111051A3"=>2018 Nov 23 11:10:51 (xx/A3 = reduced-microsec-suffix)
DTStamp ToDTStamp(const std::string &yyyymmddHHMMSS); //HHMMSS-optional
const std::string ToDateStr(DTStamp dts);

inline bool is_leap_year(int y) { return (((y%4==0)&&(y%100!= 0))||(y%400==0)); }

uint64_t get_unique_index();
std::string get_unique_name(const std::string sprefix="U", const std::string ssuffix="");
inline bool isvalidname(const std::string &sN) { return (sN.find('/')==std::string::npos); }


//--------------------------------------------------------------------------------------------------
template<typename TypePtr> size_t p2t(TypePtr p) { union { TypePtr u; size_t n; }; u=p; return n; }
template<typename TypePtr> TypePtr t2p(size_t t) { union { TypePtr u; size_t n; }; n=t; return u; }

//--------------------------------------------------------------------------------------------------
///todo: whitespace chars - beep, para.., backspace, etc
#define WHITESPACE_CHARS (const char*)" \t\n\r"
void LTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void RTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void TRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void ReplacePhrase(std::string &sTarget, const std::string &sPhrase, const std::string &sReplacement); //each occ of phrase with rep
void ReplaceChars(std::string &sTarget, const std::string &sChars, const std::string &sReplacement); //each occ of each char from chars with rep
const std::string SanitizeName(const std::string &sp); //replaces non-alphanums with underscores
//=====

///todo: utf8 upper/lower case...
const std::string ucase(const char *sz);
const std::string ucase(const std::string &s);
void toucase(std::string &s);
const std::string lcase(const char *sz);
const std::string lcase(const std::string &s);
void tolcase(std::string &s);

int scmp(const std::string &s1, const std::string &s2);
int sicmp(const std::string &s1, const std::string &s2);
bool seqs(const std::string &s1, const std::string &s2); //Strings case-sensitive EQuality-compare
bool sieqs(const std::string &s1, const std::string &s2); //Strings case-Insensitive EQuality-compare
bool scontain(const std::string &data, const std::string &fragment); //case-sensitive
bool sicontain(const std::string &data, const std::string &fragment); //not case-sensitive

inline bool is_digit(unsigned char c) { return ((c>='0')&&(c<='9')); }
inline bool is_letter(unsigned char c) { return (((c>='A')&&(c<='Z'))||((c>='a')&&(c<='z'))); }
inline bool is_ascii(unsigned char c) { return (c<=127); }

//see also 'tohex'-template below
bool hextoch(const std::string &shex, char &ch); //expect shex <- { "00", "01", ... , "fe", "ff" }
void fshex(const std::string &sraw, std::string &shex, int rawlen=0); //format rawlen(0=>all) chars to hex
const std::string ashex(const std::string &sraw); //format raw chars to "HE XH EX HE ..."
const std::string asbin(unsigned char u); //0's& 1's...

const std::string esc_quotes(const std::string &s);
const std::string unesc_quotes(const std::string &s);

//renamed from ..s to s.. without bothering to check usages - LOLMFAO!
const std::string schop(const std::string &s, int nmaxlen, bool bdots=true); //appends ".." to string
const std::string spad(const std::string &s, int nmaxlen, char c=' ', bool bfront=true, int tabsize=4);
//fixed-Column-Tab-positions..
const std::string spadct(const std::string &s, int nmaxlen, char c=' ', bool bfront=true, int tabcols=4, int startpos=0);

/// use one function to set fixed-length string, cater for dots front, mid, back, padding front/back, centering ...
enum { FIT_NONE=0, FIT_L=1, FIT_M=2, FIT_R=4, FIT_DOT=8, FIT_PAD=32, };
const std::string sfit(const std::string &s, int nmaxlen, int fitbit=FIT_NONE, char c=' ');

const std::string slcut(std::string &S, size_t n); //resizes S, returns cut from front
const std::string srcut(std::string &S, size_t n); //resizes S, returns cut from back

//--------------------------------------------------------------------------------------------------
//simple encryption
void encs(const std::string &raw, std::string &enc, const std::string &pw="");
void decs(const std::string &enc, std::string &raw, const std::string &pw="");
const std::string encs(const std::string &raw, const std::string &pw="");
const std::string decs(const std::string &enc, const std::string &pw="");

//--------------------------------------------------------------------------------------------------
size_t splitslist(const std::string &list, char delim, std::vector<std::string> &vs, bool bIncEmpty=true);
size_t splitsslist(const std::string &list, const std::string &delim, std::vector<std::string> &vs, bool bIncEmpty=true);
//size_t splitqslist(const std::string &list, char delim, std::vector<std::string> &vs, bool bIncEmpty); todo...
void splitslen(const std::string &src, size_t len, std::vector<std::string> &vs); //copy len-sized substrings from s to vs
void splitslr(std::string s, char cdiv, std::string &l, std::string &r); //split s on first cdiv into l & r
void splitslr(std::string s, std::string sdiv, std::string &l, std::string &r); //split s on first sdiv into l & r

//--------------------------------------------------------------------------------------------------
typedef std::map<std::string, std::string> SystemEnvironment; //[key-name]=value
bool GetSystemEnvironment(SystemEnvironment &SE);

//--------------------------------------------------------------------------------------------------
const std::string to_sKMGT(size_t n); //1024, Kilo/Mega/Giga/Tera/Peta/Exa/Zetta/Yotta

//--------------------------------------------------------------------------------------------------filesystem-related
const std::string filesys_error(); //applies to bool functions below

//enum FSITEMTYPE { TN_UNKNOWN=0, TN_DIR=1, TN_REGFILE, TN_PIPE, TN_LINK, TN_BLOCKDEV, TN_CHARDEV, TN_SOCKET }; //use with gettype(..)
typedef std::map<std::string, FSYS::file_type> DirEntries;
struct DirTree : public std::map<std::string, DirTree> //[RELATIVE!!-to-dirpath-name]=..whatever-it-contains..
{
	/*
		DTree[sub_path_name] = (sub-path_subdir_names-DTree)
		 - full_sub_path
		 - ([file_name]=type)*
	*/
	std::string dirpath;
	DirEntries content; //files etc (not subdirs)
	
	void rec_read(const std::string &sdir, DirTree &dt); //recursive read dir
	DirTree();
	virtual ~DirTree();
	bool Read(const std::string &sdir);
//	bool Prune_Dir(const std::string &dirname); //Prune_.. only from tree, NOT from directory
//	bool Prune_File(const std::string &filename);
};

//path..
const std::string path_append(const std::string &spath, const std::string &sapp); // (/a/b/c, d) => /a/b/c/d
bool path_realize(const std::string &sfullpath); //full path only, a file-name will create dir with that name!
const std::string path_path(const std::string &spath); //excludes last name (/a/b/c/d => /a/b/c)
const std::string path_name(const std::string &spath); //only last name (/a/b/c/d => d)
std::string path_time(const std::string &spath); //modified date-time: return format=>"yyyymmddHHMMSS"
std::string path_time_h(const std::string &spath); //human-readable modified date-time: return format=>"yyyy-mm-dd HH:MM:SS"
bool path_max_free(const std::string spath, size_t &max, size_t &free); //space on device on which path exist

//utility..
std::string getcurpath(); //current working directory
//int gettype(FSITEMTYPE tn);
//int getitemtype(FSYS::file_type ft);
FSYS::file_type getfsfiletype(const std::string &se);
const std::string gettypename(FSYS::file_type ft, bool bshort=false);
bool isdirtype(FSYS::file_type ft);
bool isfiletype(FSYS::file_type ft); //regular file
bool issymlinktype(FSYS::file_type ft);
//ispipetype..
//issockettype..
//isblockdevicetype..
//ischardevicetype..
bool fsexist(const std::string &sfs);
bool issubdir(const std::string &Sub, const std::string &Dir); //is S a subdir of D
bool isdirempty(const std::string &D); //also false if D does not exist, or cannot be read
size_t fssize(const std::string &sfs);
//bool getfulltree(const std::string &srootdir, std::map<std::string, int> &tree);
bool realizetree(const std::string &sdir, DirTree &tree); //creates dirs-only at spath
const std::string getrelativepathname(const std::string &sroot, const std::string &spath); //empty if spath not subdir of sroot
const std::string getsymlinktarget(const std::string &slnk);

//todo... check libmagic(3)...
bool isexe(const std::string sF); //check if sF is an executable (.EXE or elf-binary)
bool isscript(const std::string sF); //bash, perl, (checks #!)
bool isnontextfile(const std::string sF); //checks if file contains a null-byte
inline bool isbinaryfile(std::string sf) { return isnontextfile(sf); }
inline bool istextfile(std::string sf) { return !isbinaryfile(sf); }
bool ispicture(std::string sf); //jpg/png/...
bool issound(std::string sf); //mp3/...
bool isvideo(std::string sf); //mpg/...
bool isarchive(std::string sf); //zip/gz/bz2/...
bool ispdf(std::string sf); //pdf
bool isdocument(std::string sf); //doc/odf/ods/...
bool isdatabase(std::string sf); //sqlite/xml/...
bool issourcecode(std::string sf); //c/cpp/h/...
bool iswebfile(std::string sf); //htm[l]/css/


DTStamp get_dtmodified(const std::string sN);
const std::string get_owner(const std::string sN);

//permissions & rights..
int getpermissions(const std::string sN); //for object (owner,group,others)
bool setpermissions(const std::string sN, int prms, bool bDeep=false); //deep=>recursively if sN is a directory
//set_uid
//set_gid
//set_sticky

std::string getfullrights(const std::string sfd); //ogo
int getrights(const std::string sN); //for current user on object
bool canread(const std::string sN);
bool canwrite(const std::string sN);
bool canexecute(const std::string sN);

const std::string getrightsRWX(size_t r);
const std::string getrightsRWX(const std::string sN);

//dir..
bool dir_exist(const std::string &sdir);
inline bool isdir(const std::string &sdir) { return dir_exist(sdir); }
inline const std::string dir_path(const std::string &sdpath) { return path_path(sdpath); }
inline const std::string dir_name(const std::string &sdpath) { return path_name(sdpath); }
bool dir_create(const std::string &sdir);
bool dir_delete(const std::string &sdir);

bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly=false); //deep; bNamesOnly=>no file-sizes; true=>same

//bool dir_diff(const std::string &l, const std::string &r, bool bNamesOnly=false, std::map<std::string, int> *pdiffs); ...todo
	// pdiffs==[relative-path-to-entity]={0==access_error, 1==content_diff, 2==meta_diff, 3==not_exist_in_l, 4==not_exist_in_r}
		// enum COMPARE_STATUS { CS_ACCESS=0, CS_DIFF=1, CS_META, CS_NOTL, CS_NOTR, };
		// so that: l+relative-path-to-entity <-compared to-> r+relative-path-to-entity
	// root paths of l and r not compared, only sub-entities
	

bool dir_read(const std::string &sdir, DirEntries &content); //local entries only, excl subdir-content, [name(not path)]=type
bool dir_read_deep(const std::string &sdir, DirTree &dtree); //everything
bool dir_rename(const std::string &sdir, const std::string &sname);
bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackup=true);
bool dir_sync(const std::string &sourcedir, const std::string &syncdir); //copy only new & changed
bool dir_move(const std::string &sdir, const std::string &destdir, bool bBackup=true);
bool dir_backup(const std::string &sdir); //makes ".~nn~" copy of dir
size_t dir_size(const std::string sdir, bool bDeep=false); //deep-size (use with to_sKMGT(..) to display e.g.: "nn.nn MB")


//bool dir_sync(const std::string &sdir, const std::string &destrootdir); //sdir treated as subdir of destrootdir ...todo:
	// destrootdir updated with content of sdir after sync

//bool dir_mirror(const std::string &l, const std::string &r); // ...todo:
	// both l and r will be mutually updated to be exact same after call
	// equivalent to: dir_sync(l, r); dir_sync(r, l);


//file..
bool file_exist(const std::string &sfile);
bool isfile(const std::string &sfile); //{ return file_exist(sfile); }
inline const std::string file_path(const std::string &sfpath) { return path_path(sfpath); } //(/a/b/c.d) -> /a/b
inline const std::string file_name(const std::string &sfpath) { return path_name(sfpath); } //(/a/b/c.d) -> c.d
void file_name_ext(const std::string &sfpath, std::string &name, std::string &ext);
std::string file_name_noext(const std::string &sfpath); //(/a/b/c.d) -> c
//std::string file_extention(const std::string &sfpath); //(/a/b/c.d) -> d
std::string file_extension(const std::string &sfpath); //(/a/b/c.d) -> d
bool file_delete(const std::string &sfile);
bool file_read(const std::string &sfile, std::string &sdata);
bool file_compare(const std::string &l, const std::string &r); //true=>same
bool file_write(const std::string &sfile, const std::string &sdata);
bool file_append(const std::string &sfile, const std::string &sdata);
bool file_overwrite(const std::string &sfile, const std::string &sdata);
bool file_rename(const std::string &sfile, const std::string &sname);
bool file_copy(const std::string &sfile, const std::string &dest, bool bBackup=true);
bool file_move(const std::string &sfile, const std::string &dest, bool bBackup=true);
bool file_backup(std::string sfile); //makes ".~nn~" copy of file
size_t file_size(const std::string &sfile);
//const std::string file_extension(const std::string &sfile);
bool file_crc32(const std::string &sfile, uint32_t &crc);

bool findfile(std::string sdir, std::string sfile, std::string &sfound);

bool isencrypted(std::string sf);

//--------------------------------------------------------------------------------------------------
template<typename T> bool is_numeric(T t)
{
	return (std::is_integral<decltype(t)>{}||std::is_floating_point<decltype(t)>{});
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

//NB: use absolute value of N-param...
//template<typename N>const std::string tohex(N NB_UseAbsoluteValue, unsigned int leftpadtosize=1, char pad='0')
//...concepts...
template<typename N>const std::string tohex(N n, unsigned int leftpadtosize=1, char pad='0')
{
	if (!is_numeric(n)) return "\0";
	std::string H="";
	size_t U=size_t(n);
	while (U) { H.insert(0, 1, "0123456789ABCDEF"[(U&0xf)]); U>>=4; }
	while (H.size()<leftpadtosize) H.insert(0, 1, pad);
	return H;
}

template<typename T> const std::string ttos(const T &v) //TypeTOString
{
	//if streamable(T)
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template<typename T> const T stot(const std::string &str) //StringTOType
{
	//if streamable(T)
    std::istringstream iss(str);
    T ret;
    iss >> ret;
    return ret;
}

template<typename T> void ensv(const std::vector<T> &v, char delim, std::string &list, bool bIncEmpty=true) //en-[delimiter-]separated-values
{
	list.clear();
	std::ostringstream oss("");
	for (auto t:v) { if (!oss.str().empty()) oss << delim; oss << t; }
	list=oss.str();
}

template<typename T> size_t desv(const std::string &list, char delim, std::vector<T> &v, bool bIncEmpty=true) //de-[delimiter-]separated-values
{
	std::istringstream iss(list);
	std::string s;
	v.clear();
	auto stt=[](const std::string &str)->const T{ std::istringstream ss(str); T t; ss >> t; return t; };
	while (std::getline(iss, s, delim))
	{
		TRIM(s);
		if (!s.empty()||bIncEmpty) v.push_back(stt(s));
	}
	return v.size();
}

///...parse for quoted strings ...todo


template<typename T=std::string> const std::string vtos(const std::vector<T> &v, char delim=' ')//; //vector to string entries delimited by delim(default space)
{
	std::string list("");
	ensv<T>(v, delim, list, false);
	return list;
}

template<typename T=std::string> std::vector<T> stov(const std::string &list, char delim=' ')//; //reverses vtos()
{
	std::vector<T> v;
	desv<T>(list, delim, v, false);
	return v;
}

template<typename T> struct Stack : private std::vector<T> //LIFO
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

enum //character-type
{
	CT_NULL=0,
	CT_SPACE,		// (space) 32
	CT_TAB,			// (tab) 9
	CT_LF,			// (line-feed) 10
	CT_CR,			// (carriage-return) 13
	CT_COMMA,		// ,
	CT_PERIOD,		// .
	CT_COLON,		// :
	CT_SEMICOLON,	// ;
	CT_DASH,		// -
	CT_BAR,			// |
	CT_FSLASH,		// /
	CT_BSLASH,		// (backslash) 92
	CT_ULINE,		// _
	CT_SQUOTE,		// '
	CT_DQUOTE,		// "
	CT_BQUOTE,		// `
	CT_OCURVE,		// (
	CT_CCURVE,		// )
	CT_OSTAPLE,		// [
	CT_CSTAPLE,		// ]
	CT_OBRACE,		// {
	CT_CBRACE,		// }
};

template<typename T> struct Token
{
	int Type; //token-type
	T Tok;
	TokPos P;
	void clear() { Type=CT_NULL; Tok={}; P.clear(); }
	Token() : Type(CT_NULL), Tok({}), P({}) {}
	Token(const T &tok, TokPos p, int type) : Type(type), Tok(tok), P(p) {}
	Token(const T &tok, int x, int y, int type) : Type(type), Tok(tok), P(x,y) {}
	Token(const Token &token) { *this=token; }
	Token& operator=(const Token &token) { Type=token.Type; Tok=token.Tok; P=token.P; return *this; }
	friend std::ostream& operator<<(std::ostream &os, const Token<T> &token) { os << token.Tok; return os; }
};

using AToken=Token<std::string>;
//using UToken=Token<utf8string>;

template<typename T> struct Tokens
{
private:
	std::vector<T> vtoks;
public:
	using iterator=typename std::vector<T>::iterator;
	using const_iterator=typename std::vector<T>::const_iterator;
	
	iterator begin() { return vtoks.begin(); }
	iterator end() { return vtoks.end(); }
	const_iterator begin() const { return vtoks.begin(); }
	const_iterator end() const { return vtoks.end(); }
	
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
	
	bool next(T &t)
	{
		t.clear();
		if (index<size()) { t=vtoks.at(index); index++; return true; }
		return false;
	}

	bool peeknext(T &t, size_t dist) const
	{
		t.clear();
		size_t idx=(index+dist);
		if (idx<size()) { t=vtoks.at(idx); return true; }
		return false;
	}

	bool isvalat(const std::string val, size_t dist) const
	{
		size_t idx=(index+dist);
		if (idx<size()) return (sieqs(val, vtoks.at(idx).Tok)==0);
		return false;
	}

	bool istypeat(int type, size_t dist) const
	{
		size_t idx=(index+dist);
		if (idx<size()) return (vtoks.at(idx).Type==type);
		return false;
	}
};

using ATokens=Tokens<AToken>;
//using UTokens=Tokens<UToken>;

template<typename C> bool IsNoise(C c)		{ return ((c==32)||(c==9)||(c==10)||(c==13)); }
template<typename C> bool IsSpace(C c)		{ return (c==32); }
template<typename C> bool IsTab(C c)		{ return (c==9); }
template<typename C> bool IsNewline(C c)	{ return ((c==10)||(c==13)); }
template<typename C> bool IsQuote(C c)		{ return ((c=='"')||(c=='\'')||(c=='`')); }
template<typename C> bool IsDigit(C c)		{ return ((c>='0')&&(c<='9')); }
template<typename C> bool IsAlpha(C c)		{ return ((c=='_')||((c>='A')&&(c<='Z'))||((c>='a')&&(c<='z'))); } //underscore assumed to be alpha
template<typename C> bool IsAlphaNum(C c)	{ return (IsAlpha(c)||IsDigit(c)); }
template<typename C> bool IsBelowASCII(C c)	{ return (c<32); }
template<typename C> bool IsASCII(C c)		{ return ((c>=32)&&(c<=127)); }
template<typename C> bool IsAboveASCII(C c)	{ return (c>127); }
template<typename C> bool IsOtherASCII(C c)	{ return (IsASCII(c)&&!IsAlphaNum(c)); }


bool IsInteger(const std::string &s);
bool IsDecimal(const std::string &s);

enum //WHITE_SPACE_DISPOSITION === space/tab/lf/cr/?
{
	WSD_KEEP=0, //keep all
	WSD_DISCARD, //discard all
	WSD_REDUCE, //reduce to single space
};

template<typename CHAR=char, typename STRING=std::string, typename TOKEN=Token<STRING>, typename TOKENLIST=Tokens<TOKEN> >
 size_t tokenize(TOKENLIST &TL, const STRING &S, int wsdisp=WSD_REDUCE, bool bHasQuotedStrings=false)
{
	if (!S.size()) return 0;
	STRING t;
	CHAR c;//, p, cq;
	size_t pos=0, line=1, col=1, X, Y;
	auto Get=[&pos, &S](CHAR &u)->bool{ if (pos<S.size()) { u=S.at(pos++); return true; } return false; };
	//auto UnGet=[&pos](){ --pos; };
	auto Peek=[&pos, &S](CHAR &u)->bool{ if (pos<S.size()) { u=S.at(pos); return true; } return false; };

	TL.clear();
	while (Get(c))
	{
		col++;
		//--------------------
		if (IsNoise(c))
		{
			if (!t.empty()) TL.push(TOKEN(t, (col-t.length()-1), line, TT_TEXT));
			if (IsNewline(c)) { line++; col=1; } else col++;
			X=col-1; Y=line;
			switch (wsdisp)
			{
				case WSD_REDUCE: //..to a single space
					{
						while (Peek(c)&&IsNoise(c)) { Get(c); if (IsNewline(c)) { line++; col=1; } else col++; X=col-1; Y=line; }
						c=32;
					} //fall-thru
				case WSD_KEEP:
					{
						t=c;
						TL.push(TOKEN(t, X, Y, (wsdisp==WSD_REDUCE)?TT_SCHAR:TT_NOISE));
					} break;
			}
			t.clear();
		}
		//--------------------
		else if (IsOtherASCII(c))
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
	
//scan for quotes etc here...
//----------quotes need too be handled elsewhere----------------
// to scan thru vtoks for matching quotes, & to find apostrophes, misplaced quotes, etc
// maybe: use temp list to collect on first pass above, then filter the temp-list for quotes
// & fill the param-list with tokens for valid quoted strings, and apostrophe-words
//
			if (IsQuote(c))
			{
				if (bHasQuotedStrings)
				{
//					p=0; cq=c;
//					ins.get(c); col++;
//					if (Get(c)) t+=c;
//					while (ins.good() && ((c!=cq) || (p=='\\')))
//					{
//						if (IsNewline(c)) { line++; col=1; } //don't apply wsdisp
//						p=c; //to check for escaped (\) quote
//						ins.get(c); col++;
//						if (ins.good()) t+=c;
//					}
//					TL.push_back(TToken(t, X, Y, TT_QSTR)); //quotes included
				}
//				else TL.push_back(TToken(t, X, Y, TT_SCHAR));
//				t="";
			}
//--------------------------------------------------------------

	
	return TL.size();
}


//===================================================================================================
//NB: add compiler flag -fconcepts
template<typename T> concept bool CpCon() { return std::is_copy_constructible<T>::value; }
template<typename T> concept bool IsCon() { return (std::is_convertible<T, std::string>::value || std::is_pod<T>::value); }

template<CpCon T, IsCon U=std::string> struct Share
{
	typedef T* Share_Type;
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



#endif
