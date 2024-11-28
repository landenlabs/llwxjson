//-------------------------------------------------------------------------------------------------
//  json.cpp      Created by dennis.lang on 24-Jun-2024
//  Copyright © 2024 Dennis Lang. All rights reserved.
//-------------------------------------------------------------------------------------------------
// This file is part of llwxjson project.
//

#include "json.hpp"
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------------
static void assertValid(const char* ptr, const char* body) {
    if (ptr == nullptr) {
        std::cerr << "Invalid json near " << body << endl;
        abort();
    }
}

// ---------------------------------------------------------------------------
// Parse json word surrounded by quotes.
static void getJsonWord( JsonBuffer& buffer, char delim, JsonToken& word) {

    const char* lastPtr = strchr(buffer.ptr(), delim);
    while (lastPtr != nullptr && lastPtr[-1] == '\\') {
        lastPtr = strchr(lastPtr + 1, delim);
    }
    assertValid(lastPtr,  buffer.ptr());
    word.clear();
    int len = int(lastPtr - buffer.ptr());
    word.append(buffer.ptr(len + 1), len);
    word.isQuoted = true;
}

// ---------------------------------------------------------------------------
// Parse json array
static void getJsonArray(JsonBuffer& buffer, JsonArray& array) {
    JsonFields jsonFields;
    for(;;) {
        JsonToken token = JsonParse(buffer, jsonFields);
        if (token.mToken == JsonToken::Value) {
            if (! token.empty()) {
                JsonValue* jsonValue = new JsonValue(token);
                array.push_back(jsonValue);
            } else {
                if (jsonFields.size() == 1 && jsonFields.cbegin()->first.empty()) {
                    array.push_back(jsonFields.cbegin()->second);
                } else {
                    JsonValue* jsonValue = new JsonValue(token);
                    array.push_back(jsonValue);
                }
                jsonFields.clear();
            }

        } else {
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Parse json group
static void getJsonGroup(JsonBuffer& buffer, JsonFields& fields) {

    for(;;) {
        JsonToken token = JsonParse(buffer, fields);
        if (token.mToken == JsonToken::EndGroup) {
            return;
        }
    }
}

// ---------------------------------------------------------------------------
static void addJsonValue(JsonFields& jsonFields, JsonToken& fieldName, JsonToken& value) {
    if (! fieldName.empty() /* && !value.empty() */ ) {
        jsonFields[fieldName] = new JsonToken(value);
    }
    fieldName.clear();
    value.clear();
}

// ---------------------------------------------------------------------------
JsonToken JsonParse(JsonBuffer& buffer, JsonFields& jsonFields) {

    JsonToken fieldName = "";
    JsonToken fieldValue;
    JsonToken tmpValue;

    while (buffer.pos < buffer.size()) {
        char chr = buffer.nextChr();

        switch (chr) {
        default:
            fieldValue += chr;
            break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
            // addJsonValue(jsonFields, fieldName, fieldValue);
            break;
        case ',':
            tmpValue = fieldValue;
            addJsonValue(jsonFields, fieldName, fieldValue);
            return tmpValue;

        case ':':
            fieldName = fieldValue;
            fieldValue.clear();
            break;

        case '{': {
            JsonFields* pJsonFields = new JsonFields();
            jsonFields[fieldName] = pJsonFields;
            fieldName.clear();
            getJsonGroup(buffer, *pJsonFields);
        }
        break;
        case '}':
            if (fieldValue.empty()) {
                return END_GROUP;
            } else {
                addJsonValue(jsonFields, fieldName, fieldValue);
                buffer.backup();
                return fieldValue;
            }
            break;
        case '"':
            getJsonWord(buffer, '"', fieldValue);
            break;
        case '[': {
            JsonArray* pJsonArray = new JsonArray();
            jsonFields[fieldName] = pJsonArray;
            fieldName.clear();
            getJsonArray(buffer, *pJsonArray);
        }
        break;
        case ']':
            // addJsonValue(jsonFields, fieldName, fieldValue);
            if (jsonFields.size() != 0 || ! fieldValue.empty()) {
                buffer.backup();
                return fieldValue;
            } else {
                return END_ARRAY;
            }
            break;
        }
    }

    return END_PARSE;
}

// ---------------------------------------------------------------------------
// Dump parsed json in json format.
void JsonDump(const JsonFields& base, ostream& out) {
    // If json parsed, first node can be ignored.
    if (base.at("") != NULL) {
        base.at("")->dump(out);
    }
}
