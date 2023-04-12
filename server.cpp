#include <iostream>
#include <string>
#include <queue>
#include <chrono>
#include <algorithm>
#include <regex>
#include <vector>
#include <set>

#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON

#include <ctime>
#include <deque>

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// #pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;

const string& WHITESPACE = " \n\r\t\f\v";


template <typename T>
ostream& operator<<(ostream & os, const vector<T>& vec)
{
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        os<<*it<<" ";
    }
    return os;
}

string ltrim(const string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return start==string::npos ? "" : s.substr(start);
}
string rtrim(const string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return end==string::npos ? "" : s.substr(0,end+1);
}
string trim(const string& s)
{
    return ltrim(rtrim(s));
}

string remove_duplicates(const string& str)
{
    string res("");
    for (auto c = str.cbegin(); c!=str.cend(); ++c) {
        bool add = true;
        for (auto old = res.cbegin(); old!=res.cend(); ++old) {
            if (*old == *c) {
                add = false;
                break;
            }
        }
        if (add) res.push_back(*c);
    }

    return res;
}

#define REGEX_CLASS_CONCRETE(name,strname,regex,is_what) \
class name : public Concrete                             \
{                                                        \
public:                                                  \
    name() : Concrete() {is_what = true;}                \
    name* clone() const override {                       \
        return new name(*this);                          \
    }                                                    \
    operator string() const override {                   \
        return "<" strname ">";                          \
    }                                                    \
    string toRegex() const override {                    \
        return regex;                                    \
    }                                                    \
    string toRegexDoubleEscape() const override {        \
        return regex;                                    \
    }                                                    \
}

#define REGEX_CLASS_UNARY(name,strname,before,after)      \
class name : public Unary                                 \
{                                                         \
public:                                                   \
    using Unary::Unary;                                   \
    name* clone() const override {                        \
        return new name(*this);                           \
    }                                                     \
    operator string() const override {                    \
        if (e!=nullptr)                                   \
            return strname "("+string(*e)+")";            \
        return strname "(?)";                             \
    }                                                     \
    string toRegex() const override {                     \
        if (e==nullptr) return "(" before "()" after ")"; \
        return "(" before+e->toRegex()+after ")";         \
    }                                                     \
    string toRegexDoubleEscape() const override {                     \
        if (e==nullptr) return "(" before "()" after ")"; \
        return "(" before+e->toRegexDoubleEscape()+after ")";         \
    }                                                     \
}

#define REGEX_CLASS_BINARY(name,strname,delim)          \
class name : public Binary                              \
{                                                       \
public:                                                 \
    using Binary::Binary;                               \
    name* clone() const override {                      \
        return new name(*this);                         \
    }                                                   \
    operator string() const override {                  \
        string e1_string = string("?");                 \
        if (e1!=nullptr)  {                             \
            e1_string = string(*e1);                    \
        }                                               \
        string e2_string = string("?");                 \
        if (e2!=nullptr)  {                             \
            e2_string = string(*e2);                    \
        }                                               \
        return strname "("+e1_string+","+e2_string+")"; \
    }                                                   \
    string toRegex() const override {                   \
        string e1_regex = string("()");                 \
        if (e1!=nullptr)  {                             \
            e1_regex = e1->toRegex();                   \
        }                                               \
        string e2_regex = string("()");                 \
        if (e2!=nullptr)  {                             \
            e2_regex = e2->toRegex();                   \
        }                                               \
        return "("+e1_regex+delim+e2_regex+")";         \
    }                                                   \
    string toRegexDoubleEscape() const override {       \
        string e1_regex = string("()");                 \
        if (e1!=nullptr)  {                             \
            e1_regex = e1->toRegexDoubleEscape();                   \
        }                                               \
        string e2_regex = string("()");                 \
        if (e2!=nullptr)  {                             \
            e2_regex = e2->toRegexDoubleEscape();                   \
        }                                               \
        return "("+e1_regex+delim+e2_regex+")";         \
    }                                                   \
}

#define REGEX_CLASS_SPECIFIC(name,is_what)             \
class name : public SpecificChar                       \
{                                                      \
public:                                                \
    name(string c) : SpecificChar(c) {is_what = true;} \
    name(char c) : SpecificChar(c) {is_what = true;}   \
    name* clone() const override {                     \
        return new name(*this);                        \
    }                                                  \
}


class Regex
{
public:
    bool isConcrete;
    int closest_leaf;
    bool isNum;
    bool isLet;
    bool isLow;
    bool isCap;
    int bonus;
    int total;
    int hight;
    Regex(int bonus =0) : isConcrete(false), closest_leaf(0), isNum(false), isLet(false), isLow(false), isCap(false) ,bonus(bonus),total(bonus),hight(0){}
    virtual ~Regex() = default;
    Regex(const Regex& other) = default;
    Regex& operator=(const Regex& other) = delete;
    virtual Regex* clone() const = 0;
bool operator<(const Regex& other) const {
    if (hight == other.hight) {
        return total < other.total;
    }
    return hight > other.hight;
}
    virtual bool setClosestLeaf(Regex* token) = 0;
    virtual operator string() const = 0;
    virtual string toRegex() const = 0;
    virtual string toRegexDoubleEscape() const = 0;
    virtual const Regex* getParentOfNextToken() const = 0;
};

ostream& operator<<(ostream & os, const vector<Regex*>& vec)
{
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        os<<string(**it)<<" ";
    }
    return os;
}

class Concrete : public Regex
{
public:
    Concrete(int bonus =0) : Regex(bonus) {
        isConcrete = true;
    }
    ~Concrete() override = default;
    Concrete(const Concrete& other) = default;
    Concrete& operator=(const Concrete& other) = delete;

    bool setClosestLeaf(Regex* token) override {
        return false;
    }
    const Regex* getParentOfNextToken() const override {
        return this;
    }
};

REGEX_CLASS_CONCRETE(Any,"any",".",isConcrete);
REGEX_CLASS_CONCRETE(Num,"num","[0-9]",isNum);
REGEX_CLASS_CONCRETE(Let,"let","[a-zA-Z]",isLet);
REGEX_CLASS_CONCRETE(Low,"low","[a-z]",isLow);
REGEX_CLASS_CONCRETE(Cap,"cap","[A-Z]",isCap);

class SpecificChar : public Concrete {
public:
    string c;

    SpecificChar(string c,int bonus =0) : Concrete(bonus) ,c(c) {}
    SpecificChar(char c,int bonus = 0) : Concrete(bonus) ,c(string(1,c)) {}
    virtual SpecificChar* clone() const override {
        return new SpecificChar(*this);
    }
    operator string() const override {
        return "<"+c+">";
    }
    string toRegex() const override {
        if (string("#S&()*+-.?[]\\^{}|~").find(c[0]) != string::npos) return "\\"+c;
        return c;
    }
    string toRegexDoubleEscape() const override {
        if (c[0]=='\\') return "\\\\\\\\";
        if (string("#S&()*+-.?[]^{}|~").find(c[0]) != string::npos) return "\\\\"+c;
        return c;
    }
};

REGEX_CLASS_SPECIFIC(SpecificNum,isNum);
REGEX_CLASS_SPECIFIC(SpecificLet,isLet);
REGEX_CLASS_SPECIFIC(SpecificLow,isLow);
REGEX_CLASS_SPECIFIC(SpecificCap,isCap);

SpecificChar* createSpecificChar(char c)
{
    if ('0'<=c && c<='9') return new SpecificNum(c);
    else if ('a'<=c && c<='z') return new SpecificLow(c);
    else if ('A'<=c && c<='Z') return new SpecificCap(c);
    else return new SpecificChar(c);
}

class Unary : public Regex
{
public:
    Regex* e;

    Unary(Regex* e = nullptr,int bonus =0 ) : Regex(bonus) {
        if (e!=nullptr) {
            isConcrete = e->isConcrete;
            hight = e->hight+1;
            total += e->total;
            closest_leaf = e->closest_leaf + 1;
        } else {
            // isConcrete = false;
            closest_leaf = 1;
        }
        Unary::e = e;
    }
    ~Unary() override {
        if (e!=nullptr) {
            delete e;
        }
    }
    Unary(const Unary& other) : Regex(other) {
        e = other.e==nullptr ? nullptr : other.e->clone();
    }
    Unary& operator=(const Unary& other) = delete;

    bool setClosestLeaf(Regex* token) override {
        bool ischanged = false;
        if (e==nullptr) {
            e = token;
            total += e->total;
            hight = e->hight+1;
            closest_leaf = token->closest_leaf + 1;
            isConcrete = token->isConcrete;
            ischanged = true;
        } else if (!e->isConcrete) {
            total -= e->total;
            ischanged = e->setClosestLeaf(token);
            total += e->total;
            hight = e->hight+1;
            closest_leaf = e->closest_leaf + 1;
            isConcrete = e->isConcrete;
        }
        return ischanged;
    }
    const Regex* getParentOfNextToken() const override {
        if (e==nullptr) {
            return this;
        }
        return e->getParentOfNextToken();
    }
};

REGEX_CLASS_UNARY(Startwith,"startwith","",".*");
REGEX_CLASS_UNARY(Endwith,"endwith",".*","");
REGEX_CLASS_UNARY(Contain,"contain",".*",".*");
REGEX_CLASS_UNARY(Optional,"optional","","?");
REGEX_CLASS_UNARY(Star,"star","","*");


class Binary : public Regex
{
public:
    Regex* e1;
    Regex* e2;

    Binary(Regex* e1 = nullptr, Regex* e2 = nullptr, int bonus= 0) : Regex(bonus) {
        if (e1!=nullptr) {
            isConcrete = e1->isConcrete;
            closest_leaf = e1->closest_leaf + 1;
            total+= e1->total;
            hight = e1->hight+1;
        } else {
            // isConcrete = false;
            closest_leaf = 1;
        }
        if (e2!=nullptr) {
            total += e2->total;
            hight = max(hight,e2->hight+1);
            isConcrete = isConcrete && e2->isConcrete;
            closest_leaf = min(closest_leaf,e2->closest_leaf+1);
        } else {
            isConcrete = false;
            closest_leaf = 1;
        }
        Binary::e1 = e1;
        Binary::e2 = e2;
    }
    ~Binary() override {
        if (e1!=nullptr) {
            delete e1;
        }
        if (e2!=nullptr) {
            delete e2;
        }
    }
    Binary(const Binary& other) : Regex(other) {
        e1 = other.e1==nullptr ? nullptr : other.e1->clone();
        e2 = other.e2==nullptr ? nullptr : other.e2->clone();
    }
    Binary& operator=(const Binary& other) = delete;

    virtual bool setClosestLeaf(Regex* token) override {
        bool ischanged = false;
        if (e1==nullptr) {
            e1 = token;
            ischanged = true;
            hight = max(e1->hight+1,hight);
            total += e1->total;
        } else if (e2==nullptr) {
            e2 = token;
            hight = max(e2->hight+1,hight);
            ischanged = true;
            total += e2->total;
        } else {
            if (e1->isConcrete) {
                if (e2->isConcrete) {
                    return false;
                } else {
                    total -= e2->total;
                    ischanged = e2->setClosestLeaf(token);
                    total += e2->total;
                    hight = max(e2->hight+1,hight);
                }
            } else {
                if (e2->isConcrete) {
                    total -= e1->total;
                    ischanged = e1->setClosestLeaf(token);
                    total += e1->total;
                    hight = max(e1->hight+1,hight);
                } else {
                    if (e1->closest_leaf<=e2->closest_leaf) {
                        total -= e1->total;

                        ischanged = e1->setClosestLeaf(token);
                        total += e1->total;
                        hight = max(e1->hight+1,hight);
                    } else {
                        total -= e2->total;
                        ischanged = e2->setClosestLeaf(token);
                        total += e2->total;
                        hight = max(e2->hight+1,hight);
                    }
                }
            }
        }

        if (e2==nullptr) {
            // isConcrete = false;
            // closest_leaf = 1;
        } else {
            isConcrete = e1->isConcrete && e2->isConcrete;
            closest_leaf = !e1->isConcrete ? e1->closest_leaf+1 : e2->closest_leaf+1; //temporary value
            closest_leaf = !e2->isConcrete ? min(closest_leaf,e2->closest_leaf+1) : closest_leaf; //final value
        }

        return ischanged;
    }
    const Regex* getParentOfNextToken() const override {
        if (e1==nullptr) {
            return this;
        } else if (e2==nullptr) {
            return this;
        } else {
            if (e1->isConcrete) {
                if (e2->isConcrete) {
                    return this;
                } else {
                    return e2->getParentOfNextToken();
                }
            } else {
                if (e2->isConcrete) {
                    return e1->getParentOfNextToken();
                } else {
                    if (e1->closest_leaf<=e2->closest_leaf) {
                        return e1->getParentOfNextToken();
                    } else {
                        return e2->getParentOfNextToken();
                    }
                }
            }
        }
    }
};

REGEX_CLASS_BINARY(Concat,"concat","");

class Or : public Binary
{
public:
    Or(Regex* e1 = nullptr, Regex* e2 = nullptr,int bonus=0) : Binary(e1,e2,bonus) {
        if (e1!=nullptr) {
            isNum = e1->isNum;
            isLow = e1->isLow;
            isCap = e1->isCap;
        } else {
            // isNum = false;
            // isLow = false;
            // isCap = false;
        }
        if (e2!=nullptr) {
            isNum = isNum && e2->isNum;
            isLow = isLow && e2->isLow;
            isCap = isCap && e2->isCap;
        } else {
            isNum = false;
            isLow = false;
            isCap = false;
        }
    }
    bool setClosestLeaf(Regex* token) override {
        if (Binary::setClosestLeaf(token)==false) {
            return false;
        }

        if (e2==nullptr) {
            // isNum = false;
            // isLow = false;
            // isCap = false;
        } else {
            isNum = e1->isNum && e2->isNum;
            isLow = e1->isLow && e2->isLow;
            isCap = e1->isCap && e2->isCap;
        }
        return true;
    }

    Or *clone() const override {
        return new Or(*this);
    }
    operator string() const override {
        string e1_string = string("?");
        if (e1 != nullptr) {
            e1_string = string(*e1);
        }
        string e2_string = string("?");
        if (e2 != nullptr) {
            e2_string = string(*e2);
        }
        return "or("+e1_string+","+e2_string+")";
    }
    string toRegex() const override {
        string e1_regex = string("()");
        if (e1 != nullptr) {
            e1_regex = e1->toRegex();
        }
        string e2_regex = string("()");
        if (e2 != nullptr) {
            e2_regex = e2->toRegex();
        }
        return "("+e1_regex+"|"+e2_regex+")";
    }
    string toRegexDoubleEscape() const override {
        string e1_regex = string("()");
        if (e1 != nullptr) {
            e1_regex = e1->toRegexDoubleEscape();
        }
        string e2_regex = string("()");
        if (e2 != nullptr) {
            e2_regex = e2->toRegexDoubleEscape();
        }
        return "("+e1_regex+"|"+e2_regex+")";
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void deleteContainer(vector<Regex*> vec)
{
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        delete *it;
    }
    vec.clear();
}
void deleteContainer(queue<Regex*> que)
{
    while (que.size()>0) {
        delete que.front();
        que.pop();
    }
}

template <typename T_1>
inline bool regex_instance_of(const Regex* const p)
{
    return  dynamic_cast<const T_1*>(p)!=nullptr;
}
template <typename T_1, typename T_2, typename ...T_rest>
inline bool regex_instance_of(const Regex* const p)
{
    return regex_instance_of<T_1>(p) || regex_instance_of<T_2,T_rest...>(p);
}

template <typename T_1>
inline bool both_regexes_instance_of(const Regex* const p, const Regex* const token)
{
    return  regex_instance_of<T_1>(p) && regex_instance_of<T_1>(token);
}
template <typename T_1, typename T_2, typename ...T_rest>
inline bool both_regexes_instance_of(const Regex* const p, const Regex* const token)
{
    return both_regexes_instance_of<T_1>(p,token) || both_regexes_instance_of<T_2,T_rest...>(p,token);
}

Regex* stringToRegex(const string& str)//str is valid
{
    const string str_stripped = trim(str);

    //a
    //a( )
    //a( , )
    string root;
    Regex* e1 = nullptr;
    Regex* e2 = nullptr;

    int rparen_pos = str_stripped.find('(');
    if ((size_t)rparen_pos == string::npos) {//concrete
        root = str_stripped;
    } else {
        root = string(str_stripped.cbegin(),str_stripped.cbegin() + rparen_pos);
        auto iter = str_stripped.cbegin() + rparen_pos+1;
        int parens = 0;
        while(iter!=str_stripped.cend()) {
            if (parens==0 && *iter==',') break;
            else if (*iter=='(') ++parens;
            else if (*iter==')') --parens;
            ++iter;
        }
        if (iter==str_stripped.cend()) {//Unary
            e1 = stringToRegex(string(str_stripped.cbegin() + rparen_pos+1,str_stripped.cbegin()+str_stripped.size()-1));
        } else {//binary
            e1 = stringToRegex(string(str_stripped.cbegin() + rparen_pos+1,iter));
            e2 = stringToRegex(string(iter+1,str_stripped.cbegin()+str_stripped.size()-1));
        }
    }

    if (root.size()==1) return createSpecificChar(root[0]);
    else if (root=="num") return new Num();
    else if (root=="let") return new Let();
    else if (root=="low") return new Low();
    else if (root=="cap") return new Cap();
    else if (root=="any") return new Any();

    else if (root=="startwith") return new Startwith(e1);
    else if (root=="endwith") return new Endwith(e1);
    else if (root=="contain") return new Contain(e1);
    else if (root=="optional") return new Optional(e1);
    else if (root=="star") return new Star(e1);

    else if (root=="concat") return new Concat(e1,e2);
    else if (root=="or") return new Or(e1,e2);

    return nullptr;
}

template<typename T>
inline void token_push_concrete_if_needed(vector<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = literal.cbegin(); iter!=literal.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = general.cbegin(); iter!=general.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push_back(ptr);
}
template<typename T>
inline void token_push_unary_if_needed(vector<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Unary*>(*iter)->e==nullptr) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Unary*>(*iter)->e==nullptr) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push_back(ptr);
}
template<typename T>
inline void token_push_binary_if_needed(vector<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Binary*>(*iter)->e1==nullptr && static_cast<Binary*>(*iter)->e2==nullptr) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Binary*>(*iter)->e1==nullptr && static_cast<Binary*>(*iter)->e2==nullptr) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push_back(ptr);
}

bool insert_token_if_new(Regex* p, vector<Regex*>& tokens)
{
    for (auto it=tokens.begin(); it!=tokens.end(); ++it) {
        if (string(**it)==string(*p)) {
            return false;
        }
    }
    tokens.push_back(p->clone());
    return true;
}

queue<Regex*> set_tokens(const vector<string>& accept_examples,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    vector<Regex*> tokens;
    set<char> usedChars;
    cout <<"the tokens: ";
    for (auto iter = literal.cbegin(); iter!=literal.cend(); ++iter) {
        (*iter)->bonus = 1 ;
        (*iter)->total = 1 ;
        bool is_new = insert_token_if_new(*iter,tokens);
        if (is_new) {
cout << string(**iter)<<" ";
            if (regex_instance_of<SpecificChar>(*iter)) {
                usedChars.insert(static_cast<SpecificChar*>(*iter)->c[0]);
            }
        }
    }
    for (auto iter = general.cbegin(); iter!=general.cend(); ++iter) {
        (*iter)->bonus = 1 ;
        (*iter)->total = 1 ;
        bool is_new = insert_token_if_new(*iter,tokens);
        if (is_new) {
cout << string(**iter)<<" ";
            if (regex_instance_of<SpecificChar>(*iter)) {
                usedChars.insert(static_cast<SpecificChar*>(*iter)->c[0]);
            }
        }
    }
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        (*iter)->bonus = 1 ;
        (*iter)->total = 1 ;
        bool is_new = insert_token_if_new(*iter,tokens);
        if (is_new) {
cout << string(**iter)<<" ";
            if (regex_instance_of<SpecificChar>(*iter)) {
                usedChars.insert(static_cast<SpecificChar*>(*iter)->c[0]);
            }
        }
    }

    token_push_concrete_if_needed<Num>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Let>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Low>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Cap>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Any>(tokens,literal,general,include,exclude_node);

    string accept_example_chars = "";
    for (auto example_iter = accept_examples.cbegin(); example_iter!=accept_examples.cend(); ++example_iter) {
        for (auto char_iter = example_iter->cbegin(); char_iter!=example_iter->cend(); ++char_iter) {
            accept_example_chars.push_back(*char_iter);
        }
    }
    accept_example_chars = remove_duplicates(accept_example_chars);

    for (auto char_iter = accept_example_chars.cbegin(); char_iter!=accept_example_chars.cend(); ++char_iter) {
        if (usedChars.find(*char_iter) != usedChars.end()) continue;
        bool push = true;
        for (auto token_iter = exclude_node.cbegin(); token_iter!=exclude_node.cend(); ++token_iter) {
            if ((*token_iter)->toRegex() == string(1,*char_iter)) {
                push = false;
                break;
            }
        }
        if (!push) continue;

cout<<"<"<<*char_iter<<"> ";
        tokens.push_back(createSpecificChar(*char_iter));
    }

    token_push_unary_if_needed<Startwith>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Endwith>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Contain>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Optional>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Star>(tokens,literal,general,include,exclude_node);

    token_push_binary_if_needed<Concat>(tokens,literal,general,include,exclude_node);
    token_push_binary_if_needed<Or>(tokens,literal,general,include,exclude_node);

cout<<"\n";
    queue<Regex*> tokens_queue;
    for (auto it=tokens.begin(); it!=tokens.end(); ++it) {
        tokens_queue.push(*it);
    }
    return tokens_queue;
}

bool subtree_is_useless(const Regex* const parent, const Regex* const child1, const Regex* const child2 = nullptr)
{
    if (regex_instance_of<Startwith,Endwith,Contain>(parent)
         && regex_instance_of<Startwith,Endwith,Contain,Optional,Any>(child1)) {
        return true;
    }

    if (both_regexes_instance_of<Optional,Star>(parent,child1)) return true;

    if (both_regexes_instance_of<Concat,Or>(parent,child2)) return true;

    if (regex_instance_of<Or>(parent)
            && (regex_instance_of<Any>(child1) ||  regex_instance_of<Any>(child2))) {
        return true;
    }

    if (regex_instance_of<Or>(parent) && both_regexes_instance_of<Startwith,Endwith>(child1,child2)) return true;

    if (regex_instance_of<Or>(parent)) {
        if (both_regexes_instance_of<Concrete>(child1,child2)) {
            if (both_regexes_instance_of<SpecificChar>(child1,child2)
                    && static_cast<const SpecificChar*>(child1)->c>=static_cast<const SpecificChar*>(child2)->c) {
                return true;
            }
            if (both_regexes_instance_of<Num,Let,Low,Cap,Any>(child1,child2)) return true;
            if (regex_instance_of<Low,Cap>(child1) && regex_instance_of<Low,Cap>(child2)) return true;
        }
        if ((regex_instance_of<Num>(child1) && child2->isNum)
                || (child1->isNum && regex_instance_of<Num>(child2))) {
            return true;
        }
        if (child1->toRegex()==child2->toRegex()) return true;
    }

    return false;
}

bool skip_token_for_p(const Regex* const p, const Regex* const token)
{
    const Regex* parent = p->getParentOfNextToken();

    if (regex_instance_of<Unary>(parent)) {
        return subtree_is_useless(parent,token);
    }

    if (regex_instance_of<Binary>(parent)) {
        const Binary* p_binary = static_cast<const Binary*>(parent);
        if (p_binary->e1!=nullptr || p_binary->e2!=nullptr) {
            const Regex* child1 = p_binary->e1;
            const Regex* child2 = p_binary->e2;
            if (p_binary->e1!=nullptr) {
                child2 = token;
            } else {
                child1 = token;
            }

            return subtree_is_useless(parent,child1,child2);
        }
    }

    return false;
}

queue<Regex*> expand(const queue<Regex*>& tokens, Regex* p, const vector<Regex*>& exclude_tree, queue<Regex*>& worklist_concrete)
{
    queue<Regex*> tokens_copy(tokens);

    if (p==nullptr) {
        queue<Regex*> tokens_clone;
        while (tokens_copy.size()>0) {
            Regex* token = tokens_copy.front()->clone();
            tokens_copy.pop();
            if (token->isConcrete) worklist_concrete.push(token);
            else tokens_clone.push(token);
        }
        return tokens_clone;
    }

    queue<Regex*> children_partial;
    while (tokens_copy.size()>0) {
        Regex* token = tokens_copy.front()->clone();
        tokens_copy.pop();
        bool skip = skip_token_for_p(p,token);
        if (skip) {
            delete token;
            continue;
        }
        Regex* q = p->clone();
        q->setClosestLeaf(token);
        bool push = true;
        for (auto iter = exclude_tree.cbegin(); iter!=exclude_tree.cend(); ++iter) {
            if (q->toRegex().find((*iter)->toRegex()) != string::npos) {
                push = false;
                delete q;
                break;
            }
        }
        if (push) {
            if (q->isConcrete) worklist_concrete.push(q);
            else children_partial.push(q);
        }
    }

    delete p;
    return children_partial;
}

#undef REGEX_CLASS_CONCRETE
#undef REGEX_CLASS_UNARY
#undef REGEX_CLASS_BINARY
#undef REGEX_CLASS_SPECIFIC

bool algo1(SOCKET ClientSocket,const queue<Regex*>& tokens, const vector<string>& accept,
    const vector<string>& reject, const vector<Regex*>& exclude_tree)
{
    int numOfRegexes = 0;
    vector<string> finalRegexes;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    queue<Regex*> worklist;
    queue<Regex*> worklist_concrete;
    worklist.push(nullptr);
    while (worklist.size()>0 || worklist_concrete.size()>0) {
        while (worklist_concrete.size()>0) {
            Regex* p = worklist_concrete.front();
            worklist_concrete.pop();
            cout<<"p: "+string(*p)<<endl;
            cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
            cout<<"\t\t"+p->toRegex()<<endl;
            std::regex r(p->toRegex());
            bool acc = true;
            for (int i=0; (size_t)i<accept.size();++i) {
                if (!regex_match(accept[i],r)) {
                    acc = false;
                    break;
                }
            }
            if (acc) {
                bool rej = true;
                for (int i=0; (size_t)i<reject.size();++i) {
                    if (regex_match(reject[i],r)) {
                        rej = false;
                        break;
                    }
                }
                if (rej) {
                    string p_str(*p);
                    cout<<p_str<<" matches";
                    bool p_is_new = true;
                    for (auto it = finalRegexes.cbegin(); it!=finalRegexes.cend(); ++it) {
                        if (*it==p_str) {
                            p_is_new = false;
                            break;
                        }
                    }
                    if (!p_is_new) {
                        cout<<" but old"<<endl;
                        delete p;
                        continue;
                    }
                    cout<<" and new"<<endl;
                    cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
                    cout<<"\t\t"+p->toRegex()<<endl;
                    string sendJson = "{\""+to_string(numOfRegexes)+"\": {\"str\": \""+p_str+"\", \"regex\": \""+p->toRegexDoubleEscape()+"\"}}\n";
                    int iSendResult = send( ClientSocket, sendJson.c_str(), sendJson.size(), 0 );
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        delete p;
                        deleteContainer(worklist_concrete);
                        deleteContainer(worklist);
                        return false;
                    }
                    char* sent = new char[iSendResult+1];
                    sent[iSendResult] = '\0';
                    memcpy(sent, sendJson.c_str(), iSendResult);
                    cout<<"Bytes sent ("<<iSendResult<<"): "<<sent<<endl;
                    delete[] sent;
                    finalRegexes.push_back(p_str);
                    if (++numOfRegexes==5) {
                        delete p;
                        deleteContainer(worklist_concrete);
                        deleteContainer(worklist);
                        return true;
                    }
                }
            }
            delete p;
        }

        if (worklist.size()>0) {
            Regex* p = worklist.front();
            worklist.pop();
            queue<Regex*> worklist_ = expand(tokens,p,exclude_tree,worklist_concrete);
            while (worklist_.size() > 0) {
                worklist.push(worklist_.front());
                worklist_.pop();
            }
            vector<Regex*> vec(worklist.size());
            int i =0;
            while (!worklist.empty()) {
                vec[i] = worklist.front();
                worklist.pop();
                i++;
            }
            sort(vec.begin(), vec.end(), [](const Regex* a, const Regex* b) {
                return *b < *a;
                });

    for (auto it = vec.begin(); it != vec.end(); ++it) {
         worklist.push(*it);
        }
    }
}
return false;
}

int main()
{
    cout<<"server.cpp in main()"<<endl;
    WSADATA wsaData;
    int iResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    char recvbuf[DEFAULT_BUFLEN+1];
    memset(recvbuf,'\0',DEFAULT_BUFLEN+1);
    int recvbuflen = DEFAULT_BUFLEN;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    cout<<"server after WSAStartup"<<endl;
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    cout<<"server after getaddrinfo"<<endl;
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    cout<<"server after socket"<<endl;
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    cout<<"server after bind"<<endl;
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
while (true) {
    iResult = listen(ListenSocket, SOMAXCONN);
    cout<<"server after listen"<<endl;
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    ClientSocket = accept(ListenSocket, NULL, NULL);
    cout<<"server after accept"<<endl;
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    string message;
    char* json = nullptr;
    do {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        cout<<"server after recv"<<endl;
        if (iResult > 0) {
            cout<<"Bytes received ("<<iResult<<"): "<<recvbuf<<endl;
            message+=recvbuf;
            memset(recvbuf,'\0',iResult);
            json = new char[message.size()+1];
            json[message.size()] = '\0';
            json = strcpy(json,message.c_str());
            rapidjson::Document doc;
            if (!doc.ParseInsitu(json).HasParseError()) {
                delete[] json;
                json = nullptr;
                break;
            }
            delete[] json;
            json = nullptr;
        }
        else if (iResult == 0)
        cout<<"Closing connection with current client..."<<endl;
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
    } while (iResult > 0);

    cout<<"server received message: "<<message<<endl;

    json = new char[message.size()+1];
    json[message.size()] = '\0';
    json = strcpy(json,message.c_str());

    rapidjson::Document doc;
    cout << doc.ParseInsitu(json).HasParseError() << endl;

    cout << " include = [\"";
    for (rapidjson::Value::ConstValueIterator iter = doc["include"].Begin(); iter != doc["include"].End(); ++iter)
        cout << iter->GetString() << "\", \"";
    cout << "\"]";
    cout << " exclude = [\"";
    for (rapidjson::Value::ConstValueIterator iter = doc["exclude"].Begin(); iter != doc["exclude"].End(); ++iter)
        cout << iter->GetString() << "\", \"";
    cout << "\"]\n";

    vector<string> accept_examples;
    vector<string> reject_examples;

    vector<string> literal_str;
    vector<string> general_str;

    vector<string> include_Str;
    vector<string> exclude_Str;

    vector<Regex*> literal;
    vector<Regex*> general;
    vector<Regex*> include;
    vector<Regex*> exclude_node;
    vector<Regex*> exclude_tree;

    for (int i=0; doc.HasMember(to_string(i).c_str()); ++i) {
        const rapidjson::Value& example_row = doc[to_string(i).c_str()];
cout << "example = \"" << example_row["example"].GetString()<<"\"";
cout << " result = \"" << example_row["result"].GetString()<<"\"";
        if (strcmp(example_row["result"].GetString(),"acc")==0) {
            accept_examples.push_back(example_row["example"].GetString());
        } else {
            reject_examples.push_back(example_row["example"].GetString());
        }
cout << " literals = \"" << example_row["literals"].GetString()<<"\"";
        literal_str.push_back(example_row["literals"].GetString());
cout << " generals = [\"";
        for (auto iter = example_row["generals"].Begin(); iter!=example_row["generals"].End(); ++iter) {
cout << iter->GetString() << "\", \"";
            general_str.push_back(iter->GetString());
        }
cout << "\"]";
    }
cout << " include = [\"";
    for (auto iter = doc["include"].Begin(); iter!=doc["include"].End(); ++iter) {
cout << iter->GetString() << "\", \"";
            include_Str.push_back(iter->GetString());
    }
cout << "\"]";
cout << " exclude = [\"";
    for (auto iter = doc["exclude"].Begin(); iter!=doc["exclude"].End(); ++iter) {
cout << iter->GetString() << "\", \"";
            exclude_Str.push_back(iter->GetString());
    }
cout << "\"]\n";

    cout<<"accept_examples: "<<accept_examples<<endl;
    cout<<"reject_examples: "<<reject_examples<<endl;
    cout<<"literal_str: "<<literal_str<<endl;
    cout<<"general_str: "<<general_str<<endl;
    cout<<"include_Str: "<<include_Str<<endl;
    cout<<"exclude_Str: "<<exclude_Str<<endl;

    //cat_over_each_examples_single_chars
    for (auto iter = literal_str.begin(); iter!=literal_str.end(); ++iter) {
        if (iter->size() > 1) {
            Concat* concat_ptr = new Concat();
            for (auto c = (*iter).begin(); c!=(*iter).end(); ++c) {
                if (concat_ptr->e2 == nullptr) {
                    concat_ptr->setClosestLeaf(createSpecificChar(*c));
                } else {
                    Concat* new_root = new Concat(concat_ptr,createSpecificChar(*c));
                    concat_ptr = new_root;
                }
            }
            literal.push_back(concat_ptr);
        }
    }

    string all_literals("");
    for (auto iter = literal_str.begin(); iter!=literal_str.end(); ++iter)
        all_literals+=*iter;

    all_literals = remove_duplicates(all_literals);
    std::sort(all_literals.begin(), all_literals.end());

    for (auto c = all_literals.begin(); c!=all_literals.end(); ++c)
        literal.push_back(createSpecificChar(*c));

    //or_over_all_the_single_chars
    if (all_literals.size()>1) {
        Or* or_ptr = new Or();
        for (auto c = all_literals.begin(); c!=all_literals.end(); ++c) {
            if (or_ptr->e1 == nullptr || or_ptr->e2 == nullptr) {
                or_ptr->setClosestLeaf(createSpecificChar(*c));
            } else {
                Or* new_root = new Or(or_ptr,createSpecificChar(*c));
                or_ptr = new_root;
            }
        }

        literal.push_back(or_ptr);
    }

    for (auto iter = general_str.begin(); iter!=general_str.end(); ++iter)
        general.push_back(stringToRegex(*iter));

    for (auto iter = include_Str.begin(); iter!=include_Str.end(); ++iter)
        include.push_back(stringToRegex(*iter));

    for (auto iter = exclude_Str.begin(); iter!=exclude_Str.end(); ++iter) {
        Regex* ex = stringToRegex(*iter);
        if (regex_instance_of<Unary>(ex) && static_cast<Unary*>(ex)->e!=nullptr) {
            exclude_tree.push_back(ex);
        } else if (regex_instance_of<Binary>(ex) && (static_cast<Binary*>(ex)->e1!=nullptr || static_cast<Binary*>(ex)->e2!=nullptr)) {
            exclude_tree.push_back(ex);
        } else {
            exclude_node.push_back(ex);
        }
    }

    queue<Regex*> tokens = set_tokens(accept_examples,literal,general,include,exclude_node);

    bool found = algo1(ClientSocket,tokens,accept_examples,reject_examples,exclude_tree);
    if (!found) {
        cout<<"the synthesizer didn't find any regular expressoin"<<endl;
    }

    deleteContainer(literal);
    deleteContainer(general);
    deleteContainer(include);
    deleteContainer(exclude_node);
    deleteContainer(exclude_tree);
    deleteContainer(tokens);
}

closesocket(ListenSocket);
cout<<"server after closesocket(ListenSocket)"<<endl;
return 0;

}
