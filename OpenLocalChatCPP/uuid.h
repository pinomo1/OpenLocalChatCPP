#pragma once
extern "C"
{
#ifdef _WIN32
#pragma comment(lib, "rpcrt4.lib")
#include <windows.h>
#else
#include <uuid/uuid.h>
#endif
}

#include <string>

std::string newUUID()
{
#ifdef _WIN32
    UUID uuid;
    UuidCreate(&uuid);

    unsigned char* str;
    UuidToStringA(&uuid, &str);

    std::string s((char*)str);

    RpcStringFreeA(&str);
#else
    uuid_t uuid;
    uuid_generate_random(uuid);
    char s[37];
    uuid_unparse(uuid, s);
#endif
    return s;
}