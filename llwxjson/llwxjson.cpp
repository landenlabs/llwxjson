//-------------------------------------------------------------------------------------------------
//  llwxjson.cpp      Created by dennis.lang on 24-Jun-2024
//  Copyright © 2024 Dennis Lang. All rights reserved.
//-------------------------------------------------------------------------------------------------
// This file is part of llwxjson project.
//

// 4291 - No matching operator delete found
// #pragma warning( disable : 4291 )
// #define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

// Project files
#include "json.hpp"
#include "wxupdate.cpp"

using namespace std;

class Options {
public:
    bool dumpOnly;
    bool verbose;
    bool addHttpdPrefix;
    bool test;
    Options() : dumpOnly(false), verbose(false), addHttpdPrefix(true), test(false) {}
};

// ---------------------------------------------------------------------------
// Open, read and parse file.
bool JsonParseFile(const string& filepath, const Options& options) {
    ifstream        in;
    ofstream        out;
    struct stat     filestat;
    JsonFields fields;

    if (options.verbose) {
        std::cerr << "Parsing file:" << filepath << std::endl;
    }

    try {
        if (stat(filepath.c_str(), &filestat) != 0)
            return false;

        in.open(filepath);
        if (in.good()) {
            JsonBuffer buffer;
            buffer.resize(filestat.st_size + 1);
            streamsize inCnt = in.read(buffer.data(), buffer.size()).gcount();
            assert(inCnt < buffer.size());
            in.close();
            buffer.push_back('\0');
            JsonParse(buffer, fields);
        } else {
            if (options.verbose) cerr << strerror(errno) << ", Unable to open " << filepath << endl;
            return false;
        }
    } catch (exception ex) {
        if (options.verbose) cerr << ex.what() << ", Error in file:" << filepath << endl;
        return false;
    }

    if (options.addHttpdPrefix) {
        // Prefix for HTTPD server
        cout << "Content-type: text/json\n\n";
    }

    if (options.test) {
        JsonTest();
    } else if (options.dumpOnly) {
        JsonDump(fields, cout);
    } else {
        return JsonWxRelative(fields, cout, options.verbose);
    }

    return true;
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    Options options;
    char* cgiCmdStr = nullptr;

    if (argc == 1) {
        // Get optional CGI bin query string.
        cgiCmdStr = getenv("QUERY_STRING");
        if (cgiCmdStr == nullptr || strlen(cgiCmdStr) == 0) {
            cgiCmdStr = getenv("query_string");
        }

        if (cgiCmdStr == nullptr || strlen(cgiCmdStr) == 0) {
            cerr << "\n" << argv[0] << "  Dennis Lang v1.2 " __DATE__ << "\n"
                << "\nDes: Make weather times relative to now\n"
                    "Use: llwxjson [options] file\n"
                    "\n"
                    " Options:\n"
                    "   -dump         ; Only dump parsed json\n"
                    "   -noHttpPrefix ; Disable http content-type output\n"
                    "   -verbose    \n"
                    "   -test       \n"
                    "\n"
                    " or pass filename using environment variable QUERY_STRING \n"
                    "   setenv QUERY_STRING /path/wxjson.json \n"
                    "\n";
            return 1;
        }
    }

    bool doParseCmds = true;
    string endCmds = "--";
    for (int argn = 1; argn < argc; argn++) {
        if (*argv[argn] == '-' && doParseCmds) {
            string argStr(argv[argn]);
            switch (argStr[(unsigned)1]) {
            case 'd':   // dump
                options.dumpOnly = true;
                continue;
            case 'n':   // noHttpPrefix
                options.addHttpdPrefix = false;
                continue;
            case 't':   // test
                options.test = true;
                continue;
            case 'v':   // verbose
                options.verbose = true;
                continue;
            }

            if (endCmds == argv[argn]) {
                doParseCmds = false;
            } else {
                std::cerr << "Unknown command " << argStr << std::endl;
            }
        } else {
            return JsonParseFile(argv[argn], options) ? 0 : -1;
        }
    }

    if (cgiCmdStr != nullptr && strlen(cgiCmdStr) != 0) {
        char tmpBuf[256];
        getcwd(tmpBuf, sizeof(tmpBuf));
        string fullpath = tmpBuf;
        fullpath += "/";
        fullpath += strchr(cgiCmdStr, '=') + 1; // skip over "site=" prefix
        return JsonParseFile(fullpath.c_str(), options) ? 0 : -1;
    }

    return 0;
}
