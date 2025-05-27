//-------------------------------------------------------------------------------------------------
//  json.hpp     Created by dennis.lang on 24-Jun-2024
//  Copyright © 2024 Dennis Lang. All rights reserved.
//-------------------------------------------------------------------------------------------------
// This file is part of llwxjson project.
//


#ifndef json_h
#define json_h

#if defined(_WIN32) || defined(_WIN64)
#define HAVE_WIN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS   // define before all includes

#define strcasecmp _strcmpi
#define strncasecmp _strnicmp

// TODO - implement case insenstive strstr
#define strcasestr strstr

#define gmtime_r(__time, __tm)  gmtime_s(__tm, __time)
#define bzero(addr, size)       memset(addr, size, 0)

#include <direct.h>  
#define getcwd _getcwd
#endif

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <exception>
#include <assert.h>
#include <sstream>

using namespace std;

typedef std::vector<string> StringList;
typedef std::map<string, StringList> MapList;

// Forward
class JsonValue;
class JsonArray;
class JsonMap;

static const char* dot = ".";

inline const std::string Join(const StringList& list, const char* delim) {
    size_t len = list.size() * strlen(delim);
    for (const auto& item : list) {
        len += item.length();
    }
    std::string buf;
    buf.reserve(len);
    for (size_t i = 0; i < list.size(); i++) {
        if (i != 0) {
            buf += delim;
        }
        buf += list[i];
    }
    return buf;
}


// Base class for all Json objects
class JsonBase {
public:
    enum Jtype { None, Value, Array, Map };
    Jtype mJtype = None;
    JsonBase(Jtype jtype) {
        mJtype = jtype;
    }
    JsonBase(const JsonBase& other) {
        mJtype = other.mJtype;
    }

    bool is(Jtype jType) const  {
        return mJtype == jType;
    }

    const JsonArray* asArrayPtr() const {
        return is(JsonBase::Array) ? (const JsonArray*)this : nullptr;
    }
    JsonArray& asArray() {
        return *(JsonArray*)this;
    }
    const JsonMap* asMapPtr() const {
        return is(JsonBase::Map) ? (const JsonMap*)this : nullptr;
    }
    JsonMap& asMap() {
        return *( JsonMap*)this;
    }
    const JsonValue* asValuePtr() const {
        return is(JsonBase::Value) ? (const JsonValue*)this : nullptr;
    }
    JsonValue& asValue() {
        return  *(JsonValue*)this;
    }

    /*
    JsonBase& operator=(const JsonBase& other) {
        if (this != &other) {
            mJtype = other.mJtype;
        }
        return *this;
    }
    JsonBase& operator=(const JsonBase&& other) {
        if (this != &other) {
            mJtype = other.mJtype;
        }
        return *this;
    }
     */

    virtual
    string toString() const = 0;

    virtual
    ostream& dump(ostream& out) const = 0;

    virtual
    const JsonBase* find(const char* name, const JsonBase*& prevPtr) const {
        return nullptr;
    }

    virtual
    void toMapList(MapList& mapList, StringList& keys) const = 0;
};


// Simple Value
class JsonValue : public JsonBase, public string {
public:
    const char* quote = "\"";

    bool isQuoted = false;
    JsonValue() : JsonBase(Value), string() {
    }
    JsonValue(const char* str) : JsonBase(Value), string(str) {
    }
    JsonValue(string& str) : JsonBase(Value), string(str) {
    }
    JsonValue(const JsonValue& other) : JsonBase(other), string(other), isQuoted(other.isQuoted) {
    }

    // Keep isQuoted unchanged.
    JsonBase& operator=(const string& other) {
        string::operator=((string&)other);
        return *this;
    }
    JsonBase& operator=(const char* other) {
        string::operator=(other);
        return *this;
    }

    void clear() {
        isQuoted = false;
        string::clear();
    }
    ostream& dump(ostream& out) const {
        out << toString();
        return out;
    }

    const JsonBase* find(const char* name, const JsonBase*& prevPtr) const {
        if ((*this) == name) {
            if (prevPtr == nullptr)
                return this;
            if (prevPtr == this)
                prevPtr = nullptr;
        }
        return nullptr;
    }

    void toMapList(MapList& mapList, StringList& keys) const {
        // can't convert a value to a key,value pair.
        StringList& list = mapList[Join(keys, dot)];
        list.push_back(toString());
    }

    string toString() const {
        if (isQuoted) {
            return string(quote) + *this + string(quote);
        }
        return *this; // ->c_str();
    }
};

typedef std::vector<JsonBase*> VecJson;
typedef std::map<JsonValue, JsonBase*> MapJson;


// Array of Json objects
class JsonArray : public JsonBase, public VecJson {
public:
    JsonArray() : JsonBase(Array) {
    }

    string toString() const {
        std::ostringstream ostr;
        ostr << "[\n";
        JsonArray::const_iterator it = begin();
        bool addComma = false;
        while (it != end()) {
            if (addComma)
                ostr << ",\n";
            addComma = true;
            ostr << (*it++)->toString();
        }
        ostr << "\n]";
        return ostr.str();
    }

    ostream& dump(ostream& out) const {
        out << toString();
        return out;
    }

    const JsonBase* find(const char* name, const JsonBase*& prevPtr) const  {
        const JsonBase* found = nullptr; // JsonBase::find(name);
        JsonArray::const_iterator it = begin();
        while (it != end() && found == nullptr) {
            found = (*it++)->find(name, prevPtr);
        }
        return found;
    }

    void toMapList(MapList& mapList, StringList& keys) const {
        // StringList& list = mapList[Join(keys, dot)];
        JsonArray::const_iterator it = begin();
        StringList itemKeys;
        std::set<string> uniqueKeys;

        while (it != end()) {
            // list.push_back((*it++)->toString());
            itemKeys.clear();
            JsonBase* pValue = *it;
            pValue->toMapList(mapList, itemKeys);
            for (auto& key : itemKeys) {
                uniqueKeys.insert(key);
            }
            it++;
        }

        for (auto& key : uniqueKeys) {
            keys.push_back(key);
        }
    }
};

// Map (group) of Json objects
class JsonMap : public JsonBase, public MapJson {
public:
    JsonMap() : JsonBase(Map), MapJson() {
    }

    string toString() const {
        ostringstream out;
        //bool wrapped = false;

        out << "{\n";
        JsonMap::const_iterator it = begin();
        bool addComma = false;
        while (it != end()) {
            if (addComma)
                out << ",\n";
            addComma = true;

            JsonValue name = it->first;
            JsonBase* pValue = it->second;
            if (! name.empty()) {
                // if (!wrapped) {
                //     wrapped = true;
                //    out << "{\n";
                //}
                out << name.toString() << ": ";
            }
            out << pValue->toString();
            it++;
        }
        // if (wrapped) {
        out << "\n}\n";
        // }

        return out.str();
    }

    const JsonBase* find(const char* name, const JsonBase*& prevPtr) const  {
        const JsonBase* found = nullptr;
        JsonMap::const_iterator it = begin();
        while (it != end() && found == nullptr) {
            if (it->first == name) {
                if (prevPtr == nullptr)
                    return it->second;
                if (prevPtr == it->second)
                    prevPtr = nullptr;
            }
            found = it->second->find(name, prevPtr);
            it++;
        }
        return found;
    }

    ostream& dump(ostream& out) const {
        out << toString();
        return out;
    }

    void toMapList(MapList& mapList, StringList& keys) const {
        JsonMap::const_iterator it = begin();
        while (it != end()) {
            std::string name = it->first;
            JsonBase* pValue = it->second;
            keys.push_back(name);
            pValue->toMapList(mapList, keys);
            keys.pop_back();
            it++;
        }
    }
};

// Alternate name JsonFields for JsonMap
typedef JsonMap JsonFields;

// String buffer being parsed
class JsonBuffer : public std::vector<char> {
public:
    char keyBuf[10];

    size_t pos = 0;
    int seq = 100;

    void push(const char* cptr) {
        while (char c = *cptr++) {
            push_back(c);
        }
        push_back('\0');
    }
    char nextChr() {
        if (pos < size()) {
            return at(pos++);
        }
        return '\0';
    }
    void backup() {
        assert(pos > 0);
        pos--;
    }

    string nextKey() {
        snprintf(keyBuf, sizeof(keyBuf), "%03d", seq++);
        return *(new string(keyBuf));
    }

    const char* ptr(int len = 0) {
        const char* nowPtr = &at(pos);
        pos = std::min(pos + len, size());
        return nowPtr;
    }
};

// Json value or parse state change.
class JsonToken : public JsonValue {
public:
    enum Token { Value, EndArray, EndGroup, EndParse } ;
    Token mToken = Value;

    JsonToken() : JsonValue() {
    }
    JsonToken(const char* str) : JsonValue(str) {
    }
    JsonToken(Token token) : JsonValue(), mToken(token) {
    }
    JsonToken(const JsonToken& other) : JsonValue(other), mToken(other.mToken) {
    }
};

static JsonToken END_ARRAY(JsonToken::EndArray);
static JsonToken END_GROUP(JsonToken::EndGroup);
static JsonToken END_PARSE(JsonToken::EndParse);

// Forward definition
JsonToken JsonParse(JsonBuffer& buffer, JsonFields& jsonFields);
void JsonDump(const JsonFields& base, ostream& out);

#endif /* json_h */

