#include <iostream>

extern "C" {
#include "md5.h"
}

#include "hash.hh"
#include "archive.hh"


Hash::Hash(HashType type)
{
    this->type = type;
    if (type == htMD5) hashSize = md5HashSize;
    else if (type == htSHA1) hashSize = sha1HashSize;
    memset(hash, 0, hashSize);
}


bool Hash::operator == (const Hash & h2) const
{
    if (hashSize != h2.hashSize) return false;
    for (unsigned int i = 0; i < hashSize; i++)
        if (hash[i] != h2.hash[i]) return false;
    return true;
}


bool Hash::operator != (const Hash & h2) const
{
    return !(*this == h2);
}


bool Hash::operator < (const Hash & h) const
{
    for (unsigned int i = 0; i < hashSize; i++) {
        if (hash[i] < h.hash[i]) return true;
        if (hash[i] > h.hash[i]) return false;
    }
    return false;
}


Hash::operator string() const
{
    ostringstream str;
    for (unsigned int i = 0; i < hashSize; i++) {
        str.fill('0');
        str.width(2);
        str << hex << (int) hash[i];
    }
    return str.str();
}

    
Hash parseHash(const string & s)
{
    Hash hash(htMD5);
    if (s.length() != hash.hashSize * 2)
        throw Error(format("invalid hash `%1%'") % s);
    for (unsigned int i = 0; i < hash.hashSize; i++) {
        string s2(s, i * 2, 2);
        if (!isxdigit(s2[0]) || !isxdigit(s2[1])) 
            throw Error(format("invalid hash `%1%'") % s);
        istringstream str(s2);
        int n;
        str >> hex >> n;
        hash.hash[i] = n;
    }
    return hash;
}


bool isHash(const string & s)
{
    if (s.length() != 32) return false;
    for (int i = 0; i < 32; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f')))
            return false;
    }
    return true;
}


Hash hashString(const string & s)
{
    Hash hash(htMD5);
    md5_buffer(s.c_str(), s.length(), hash.hash);
    return hash;
}


Hash hashFile(const Path & path)
{
    Hash hash(htMD5);
    FILE * file = fopen(path.c_str(), "rb");
    if (!file)
        throw SysError(format("file `%1%' does not exist") % path);
    int err = md5_stream(file, hash.hash);
    fclose(file);
    if (err) throw SysError(format("cannot hash file `%1%'") % path);
    return hash;
}


struct HashSink : DumpSink
{
    struct md5_ctx ctx;
    virtual void operator ()
        (const unsigned char * data, unsigned int len)
    {
        md5_process_bytes(data, len, &ctx);
    }
};


Hash hashPath(const Path & path)
{
    Hash hash(htMD5);
    HashSink sink;
    md5_init_ctx(&sink.ctx);
    dumpPath(path, sink);
    md5_finish_ctx(&sink.ctx, hash.hash);
    return hash;
}
