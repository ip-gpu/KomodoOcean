#define PACKAGE_NAME "KomodoOcean"

#define CLIENT_VERSION_MAJOR 2
#define CLIENT_VERSION_MINOR 0
#define CLIENT_VERSION_REVISION 15
#define CLIENT_VERSION_BUILD 26

//! Set to true for release, false for prerelease or test build
#define CLIENT_VERSION_IS_RELEASE false

#define COPYRIGHT_YEAR 2018

#define _COPYRIGHT_HOLDERS "The %s developers"
#define _COPYRIGHT_HOLDERS_SUBSTITUTION "Ocean and Decker"
#define COPYRIGHT_HOLDERS _COPYRIGHT_HOLDERS
#define COPYRIGHT_HOLDERS_SUBSTITUTION _COPYRIGHT_HOLDERS_SUBSTITUTION

//typedef int ssize_t;
#ifdef _WIN64
#define ssize_t __int64
#else
#define ssize_t long
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif

#pragma warning(disable : 4090)
#pragma warning(disable : 4100)
#pragma warning(disable : 4189)
//
