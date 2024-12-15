//-------------------------------------------------------------------------------------------------
//  wxupdate.cpp      Created by dennis.lang on 24-Jun-2024
//  Copyright © 2024 Dennis Lang. All rights reserved.
//-------------------------------------------------------------------------------------------------
// This file is part of llwxjson project.
//


#ifndef wxupdate_cpp
#define wxupdate_cpp

// Project files
#include "json.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale.h>
#include <time.h>
#include <cstdlib>
#include <string>

typedef time_t Epoch_t;
typedef struct tm Tm_t;
typedef unsigned int uint;

static Tm_t TM; // conversion
static Epoch_t now;
static Tm_t nowTm;
static Epoch_t refEpoch;    // Reference time from Weather Json.

static const char* DOW[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", nullptr } ;
static const uint SECS_PER_DAY = 24 * 60 * 60;
static const uint SECS_PER_HOUR = 60 * 60;
static const uint SECS_PER_MIN = 60;


static const uint NO_MATCH = -1;
static uint indexOf(const char** array, const char* want, uint defIdx) {
    uint idx = 0;
    while (array[idx] != nullptr) {
        if (strcasecmp(array[idx], want) == 0)
            return idx;
        idx++;
    }
    return defIdx;
}

// https://www.ibm.com/docs/en/i/7.3?topic=functions-strptime-convert-string-datetime
/*
 %a    Name of day of the week, can be either the full name or an abbreviation.
 %A    Same as %a.
 %b    Month name, can be either the full name or an abbreviation.
 %B    Same as %b.
 %c    Date/time, in the format of the locale.
 %C    Century number [00–99]. Calculates the year if a two-digit year is used.
 %d    Day of the month [1–31].
 %D    Date format, same as %m/%d/%y.
 %e    Same as %d.
 %g    2 digit year portion of ISO week date [00–99].
 %G    4 digit year portion of ISO week date. Can be negative.
 %h    Same as %b.
 %H    Hour in 24-hour format [0–23].
 %I    Hour in 12-hour format [1-12].
 %j    Day of the year [1-366].
 %m    Month [1-12].
 %M    Minute [0-59].
 %n    Skip all whitespaces until a newline character is found.
 %p    AM or PM string, used for calculating the hour if 12-hour format is used.
 %r    Time in AM/PM format of the locale. If not available in the locale time format, defaults to the POSIX time AM/PM format: %I:%M:%S %p.
 %R    24-hour time format without seconds, same as %H:%M.
 %S    Second [00-61]. The range for seconds allows for a leap second and a double leap second.
 %t    Skip all whitespaces until a tab character is found.
 %T    24 hour time format with seconds, same as %H:%M:%S .
 %u    Weekday [1–7]. Monday is 1 and Sunday is 7.
 %U    Week number of the year [0-53], Sunday is the first day of the week. Used in calculating the day of the year.
 %V    ISO week number of the year [1-53]. Monday is the first day of the week. If the week containing January 1st has four or more days in the new year, it is considered week 1. Otherwise, it is the last week of the previous year, and the next week is week 1 of the new year. Used in calculating the day of the year.
 %w    Weekday [0 -6]. Sunday is 0.
 %W    Week number of the year [0-53]. Monday is the first day of the week. Used in calculating the day of the year.
 %x    Date in the format of the locale.
 %X    Time in the format of the locale.
 %y    2-digit year [0-99].
 %Y    4-digit year. Can be negative.
 %z    UTC offset. Output is a string with format +HHMM or -HHMM, where + indicates east of GMT, - indicates west of GMT, HH indicates the number of hours from GMT, and MM indicates the number of minutes from GMT.
 %Z    Time zone name.
 %%    % character.
*/
static const char* TM_ISO8601_FMT = "%Y-%m-%dT%T%z";


// Unix Times ex: expirationTimeUtc: 1568663171
static const char* FIELD_EPOCH[] = {"expirationTimeUtc", "validTimeUtc", "insightValidTimeUtc", "expireTimeUTC", "processTimeUTC", "expire_time_gmt", "fcst_valid", nullptr};
static const char* FIELD_EPOCH_DAY[] = {"sunriseTimeUtc", "sunsetTimeUtc", "moonriseTimeUtc", "moonsetTimeUtc", nullptr };
// ISO Times, ex: sunsetTimeLocal: "2019-09-16T17:30:25-0400"
static const char* FIELD_ISO[] = {"expirationTimeLocal", "validTimeLocal", "fcstValidLocal", "expireTimeLocal", "issueTimeLocal", "effectiveTimeLocal", "onsetTimeLocal", "fcst_valid_local", nullptr};
static const char* FIELD_ISO_DAY[] = {"sunriseTimeLocal", "sunsetTimeLocal", "moonriseTimeLocal", "moonsetTimeLocal", nullptr};
// DayOfWeek, ex: dayOfWeek: "Monday"
static const char* FIELD_DOW[] = { "dayOfWeek", "dow", nullptr };
// MonthDay - Almanac
static const char* FIELD_MDAY[] = { "almanacRecordDate", nullptr };

static Tm_t& toGmtTm(const Epoch_t epoch) {
    // localtime_r(&epoch, &TM);
    gmtime_r(&epoch, &TM);
    return TM;
}
static Epoch_t toEpoch(Tm_t& time) {
    // return mktime(&time);           // Assumes local timezone, same as timelocal
    time.tm_sec -= time.tm_gmtoff;
    time.tm_gmtoff = 0;
    return timegm(&time);            // Assumes GMT (ignores gmt offset)
    // return timelocal(&time);            // Support gmt offset
}
static Epoch_t toEpochDay(const Tm_t& day, Epoch_t epochHHMMSS) {
    Tm_t tm = toGmtTm(epochHHMMSS);    // hour, minute, seconds, etc
    tm.tm_year = day.tm_year;
    tm.tm_mon = day.tm_mon;
    tm.tm_mday = day.tm_mday;
    return toEpoch(tm);
}

static const char ISO8601_HHCMM[] = "%04d-%02d-%02dT%02d:%02d:%02d+00:00";
static const char ISO8601_HHMM[] = "%04d-%02d-%02dT%02d:%02d:%02d+0000";

static string& toISO8601(string& out, Epoch_t epoch) {
    char buffer [80];
    // 01234567890123456789012345
    // 2020-03-31T18:00:00-04:10    length=25
    // 2020-03-31T18:00:00-0410     length=24
    Tm_t gmTime = toGmtTm(epoch);
    snprintf(buffer, sizeof(buffer),
        (out.length() > 24) ? ISO8601_HHCMM : ISO8601_HHMM,
        gmTime.tm_year + 1900,
        gmTime.tm_mon + 1,
        gmTime.tm_mday,
        gmTime.tm_hour,
        gmTime.tm_min,
        gmTime.tm_sec);
    out = buffer;
    return out;
}

inline int parseInt(const char* value) {
    return (int)strtol(value, nullptr, 10);
}

// 0123456789012345678901234
// 2020-03-31T18:00:00-04:10
static Epoch_t parseISO8601(const string& value, Tm_t& time) {
    bzero(&time, sizeof(time));
    if (value.length() > 19) {
        const char* str = value.c_str();
        time.tm_year = parseInt(str + 0) - 1900;
        time.tm_mon = parseInt(str + 5) - 1;
        time.tm_mday = parseInt(str + 8);
        time.tm_hour = parseInt(str + 11);
        time.tm_min = parseInt(str + 14);
        time.tm_sec = parseInt(str + 17);

        long gmtOffsetHours = parseInt(str + 19); //   "+/-HH:MM"  or  "+/-HHMM"
        long gmtOffsetMins = 0;
        const char* tzMinPtr = strchr(str + 19, ':');

        if (tzMinPtr != nullptr) {
            long sign = (gmtOffsetHours == 0) ? 1 : abs(gmtOffsetHours) / gmtOffsetHours;
            gmtOffsetMins = parseInt(tzMinPtr + 1) * sign;
        } else {
            gmtOffsetMins = gmtOffsetHours % 100;
            gmtOffsetHours /= 100;
        }
        time.tm_gmtoff = gmtOffsetHours * SECS_PER_HOUR + gmtOffsetMins * SECS_PER_MIN;
        time.tm_isdst = 0;
        return toEpoch(time);
    }
    return 0;
}

static Epoch_t parseISO8601(JsonValue& value) {
    Tm_t time;
    return parseISO8601(value, time);
}

static void setISO8601(JsonValue& value, Epoch_t epoch) {
    toISO8601(value, epoch);
}
static void setISO8601Day(JsonValue& value, Epoch_t epochDay) {
    Epoch_t epochHour = parseISO8601(value);
    Epoch_t epoch = toEpochDay(toGmtTm(epochDay), epochHour);
    toISO8601(value, epoch);
}
static Epoch_t parseEpoch(JsonValue& value) {
    return std::strtoul(value.c_str(), nullptr, 10);
}
static void setEpoch(JsonValue& value, Epoch_t epoch) {
    string str = to_string(epoch);
    value = str;
}
static void setEpochDay(JsonValue& value, Epoch_t epochDay) {
    Epoch_t epochHour = parseEpoch(value);
    Epoch_t epoch = toEpochDay(toGmtTm(epochDay), epochHour);
    string str = to_string(epoch);
    value = str;
}
static Epoch_t parseDOW(JsonValue& value) {
    Tm_t tm = toGmtTm(refEpoch);
    uint dayOfWeek = indexOf(DOW, value.c_str(), NO_MATCH);
    return refEpoch + (dayOfWeek - tm.tm_wday) * SECS_PER_DAY;
}
static void setDOW(JsonValue& value, Epoch_t epoch) {
    Tm_t tm = toGmtTm(epoch);
    value = DOW[tm.tm_wday];
}

typedef Epoch_t (*ParseTime)(JsonValue& value);
typedef void SetTime(JsonValue& value, Epoch_t epoch);

// ---------------------------------------------------------------------------
static void update(const char* name, JsonArray& array, Epoch_t offset, ParseTime parseFunc, SetTime setFunc, bool verbose) {
    for (JsonBase* itemPtr : array) {
        JsonValue& value = itemPtr->asValue();
        Epoch_t time = parseFunc(value);
        if (time != 0) {
            setFunc(value, time + offset);
        } else if (verbose) {
            cerr << "Empty time in array " << name << " value=" << value << endl;
        }
    }
}

// ---------------------------------------------------------------------------
static void update(const char** names, JsonFields& base, Epoch_t offset, ParseTime parseFunc, SetTime setFunc, bool verbose) {
    while (*names) {
        const JsonBase* prevPtr = nullptr;
        JsonBase* ptr = (JsonBase*) base.at("")->find(*names, prevPtr);
        if (ptr != nullptr) {
            while(ptr != nullptr) {
                switch (ptr->mJtype) {
                case JsonBase::Array:
                    update(*names, ptr->asArray(), offset, parseFunc, setFunc, verbose);
                    break;
                case JsonBase::Value: {
                    JsonValue& value = ptr->asValue();
                    Epoch_t time = parseFunc(value);
                    if (time != 0) {
                        if (verbose) cerr << "set " << *names << " from=" << value;
                        setFunc(value, time + offset);
                        if (verbose) cerr << " to=" << value << endl;
                    }
                }
                break;
                case JsonBase::Map:
                case JsonBase::None:
                    assert(false);
                    abort();
                }
                prevPtr = ptr;
                ptr = (JsonBase*) base.at("")->find(*names, prevPtr);
            }
        }
        names++;
    }
}

// ---------------------------------------------------------------------------
static Epoch_t getEpochFrom(const JsonBase* cptr, ParseTime parseFunc) {
    JsonBase* ptr = (JsonBase*)cptr;
    if (ptr != nullptr) {
        if (ptr->is(JsonBase::Array) ) {
            ptr = ptr->asArray().at(0);
        }
        const JsonValue* valPtr = ptr->asValuePtr();
        if (valPtr != nullptr) {
            return parseFunc((JsonValue&) * valPtr);
        }
    }
    return 0;
}

static const char* defaultIf(const char* value, const char* defValue) {
    return (value != nullptr) ? value : defValue;
}

static void dumpTm(const Tm_t& time) {
    cout << "\n   Year=" << time.tm_year
        << "\nYearDay=" << time.tm_yday
        << "\n  Month=" << time.tm_mon
        << "\n    Day=" << time.tm_mday
        << "\n    DOW=" << time.tm_mday
        << "\n  Hours=" << time.tm_hour
        << "\nMinutes=" << time.tm_min
        << "\nSeconds=" << time.tm_sec
        << "\nGMT off=" << time.tm_gmtoff << ", " << setprecision(2) << (float)time.tm_gmtoff / SECS_PER_HOUR << "hr"
        << "\n     TZ=" << time.tm_zone
        << endl;

    // int    tm_isdst;    /* Daylight Savings Time flag */
}
// ---------------------------------------------------------------------------
static void JsonTest() {
    //           0123456789012345678901234
    string s1 = "2020-02-03T08:01:02-01:00";
    string s2 = "2020-02-03T08:01:02+01:00";

    Tm_t time1, time2;
    Epoch_t t1 = parseISO8601(s1, time1);
    Epoch_t t2 = parseISO8601(s2, time2);

    string out1, out2;
    cout << s1 << " converted to=" << toISO8601(out1, t1) << endl;
    dumpTm(time1);
    cout << s2 << " converted to=" << toISO8601(out2, t2) << endl;
    dumpTm(time2);
    cout << "[done]\n";
}

// ---------------------------------------------------------------------------
static bool JsonWxRelative(JsonFields& base, ostream& out, bool verbose) {
    now = std::time(0);
    nowTm = toGmtTm(now);

    if (base.at("") != NULL) {
        refEpoch = 0;

        const JsonBase* prevPtr = nullptr;
        refEpoch = getEpochFrom(base.at("")->find("validTimeUtc", prevPtr), &parseEpoch);
        if (refEpoch == 0) {
            refEpoch = getEpochFrom(base.at("")->find("validTimeLocal", prevPtr), &parseISO8601);
        }
        if (refEpoch == 0) {
            refEpoch = getEpochFrom(base.at("")->find("fcst_valid", prevPtr), &parseEpoch);
        }
        if (refEpoch == 0) {
            refEpoch = getEpochFrom(base.at("")->find("fcst_valid_local", prevPtr), &parseISO8601);
        }
        if (refEpoch == 0) {
            refEpoch = getEpochFrom(base.at("")->find("fcstValidLocal", prevPtr), &parseISO8601);
        }
        if (refEpoch == 0) {
            refEpoch = getEpochFrom(base.at("")->find("obsTimeLocal", prevPtr), &parseISO8601);
        }
        if (refEpoch == 0 ) {
            if (verbose) std::cerr << "Missing any of these: validTimeUtc, validTimeLocal, fcst_valid, fcst_valid_local, fcstValidLocal, obsTimeLocal" << endl;
            return false;
        }

        Epoch_t offset = now - refEpoch;
        update(FIELD_EPOCH, base, offset, &parseEpoch, &setEpoch, verbose);
        update(FIELD_EPOCH_DAY, base, offset, &parseEpoch, &setEpochDay, verbose);
        update(FIELD_ISO, base, offset, &parseISO8601, &setISO8601, verbose);
        update(FIELD_ISO_DAY, base, offset, &parseISO8601, &setISO8601Day, verbose);
        update(FIELD_DOW, base, offset, &parseDOW, &setDOW, verbose);
        // update(FIELD_MDAY, base, offset, &parseMDay, &setMDay, verbose);

        JsonDump(base, out);
        return true;
    }
    return false;
}

#endif
