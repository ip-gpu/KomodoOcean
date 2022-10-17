// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "utiltime.h"

#include <thread>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

static int64_t nMockTime = 0;  //! For unit testing

/***
 * Get the current time
 */
int64_t GetTime()
{
    if (nMockTime) 
        return nMockTime;

    return time(NULL);
}

/***
 * Set the current time (for unit testing)
 * @param nMocKtimeIn the time that will be returned by GetTime()
 */
void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime = nMockTimeIn;
}

/***
 * @returns the system time in milliseconds since the epoch (1/1/1970)
 */
int64_t GetTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

/****
 * @returns the system time in microseconds since the epoch (1/1/1970)
 */
int64_t GetTimeMicros()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t GetSystemTimeInSeconds()
{
    return GetTimeMicros()/1000000;
}

/****
 * @param n the number of milliseconds to put the current thread to sleep
 */
void MilliSleep(int64_t n)
{
    //std::this_thread::sleep_for(std::chrono::milliseconds(n)); <-- unable to use due to boost interrupt logic
    boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
}

/***
 * Convert time into a formatted string
 * @param pszFormat the format
 * @param nTime the time
 * @returns the time in the format
 */
std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
{
    // std::locale takes ownership of the pointer
    std::locale loc(std::locale::classic(), new boost::posix_time::time_facet(pszFormat));
    std::stringstream ss;
    ss.imbue(loc);
    ss << boost::posix_time::from_time_t(nTime);
    return ss.str();
}
