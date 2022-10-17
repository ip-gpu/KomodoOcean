/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#include "komodo_defs.h"
#include "komodo.h"
#include "komodo_utils.h"
#include "komodo_globals.h"
#include "komodo_notary.h"
#include "komodo_gateway.h"
#include "notaries_staked.h"

#include "cc/CCinclude.h"

void vcalc_sha256(char deprecated[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len)
{
    CSHA256().Write((const unsigned char *)src, len).Finalize(hash);
}

bits256 bits256_doublesha256(char *deprecated,uint8_t *data,int32_t datalen)
{
    bits256 hash,hash2; int32_t i;
    vcalc_sha256(0,hash.bytes,data,datalen);
    vcalc_sha256(0,hash2.bytes,hash.bytes,sizeof(hash));
    for (i=0; i<sizeof(hash); i++)
        hash.bytes[i] = hash2.bytes[sizeof(hash) - 1 - i];
    return(hash);
}

static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t calc_crc32(uint32_t crc,const void *buf,size_t size)
{
	const uint8_t *p;

	p = (const uint8_t *)buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen)
{
    CHash160().Write((const unsigned char *)data, datalen).Finalize(rmd160); // SHA-256 + RIPEMD-160
}

int32_t bitcoin_addr2rmd160(uint8_t *addrtypep,uint8_t rmd160[20],const char *coinaddr)
{
    bits256 hash; uint8_t *buf,_buf[25]; int32_t len;
    memset(rmd160,0,20);
    *addrtypep = 0;
    buf = _buf;
    if ( (len= bitcoin_base58decode(buf,coinaddr)) >= 4 )
    {
        // validate with trailing hash, then remove hash
        hash = bits256_doublesha256(0,buf,21);
        *addrtypep = *buf;
        memcpy(rmd160,buf+1,20);
        if ( (buf[21]&0xff) == hash.bytes[31] && (buf[22]&0xff) == hash.bytes[30] &&(buf[23]&0xff) == hash.bytes[29] && (buf[24]&0xff) == hash.bytes[28] )
        {
            //LogPrintf("coinaddr.(%s) valid checksum addrtype.%02x\n",coinaddr,*addrtypep);
            return(20);
        }
        else
        {
            int32_t i;
            if ( len > 20 )
            {
                hash = bits256_doublesha256(0,buf,len);
            }
            for (i=0; i<len; i++)
                LogPrintf("%02x ",buf[i]);
            LogPrintf("\nhex checkhash.(%s) len.%d mismatch %02x %02x %02x %02x vs %02x %02x %02x %02x\n",coinaddr,len,buf[len-1]&0xff,buf[len-2]&0xff,buf[len-3]&0xff,buf[len-4]&0xff,hash.bytes[31],hash.bytes[30],hash.bytes[29],hash.bytes[28]);
        }
    }
	return(0);
}

char *bitcoin_address(char *coinaddr,uint8_t addrtype,uint8_t *pubkey_or_rmd160,int32_t len)
{
    int32_t i; 
    uint8_t data[25]; 
    bits256 hash;
    if ( len != 20 )
        calc_rmd160_sha256(data+1,pubkey_or_rmd160,len);
    else 
        memcpy(data+1,pubkey_or_rmd160,20);
    data[0] = addrtype;
    hash = bits256_doublesha256(0,data,21);
    for (i=0; i<4; i++)
        data[21+i] = hash.bytes[31-i];
    return bitcoin_base58encode(coinaddr,data,25);
}


/*int32_t komodo_baseid(const char *origbase)
{
    int32_t i; char base[64];
    for (i=0; origbase[i]!=0&&i<sizeof(base); i++)
        base[i] = toupper((int32_t)(origbase[i] & 0xff));
    base[i] = 0;
    for (i=0; i<=MAX_CURRENCIES; i++)
        if ( strcmp(CURRENCIES[i],base) == 0 )
            return(i);
    return(-1);
}*/

int32_t bitweight(uint64_t x)
{
    int i,wt = 0;
    for (i=0; i<64; i++)
        if ( (1LL << i) & x )
            wt++;
    return(wt);
}

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp)
{
    int32_t i; uint64_t x;
    if ( rwflag == 0 )
    {
        x = 0;
        for (i=len-1; i>=0; i--)
        {
            x <<= 8;
            x |= serialized[i];
        }
        switch ( len )
        {
            case 1: *(uint8_t *)endianedp = (uint8_t)x; break;
            case 2: *(uint16_t *)endianedp = (uint16_t)x; break;
            case 4: *(uint32_t *)endianedp = (uint32_t)x; break;
            case 8: *(uint64_t *)endianedp = (uint64_t)x; break;
        }
    }
    else
    {
        x = 0;
        switch ( len )
        {
            case 1: x = *(uint8_t *)endianedp; break;
            case 2: x = *(uint16_t *)endianedp; break;
            case 4: x = *(uint32_t *)endianedp; break;
            case 8: x = *(uint64_t *)endianedp; break;
        }
        for (i=0; i<len; i++,x >>= 8)
            serialized[i] = (uint8_t)(x & 0xff);
    }
    return(len);
}

int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp)
{
    int32_t i;
    if ( rwflag == 0 )
    {
        for (i=0; i<len; i++)
            endianedp[i] = serialized[i];
    }
    else
    {
        for (i=0; i<len; i++)
            serialized[i] = endianedp[i];
    }
    return(len);
}

int32_t komodo_opreturnscript(uint8_t *script,uint8_t type,uint8_t *opret,int32_t opretlen)
{
    int32_t offset = 0;
    script[offset++] = 0x6a;
    opretlen++;
    if ( opretlen >= 0x4c )
    {
        if ( opretlen > 0xff )
        {
            script[offset++] = 0x4d;
            script[offset++] = opretlen & 0xff;
            script[offset++] = (opretlen >> 8) & 0xff;
        }
        else
        {
            script[offset++] = 0x4c;
            script[offset++] = opretlen;
        }
    } else script[offset++] = opretlen;
    script[offset++] = type; // covered by opretlen
    memcpy(&script[offset],opret,opretlen-1);
    return(offset + opretlen - 1);
}

// get a pseudo random number that is the same for each block individually at all times and different
// from all other blocks. the sequence is extremely likely, but not guaranteed to be unique for each block chain
uint64_t komodo_block_prg(uint32_t nHeight)
{
    uint64_t i, result = 0, hashSrc64 = ((uint64_t)ASSETCHAINS_MAGIC << 32) | (uint64_t)nHeight;
    uint8_t hashSrc[8];
    bits256 hashResult;

    for ( i = 0; i < sizeof(hashSrc); i++ )
    {
        uint64_t x = hashSrc64 >> (i * 8);
        hashSrc[i] = (uint8_t)(x & 0xff);
    }
    vcalc_sha256(0, hashResult.bytes, hashSrc, sizeof(hashSrc));
    for ( i = 0; i < 8; i++ )
    {
        result = (result << 8) | hashResult.bytes[i];
    }
    return result;
}

// given a block height, this returns the unlock time for that block height, derived from
// the ASSETCHAINS_MAGIC number as well as the block height, providing different random numbers
// for corresponding blocks across chains, but the same sequence in each chain
int64_t komodo_block_unlocktime(uint32_t nHeight)
{
    uint64_t fromTime, toTime, unlocktime;

    if ( ASSETCHAINS_TIMEUNLOCKFROM == ASSETCHAINS_TIMEUNLOCKTO )
        unlocktime = ASSETCHAINS_TIMEUNLOCKTO;
    else
    {
        unlocktime = komodo_block_prg(nHeight) % (ASSETCHAINS_TIMEUNLOCKTO - ASSETCHAINS_TIMEUNLOCKFROM);
        unlocktime += ASSETCHAINS_TIMEUNLOCKFROM;
    }
    return ((int64_t)unlocktime);
}

long _stripwhite(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        LogPrintf("warning cloning nullstr.%p\n",str);
#ifdef __APPLE__
        while ( 1 ) sleep(1);
#endif
        str = (char *)"<nullstr>";
    }
    clone = (char *)malloc(strlen(str)+16);
    strcpy(clone,str);
    return(clone);
}

int32_t safecopy(char *dest,char *src,long len)
{
    int32_t i = -1;
    if ( src != 0 && dest != 0 && src != dest )
    {
        if ( dest != 0 )
            memset(dest,0,len);
        for (i=0; i<len&&src[i]!=0; i++)
            dest[i] = src[i];
        if ( i == len )
        {
            LogPrintf("safecopy: %s too long %ld\n",src,len);
#ifdef __APPLE__
            //getchar();
#endif
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}

char *parse_conf_line(char *line,char *field)
{
    line += strlen(field);
    for (; *line!='='&&*line!=0; line++)
        break;
    if ( *line == 0 )
        return(0);
    if ( *line == '=' )
        line++;
    while ( line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n' || line[strlen(line)-1] == ' ' )
        line[strlen(line)-1] = 0;
    //LogPrintf("LINE.(%s)\n",line);
    _stripwhite(line,0);
    return(clonestr(line));
}

double OS_milliseconds()
{
    struct timeval tv; double millis;
    gettimeofday(&tv,NULL);
    millis = ((double)tv.tv_sec * 1000. + (double)tv.tv_usec / 1000.);
    //LogPrintf("tv_sec.%ld usec.%d %f\n",tv.tv_sec,tv.tv_usec,millis);
    return(millis);
}

#ifndef _WIN32
void OS_randombytes(unsigned char *x,long xlen)
{
    static int fd = -1;
    int32_t i;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom",O_RDONLY);
            if (fd != -1) break;
            sleep(1);
        }
    }
    while (xlen > 0) {
        if (xlen < 1048576) i = (int32_t)xlen; else i = 1048576;
        i = (int32_t)read(fd,x,i);
        if (i < 1) {
            sleep(1);
            continue;
        }
        if ( 0 )
        {
            int32_t j;
            for (j=0; j<i; j++)
                LogPrintf("%02x ",x[j]);
            LogPrintf("-> %p\n",x);
        }
        x += i;
        xlen -= i;
    }
}
#endif

/**
 * @brief Get the username, password, and port from a file
 * @param[out] username the username found in the config file 
 * @param[out] password the password found in the config file
 * @param[in] fp the file to be read
 * @return the RPC port
 */
uint16_t _komodo_userpass(char *username,char *password,FILE *fp)
{
    uint16_t port = 0;
    char *rpcuser = nullptr;
    char *rpcpassword = nullptr;
    char line[8192];

    username[0] = 0;
    password[0] = 0;

    while ( fgets(line,sizeof(line),fp) != 0 )
    {
        char *str = nullptr;
        if ( line[0] == '#' )
            continue;
        if ( (str= strstr(line,(char *)"rpcuser")) != nullptr )
        {
            rpcuser = parse_conf_line(str,(char *)"rpcuser");
        }
        else if ( (str= strstr(line,(char *)"rpcpassword")) != nullptr )
        {
            rpcpassword = parse_conf_line(str,(char *)"rpcpassword");
        }
        else if ( (str= strstr(line,(char *)"rpcport")) != nullptr )
        {
            port = atoi(parse_conf_line(str,(char *)"rpcport"));
        }
    }
    if ( rpcuser != nullptr && rpcpassword != nullptr )
    {
        strcpy(username,rpcuser);
        strcpy(password,rpcpassword);
    }
    if ( rpcuser != nullptr )
        free(rpcuser);
    if ( rpcpassword != nullptr )
        free(rpcpassword);
    return port;
}

void komodo_statefname(char *fname, const char *symbol, const char *str)
{
    int32_t n,len;
    snprintf(fname, MAX_STATEFNAME, "%s",GetDataDir(false).string().c_str());
    if ( (n= (int32_t)chainName.symbol().size()) != 0 )
    {
        len = (int32_t)strlen(fname);
        if ( !mapArgs.count("-datadir") && strcmp(chainName.symbol().c_str(),&fname[len - n]) == 0 )
            fname[len - n] = 0;
        else if(mapArgs.count("-datadir")) LogPrintf("DEBUG - komodo_utils:1363: custom datadir\n");
        else
        {
            if ( strcmp(symbol,"REGTEST") != 0 )
                LogPrintf("unexpected fname.(%s) vs %s [%s] n.%d len.%d (%s)\n",fname,symbol,chainName.symbol().c_str(),n,len,&fname[len - n]);
            return;
        }
    }
    else
    {
#ifdef _WIN32
        strcat(fname,"\\");
#else
        strcat(fname,"/");
#endif
    }
    if ( symbol != 0 && symbol[0] != 0 && strcmp("KMD",symbol) != 0 )
    {
        if(!mapArgs.count("-datadir")) strcat(fname,symbol);
#ifdef _WIN32
        strcat(fname,"\\");
#else
        strcat(fname,"/");
#endif
    }
    strcat(fname,str);
}

void komodo_configfile(const char *symbol,uint16_t rpcport)
{
    static char myusername[512],mypassword[8192];
    FILE *fp; uint16_t kmdport; uint8_t buf2[33]; char fname[512],buf[128],username[512],password[8192]; uint32_t crc,r,r2,i;
    if ( symbol != 0 && rpcport != 0 )
    {
        r = (uint32_t)time(NULL);
        r2 = OS_milliseconds();
        memcpy(buf,&r,sizeof(r));
        memcpy(&buf[sizeof(r)],&r2,sizeof(r2));
        memcpy(&buf[sizeof(r)+sizeof(r2)],symbol,strlen(symbol));
        crc = calc_crc32(0,(uint8_t *)buf,(int32_t)(sizeof(r)+sizeof(r2)+strlen(symbol)));
				#ifdef _WIN32
				randombytes_buf(buf2,sizeof(buf2));
				#else
        OS_randombytes(buf2,sizeof(buf2));
				#endif
        for (i=0; i<sizeof(buf2); i++)
            sprintf(&password[i*2],"%02x",buf2[i]);
        password[i*2] = 0;
        sprintf(buf,"%s.conf",symbol);
        BITCOIND_RPCPORT = rpcport;
#ifdef _WIN32
        sprintf(fname,"%s\\%s",GetDataDir(false).string().c_str(),buf);
#else
        sprintf(fname,"%s/%s",GetDataDir(false).string().c_str(),buf);
#endif
        if(mapArgs.count("-conf")) sprintf(fname, "%s", GetConfigFile().string().c_str());
        if ( (fp= fopen(fname,"rb")) == 0 )
        {
#ifndef FROM_CLI
            if ( (fp= fopen(fname,"wb")) != 0 )
            {
                fprintf(fp,"rpcuser=user%u\nrpcpassword=pass%s\nrpcport=%u\nserver=1\ntxindex=1\nrpcworkqueue=256\nrpcallowip=127.0.0.1\nrpcbind=127.0.0.1\n",crc,password,rpcport);
                fclose(fp);
                LogPrintf("Created (%s)\n",fname);
            } else LogPrintf("Couldnt create (%s)\n",fname);
#endif
        }
        else
        {
            _komodo_userpass(myusername,mypassword,fp);
            mapArgs["-rpcpassword"] = mypassword;
            mapArgs["-rpcusername"] = myusername;
            //LogPrintf("myusername.(%s)\n",myusername);
            fclose(fp);
        }
    }
    strcpy(fname,GetDataDir().string().c_str());
#ifdef _WIN32
    while ( fname[strlen(fname)-1] != '\\' )
        fname[strlen(fname)-1] = 0;
    strcat(fname,"komodo.conf");
#else
    while ( fname[strlen(fname)-1] != '/' )
        fname[strlen(fname)-1] = 0;
#ifdef __APPLE__
    strcat(fname,"Komodo.conf");
#else
    strcat(fname,"komodo.conf");
#endif
#endif
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        if ( (kmdport= _komodo_userpass(username,password,fp)) != 0 )
            KMD_PORT = kmdport;
        sprintf(KMDUSERPASS,"%s:%s",username,password);
        fclose(fp);
    }
}

uint16_t komodo_userpass(char *userpass,const char *symbol)
{
    FILE *fp; uint16_t port = 0; char fname[512],username[512],password[512],confname[KOMODO_ASSETCHAIN_MAXLEN];
    userpass[0] = 0;
    if ( strcmp("KMD",symbol) == 0 )
    {
#ifdef __APPLE__
        sprintf(confname,"Komodo.conf");
#else
        sprintf(confname,"komodo.conf");
#endif
    }
    else if(!mapArgs.count("-conf")) {
        sprintf(confname,"%s.conf",symbol);
        komodo_statefname(fname,symbol,confname);
    } else sprintf(fname,"%s",GetDataDir().string().c_str());
    
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        port = _komodo_userpass(username,password,fp);
        sprintf(userpass,"%s:%s",username,password);
        if ( chainName.isSymbol(symbol) )
            strcpy(ASSETCHAINS_USERPASS,userpass);
        fclose(fp);
    }
    return(port);
}

/**
 * @brief Compute the magic number
 * 
 * @param symbol the chain symbol
 * @param supply max supply
 * @param extraptr details of chain parameters
 * @param extralen length of extraptr
 * @return the magic number
 */
uint32_t komodo_assetmagic(const char *symbol,uint64_t supply,uint8_t *extraptr,int32_t extralen)
{
    uint8_t buf[512]; 
    uint32_t crc0=0; 
    int32_t len = 0; 
    bits256 hash;
    if ( strcmp(symbol,"KMD") == 0 )
        return 0x8de4eef9;

    len = iguana_rwnum(1,&buf[len],sizeof(supply),(void *)&supply);
    strcpy((char *)&buf[len],symbol);
    len += strlen(symbol);

    if ( extraptr != 0 && extralen != 0 )
    {
        bits256 hash;
        vcalc_sha256(0,hash.bytes,extraptr,extralen);
        crc0 = hash.uints[0];
        for (int32_t i=0; i<extralen; i++)
            LogPrintf("%02x",extraptr[i]);
        LogPrintf(" extralen.%d crc0.%x\n",extralen,crc0);
    }

    return calc_crc32(crc0,buf,len);
}

/**
 * @brief Compute the default port
 * 
 * @param magic the magic number
 * @param extralen length of extra chain parameters
 * @return the default port
 */
uint16_t komodo_assetport(uint32_t magic,int32_t extralen)
{
    if ( magic == 0x8de4eef9 )
        return 7770;
    else if ( extralen == 0 )
        return 8000 + (magic % 7777);
    return 16000 + (magic % 49500);
}

/**
 * @brief get the magicp and port for this chain
 * 
 * @param symbol chain symbol
 * @param supply max supply
 * @param[out] magicp the magicp for this chain
 * @param extraptr details of chain
 * @param extralen length of extraptr
 * @return the default port for this chain
 */
uint16_t komodo_port(const char *symbol,uint64_t supply,uint32_t *magicp,uint8_t *extraptr,int32_t extralen)
{
    if ( symbol == 0 || symbol[0] == 0 || strcmp("KMD",symbol) == 0 )
    {
        *magicp = 0x8de4eef9;
        return 7770;
    }
    *magicp = komodo_assetmagic(symbol,supply,extraptr,extralen);
    return komodo_assetport(*magicp,extralen);
}


int32_t komodo_whoami(char *pubkeystr,int32_t height,uint32_t timestamp)
{
    int32_t i,notaryid;
    for (i=0; i<33; i++)
        sprintf(&pubkeystr[i<<1],"%02x",NOTARY_PUBKEY33[i]);
    pubkeystr[66] = 0;
    komodo_chosennotary(&notaryid,height,NOTARY_PUBKEY33,timestamp);
    return(notaryid);
}

char *argv0suffix[] =
{
    (char *)"mnzd", (char *)"mnz-cli", (char *)"mnzd.exe", (char *)"mnz-cli.exe", (char *)"btchd", (char *)"btch-cli", (char *)"btchd.exe", (char *)"btch-cli.exe"
};

char *argv0names[] =
{
    (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH"
};

// todo remove
//#ifndef SATOSHIDEN
//#define SATOSHIDEN ((uint64_t)100000000L)
//#endif

uint64_t komodo_current_supply(uint32_t nHeight)
{
    uint64_t cur_money;
    int32_t baseid;

    {
        // figure out max_money by adding up supply to a maximum of 10,000,000 blocks
        cur_money = (ASSETCHAINS_SUPPLY+1) * SATOSHIDEN + (ASSETCHAINS_MAGIC & 0xffffff) + ASSETCHAINS_GENESISTXVAL;
        if ( ASSETCHAINS_LASTERA == 0 && ASSETCHAINS_REWARD[0] == 0 )
        {
            cur_money += (nHeight * 10000);// / SATOSHIDEN;
        }
        else
        {
            for ( int j = 0; j <= ASSETCHAINS_LASTERA; j++ )
            {
                // if any condition means we have no more rewards, break
                if (j != 0 && (nHeight <= ASSETCHAINS_ENDSUBSIDY[j - 1] || (ASSETCHAINS_ENDSUBSIDY[j - 1] == 0 &&
                    (ASSETCHAINS_REWARD[j] == 0 && (j == ASSETCHAINS_LASTERA || ASSETCHAINS_DECAY[j] != SATOSHIDEN)))))
                    break;

                // add rewards from this era, up to nHeight
                int64_t reward = ASSETCHAINS_REWARD[j];
                
                //LogPrintf("last.%d reward %llu period %llu\n",(int32_t)ASSETCHAINS_LASTERA,(long long)reward,(long long)ASSETCHAINS_HALVING[j]);
                if ( reward > 0 )
                {
                    uint64_t lastEnd = j == 0 ? 0 : ASSETCHAINS_ENDSUBSIDY[j - 1];
                    uint64_t curEnd = ASSETCHAINS_ENDSUBSIDY[j] == 0 ? nHeight : nHeight > ASSETCHAINS_ENDSUBSIDY[j] ? ASSETCHAINS_ENDSUBSIDY[j] : nHeight;
                    uint64_t period = ASSETCHAINS_HALVING[j];
                    if ( period == 0 )
                        period = 210000;
                    uint32_t nSteps = (curEnd - lastEnd) / period;
                    uint32_t modulo = (curEnd - lastEnd) % period;
                    uint64_t decay = ASSETCHAINS_DECAY[j];

                    //LogPrintf("period.%llu cur_money %.8f += %.8f * %d\n",(long long)period,(double)cur_money/COIN,(double)reward/COIN,nHeight);
                    if ( ASSETCHAINS_HALVING[j] == 0 )
                    {
                        // no halving, straight multiply
                        cur_money += reward * (nHeight - 1);
                        //LogPrintf("cur_money %.8f\n",(double)cur_money/COIN);
                    }
                    // if exactly SATOSHIDEN, linear decay to zero or to next era, same as:
                    // (next_era_reward + (starting reward - next_era_reward) / 2) * num_blocks
                    else if ( decay == SATOSHIDEN )
                    {
                        int64_t lowestSubsidy, subsidyDifference, stepDifference, stepTriangle;
                        int64_t denominator, modulo=1;
                        int32_t sign = 1;

                        if ( j == ASSETCHAINS_LASTERA )
                        {
                            subsidyDifference = reward;
                            lowestSubsidy = 0;
                        }
                        else
                        {
                            // Ex: -ac_eras=3 -ac_reward=0,384,24 -ac_end=1440,260640,0 -ac_halving=1,1440,2103840 -ac_decay 100000000,97750000,0
                            subsidyDifference = reward - ASSETCHAINS_REWARD[j + 1];
                            if (subsidyDifference < 0)
                            {
                                sign = -1;
                                subsidyDifference *= sign;
                                lowestSubsidy = reward;
                            }
                            else
                            {
                                lowestSubsidy = ASSETCHAINS_REWARD[j + 1];
                            }
                        }

                        // if we have not finished the current era, we need to caluclate a total as if we are at the end, with the current
                        // subsidy. we will calculate the total of a linear era as follows. Each item represents an area calculation:
                        // a) the rectangle from 0 to the lowest reward in the era * the number of blocks
                        // b) the rectangle of the remainder of blocks from the lowest point of the era to the highest point of the era if any remainder
                        // c) the minor triangle from the start of transition from the lowest point to the start of transition to the highest point
                        // d) one halving triangle (half area of one full step)
                        //
                        // we also need:
                        // e) number of steps = (n - erastart) / halving interval
                        //
                        // the total supply from era start up to height is:
                        // a + b + c + (d * e)

                        // calculate amount in one step's triangular protrusion over minor triangle's hypotenuse
                        denominator = nSteps * period;
                        if ( denominator == 0 )
                            denominator = 1;
                        // difference of one step vs. total
                        stepDifference = (period * subsidyDifference) / denominator;

                        // area == coin holding of one step triangle, protruding from minor triangle's hypotenuse
                        stepTriangle = (period * stepDifference) >> 1;

                        // sign is negative if slope is positive (start is less than end)
                        if (sign < 0)
                        {
                            // use steps minus one for our calculations, and add the potentially partial rectangle
                            // at the end
                            cur_money += stepTriangle * (nSteps - 1);
                            cur_money += stepTriangle * (nSteps - 1) * (nSteps - 1);

                            // difference times number of steps is height of rectangle above lowest subsidy
                            cur_money += modulo * stepDifference * nSteps;
                        }
                        else
                        {
                            // if negative slope, the minor triangle is the full number of steps, as the highest
                            // level step is full. lowest subsidy is just the lowest so far
                            lowestSubsidy = reward - (stepDifference * nSteps);

                            // add the step triangles, one per step
                            cur_money += stepTriangle * nSteps;

                            // add the minor triangle
                            cur_money += stepTriangle * nSteps * nSteps;
                        }

                        // add more for the base rectangle if lowest subsidy is not 0
                        cur_money += lowestSubsidy * (curEnd - lastEnd);
                    }
                    else
                    {
                        for ( int k = lastEnd; k < curEnd; k += period )
                        {
                            cur_money += period * reward;
                            // if zero, we do straight halving
                            reward = decay ? (reward * decay) / SATOSHIDEN : reward >> 1;
                        }
                        cur_money += modulo * reward;
                    }
                }
            }
        }
    }    
    if ( KOMODO_BIT63SET(cur_money) != 0 )
        return(KOMODO_MAXNVALUE);
    if ( ASSETCHAINS_COMMISSION != 0 )
    {
        uint64_t newval = (cur_money + (cur_money/COIN * ASSETCHAINS_COMMISSION));
        if ( KOMODO_BIT63SET(newval) != 0 )
            return(KOMODO_MAXNVALUE);
        else if ( newval < cur_money ) // check for underflow
            return(KOMODO_MAXNVALUE);
        return(newval);
    }
    //LogPrintf("cur_money %.8f\n",(double)cur_money/COIN);
    return(cur_money);
}

uint64_t komodo_max_money()
{
    return komodo_current_supply(10000000);
}

uint64_t komodo_ac_block_subsidy(int nHeight)
{
    // we have to find our era, start from beginning reward, and determine current subsidy
    int64_t numerator, denominator, subsidy = 0;
    int64_t subsidyDifference;
    int32_t numhalvings, curEra = 0, sign = 1;
    static uint64_t cached_subsidy; static int32_t cached_numhalvings; static int cached_era;

    // check for backwards compat, older chains with no explicit rewards had 0.0001 block reward
    if ( ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] == 0 )
        subsidy = 10000;
    else if ( (ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] != 0) || ASSETCHAINS_ENDSUBSIDY[0] != 0 )
    {
        // if we have an end block in the first era, find our current era
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 )
        {
            for ( curEra = 0; curEra <= ASSETCHAINS_LASTERA; curEra++ )
            {
                if ( ASSETCHAINS_ENDSUBSIDY[curEra] > nHeight || ASSETCHAINS_ENDSUBSIDY[curEra] == 0 )
                    break;
            }
        }
        if ( curEra <= ASSETCHAINS_LASTERA )
        {
            int64_t nStart = curEra ? ASSETCHAINS_ENDSUBSIDY[curEra - 1] : 0;
            subsidy = (int64_t)ASSETCHAINS_REWARD[curEra];
            if ( subsidy || (curEra != ASSETCHAINS_LASTERA && ASSETCHAINS_REWARD[curEra + 1] != 0) )
            {
                if ( ASSETCHAINS_HALVING[curEra] != 0 )
                {
                    if ( (numhalvings = ((nHeight - nStart) / ASSETCHAINS_HALVING[curEra])) > 0 )
                    {
                        if ( ASSETCHAINS_DECAY[curEra] == 0 )
                            subsidy >>= numhalvings;
                        else if ( ASSETCHAINS_DECAY[curEra] == 100000000 && ASSETCHAINS_ENDSUBSIDY[curEra] != 0 )
                        {
                            if ( curEra == ASSETCHAINS_LASTERA )
                            {
                                subsidyDifference = subsidy;
                            }
                            else
                            {
                                // Ex: -ac_eras=3 -ac_reward=0,384,24 -ac_end=1440,260640,0 -ac_halving=1,1440,2103840 -ac_decay 100000000,97750000,0
                                subsidyDifference = subsidy - ASSETCHAINS_REWARD[curEra + 1];
                                if (subsidyDifference < 0)
                                {
                                    sign = -1;
                                    subsidyDifference *= sign;
                                }
                            }
                            denominator = ASSETCHAINS_ENDSUBSIDY[curEra] - nStart;
                            numerator = denominator - ((ASSETCHAINS_ENDSUBSIDY[curEra] - nHeight) + ((nHeight - nStart) % ASSETCHAINS_HALVING[curEra]));
                            subsidy = subsidy - sign * ((subsidyDifference * numerator) / denominator);
                        }
                        else
                        {
                            if ( cached_subsidy > 0 && cached_era == curEra && cached_numhalvings == numhalvings )
                                subsidy = cached_subsidy;
                            else
                            {
                                for (int i=0; i < numhalvings && subsidy != 0; i++)
                                    subsidy = (subsidy * ASSETCHAINS_DECAY[curEra]) / 100000000;
                                cached_subsidy = subsidy;
                                cached_numhalvings = numhalvings;
                                cached_era = curEra;
                            }
                        }
                    }
                }
            }
        }
    }
    uint32_t magicExtra = ASSETCHAINS_STAKED ? ASSETCHAINS_MAGIC : (ASSETCHAINS_MAGIC & 0xffffff);
    if ( ASSETCHAINS_SUPPLY > 10000000000 ) // over 10 billion?
    {
        if ( nHeight <= ASSETCHAINS_SUPPLY/1000000000 )
        {
            subsidy += (uint64_t)1000000000 * COIN;
            if ( nHeight == 1 )
                subsidy += (ASSETCHAINS_SUPPLY % 1000000000)*COIN + magicExtra;
        }
    }
    else if ( nHeight == 1 )
    {
        if ( ASSETCHAINS_LASTERA == 0 )
            subsidy = ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
        else
            subsidy += ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
    }
    else if ( is_STAKED(chainName.symbol()) == 2 )
        return(0);
    // LABS fungible chains, cannot have any block reward!
    return(subsidy);
}

//void SplitStr(const std::string& strVal, std::vector<std::string> &outVals);

int8_t equihash_params_possible(uint64_t n, uint64_t k)
{
    /* To add more of these you also need to edit:
    * equihash.cpp very end of file with the tempate to point to the new param numbers 
    * equihash.h
    *  line 210/217 (declaration of equihash class)
    * Add this object to the following functions: 
    *  EhInitialiseState 
    *  EhBasicSolve
    *  EhOptimisedSolve
    *  EhIsValidSolution
    * Alternatively change ASSETCHAINS_N and ASSETCHAINS_K in komodo_nk.h for fast testing.
    */
    if ( k == 9 && (n == 200 || n == 210) )
        return(0);
    if ( k == 5 && (n == 150 || n == 144 || n == 96 || n == 48) )
        return(0);
    if ( k == ASSETCHAINS_K && n == ASSETCHAINS_N)
        return(0);
    return(-1);
}

/***
 * @brief get username, password, and port from a config file
 * @param[in] path the path to the data directory
 * @param[in] filename the filename of the config file (without directory)
 * @param[out] userpass the username and password from the config file (colon separated)
 * @param[out] port the RPC port found in the config file
 */
void get_userpass_and_port(const boost::filesystem::path& path, const std::string& filename, 
        std::string& userpass, uint16_t& port)
{
    userpass = "";
    port = 0;
    boost::filesystem::path datadir_path = path;
    datadir_path /= filename;
    FILE* fp = fopen(datadir_path.string().c_str(), "rb");
    if ( fp != nullptr )
    {
        char username[512];
        char password[4096];
        port = _komodo_userpass(username,password,fp);
        if ( username[0] != 0 && password[0] != 0 )
            userpass = std::string(username) + ":" + std::string(password);
        fclose(fp);
    } 
    else 
        LogPrintf("couldnt open.(%s) will not validate dest notarizations\n", datadir_path.string().c_str());
}

/****
 * @brief set ports and usernames/passwords from command line and/or config files
 * @note modifies ASSETCHAINS_P2PPORT, ASSETCHAINS_RPCPORT, KMDUSERPASS, BTCUSERPASS, DESTPORT
 * @note IS_KOMODO_NOTARY should already be set
 * @param ltc_config_filename configuration file for ltc (via -notary command line parameter)
 */
void set_kmd_user_password_port(const std::string& ltc_config_filename)
{
    ASSETCHAINS_P2PPORT = 7770; // default port for P2P
    ASSETCHAINS_RPCPORT = 7771; // default port for RPC
#ifdef __APPLE__
    std::string filename = "Komodo.conf";
#else
    std::string filename = "komodo.conf";
#endif

    auto datadir_path = GetDataDir();
    uint16_t ignore;
    std::string userpass;
    get_userpass_and_port(datadir_path, filename, userpass, ignore);
    if (!userpass.empty())
        strncpy(KMDUSERPASS, userpass.c_str(), 8705);
    if (IS_KOMODO_NOTARY)
    {
        auto approot_path = GetAppDir();  // go to app root dir
        get_userpass_and_port(approot_path, ltc_config_filename, userpass, DEST_PORT);
        if (!userpass.empty())
            strncpy(BTCUSERPASS, userpass.c_str(), 8192);
    }
}

void komodo_args(char *argv0)
{
    uint8_t disablebits[32];
    uint8_t *extraptr=nullptr;
    FILE *fp; 
    uint16_t nonz=0; // keep track of # CCs enabled
    int32_t extralen = 0; 

    const std::string ntz_dest_path = GetArg("-notary", "");
    IS_KOMODO_NOTARY = ntz_dest_path == "" ? 0 : 1;


    STAKED_NOTARY_ID = GetArg("-stakednotary", -1);
    KOMODO_NSPV = GetArg("-nSPV",0);
    memset(disablebits,0,sizeof(disablebits)); // everything enabled
    if ( GetBoolArg("-gen", false) != 0 )
    {
        KOMODO_MININGTHREADS = GetArg("-genproclimit",-1);
    }
    IS_MODE_EXCHANGEWALLET = GetBoolArg("-exchange", false);
    if ( IS_MODE_EXCHANGEWALLET )
        LogPrintf("IS_MODE_EXCHANGEWALLET mode active\n");
    DONATION_PUBKEY = GetArg("-donation", "");
    NOTARY_PUBKEY = GetArg("-pubkey", "");
    IS_KOMODO_DEALERNODE = GetArg("-dealer",0);
    IS_KOMODO_TESTNODE = GetArg("-testnode",0);
    ASSETCHAINS_STAKED_SPLIT_PERCENTAGE = GetArg("-splitperc",0);
    if ( strlen(NOTARY_PUBKEY.c_str()) == 66 )
    {
        decode_hex(NOTARY_PUBKEY33,33,(char *)NOTARY_PUBKEY.c_str());
        USE_EXTERNAL_PUBKEY = 1;
        if ( !IS_KOMODO_NOTARY )
        {
            // We dont have any chain data yet, so use system clock to guess. 
            // I think on season change should reccomend notaries to use -notary to avoid needing this. 
            int32_t kmd_season = getacseason(time(NULL));
            for (uint16_t i=0; i<64; i++)
            {
                if ( strcmp(NOTARY_PUBKEY.c_str(),notaries_elected[kmd_season-1][i][1]) == 0 )
                {
                    IS_KOMODO_NOTARY = true;
                    KOMODO_MININGTHREADS = 1;
                    mapArgs ["-genproclimit"] = itostr(KOMODO_MININGTHREADS);
                    STAKED_NOTARY_ID = -1;
                    LogPrintf("running as notary.%d %s\n",i,notaries_elected[kmd_season-1][i][0]);
                    break;
                }
            }
        }
    }
    if ( STAKED_NOTARY_ID != -1 && IS_KOMODO_NOTARY == true ) {
        LogPrintf( "Cannot be STAKED and KMD notary at the same time!\n");
        StartShutdown();
    }
	std::string name = GetArg("-ac_name","");
    if ( argv0 != 0 )
    {
        int32_t len = (int32_t)strlen(argv0);
        for (unsigned long i=0; i<sizeof(argv0suffix)/sizeof(*argv0suffix); i++)
        {
            int32_t n = (int32_t)strlen(argv0suffix[i]);
            if ( strcmp(&argv0[len - n],argv0suffix[i]) == 0 )
            {
                //LogPrintf("ARGV0.(%s) -> matches suffix (%s) -> ac_name.(%s)\n",argv0,argv0suffix[i],argv0names[i]);
                name = argv0names[i];
                break;
            }
        }
    }
    chainName = assetchain(name);
    KOMODO_STOPAT = GetArg("-stopat",0);
    MAX_REORG_LENGTH = GetArg("-maxreorg",MAX_REORG_LENGTH);
    WITNESS_CACHE_SIZE = MAX_REORG_LENGTH+10;
    ASSETCHAINS_CC = GetArg("-ac_cc",0);
    KOMODO_CCACTIVATE = GetArg("-ac_ccactivate",0);
    ASSETCHAINS_BLOCKTIME = GetArg("-ac_blocktime",60);
    ASSETCHAINS_PUBLIC = GetArg("-ac_public",0);
    ASSETCHAINS_PRIVATE = GetArg("-ac_private",0);
    KOMODO_SNAPSHOT_INTERVAL = GetArg("-ac_snapshot",0);
    Split(GetArg("-ac_nk",""), sizeof(ASSETCHAINS_NK)/sizeof(*ASSETCHAINS_NK), ASSETCHAINS_NK, 0);
    
    // -ac_ccactivateht=evalcode,height,evalcode,height,evalcode,height....
    uint64_t ccEnablesHeight[512];
    memset(ccEnablesHeight, 0, sizeof(ccEnablesHeight));
    Split(GetArg("-ac_ccactivateht",""), sizeof(ccEnablesHeight)/sizeof(*ccEnablesHeight), ccEnablesHeight, 0);
    // fill map with all eval codes and activation height of 0.
    for ( int i = 0; i < 256; i++ )
        mapHeightEvalActivate[i] = 0;
    for ( int i = 0; i < 512; i++ )
    {
        int32_t ecode = ccEnablesHeight[i];
        int32_t ht = ccEnablesHeight[i+1];
        if ( i > 1 && ccEnablesHeight[i-2] == ecode )
            break;
        if ( ecode > 255 || ecode < 0 )
            LogPrintf( "ac_ccactivateht: invalid evalcode.%i must be between 0 and 256.\n", ecode);
        else if ( ht > 0 )
        {
            // update global map. 
            mapHeightEvalActivate[ecode] = ht;
            LogPrintf( "ac_ccactivateht: ecode.%i activates at height.%i\n", ecode, mapHeightEvalActivate[ecode]);
        }
        i++;
    }
    
    if ( (KOMODO_REWIND= GetArg("-rewind",0)) != 0 )
    {
        LogPrintf("KOMODO_REWIND %d\n",KOMODO_REWIND);
    }
    KOMODO_EARLYTXID = Parseuint256(GetArg("-earlytxid","0").c_str());    
    ASSETCHAINS_EARLYTXIDCONTRACT = GetArg("-ac_earlytxidcontract",0);

    if ( !chainName.isKMD() )
    {
        std::string selectedAlgo = GetArg("-ac_algo", std::string(ASSETCHAINS_ALGORITHMS[0]));

        uint32_t i;
        for ( i = 0; i < ASSETCHAINS_NUMALGOS; i++ )
        {
            if (std::string(ASSETCHAINS_ALGORITHMS[i]) == selectedAlgo)
            {
                ASSETCHAINS_ALGO = i;
                STAKING_MIN_DIFF = ASSETCHAINS_MINDIFF[i];
                // only worth mentioning if it's not equihash
                if (ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH)
                    LogPrintf("ASSETCHAINS_ALGO, algorithm set to %s\n", selectedAlgo.c_str());
                break;
            }
        }
        if ( ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH && ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
        {
            if ( equihash_params_possible(ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]) == -1 ) 
            {
                LogPrintf("equihash values N.%li and K.%li are not currently available\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
                exit(0);
            } else LogPrintf("ASSETCHAINS_ALGO, algorithm set to equihash with N.%li and K.%li\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
        }
        if (i == ASSETCHAINS_NUMALGOS)
        {
            LogPrintf("ASSETCHAINS_ALGO, %s not supported. using equihash\n", selectedAlgo.c_str());
        }

        ASSETCHAINS_LASTERA = GetArg("-ac_eras", 1);
        if ( ASSETCHAINS_LASTERA < 1 || ASSETCHAINS_LASTERA > ASSETCHAINS_MAX_ERAS )
        {
            ASSETCHAINS_LASTERA = 1;
            LogPrintf("ASSETCHAINS_LASTERA, if specified, must be between 1 and %u. ASSETCHAINS_LASTERA set to %lu\n", ASSETCHAINS_MAX_ERAS, ASSETCHAINS_LASTERA);
        }
        ASSETCHAINS_LASTERA -= 1;

        ASSETCHAINS_TIMELOCKGTE = (uint64_t)GetArg("-ac_timelockgte", _ASSETCHAINS_TIMELOCKOFF);
        ASSETCHAINS_TIMEUNLOCKFROM = GetArg("-ac_timeunlockfrom", 0);
        ASSETCHAINS_TIMEUNLOCKTO = GetArg("-ac_timeunlockto", 0);
        if ( ASSETCHAINS_TIMEUNLOCKFROM > ASSETCHAINS_TIMEUNLOCKTO )
        {
            LogPrintf("ASSETCHAINS_TIMELOCKGTE - must specify valid ac_timeunlockfrom and ac_timeunlockto\n");
            ASSETCHAINS_TIMELOCKGTE = _ASSETCHAINS_TIMELOCKOFF;
            ASSETCHAINS_TIMEUNLOCKFROM = ASSETCHAINS_TIMEUNLOCKTO = 0;
        }

        Split(GetArg("-ac_end",""), sizeof(ASSETCHAINS_ENDSUBSIDY)/sizeof(*ASSETCHAINS_ENDSUBSIDY),  ASSETCHAINS_ENDSUBSIDY, 0);
        Split(GetArg("-ac_reward",""), sizeof(ASSETCHAINS_REWARD)/sizeof(*ASSETCHAINS_REWARD),  ASSETCHAINS_REWARD, 0);
        Split(GetArg("-ac_halving",""), sizeof(ASSETCHAINS_HALVING)/sizeof(*ASSETCHAINS_HALVING),  ASSETCHAINS_HALVING, 0);
        Split(GetArg("-ac_decay",""), sizeof(ASSETCHAINS_DECAY)/sizeof(*ASSETCHAINS_DECAY),  ASSETCHAINS_DECAY, 0);
        Split(GetArg("-ac_notarypay",""), sizeof(ASSETCHAINS_NOTARY_PAY)/sizeof(*ASSETCHAINS_NOTARY_PAY),  ASSETCHAINS_NOTARY_PAY, 0);

        for ( int i = 0; i < ASSETCHAINS_MAX_ERAS; i++ )
        {
            if ( ASSETCHAINS_DECAY[i] == 100000000 && ASSETCHAINS_ENDSUBSIDY[i] == 0 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                LogPrintf("ERA%u: ASSETCHAINS_DECAY of 100000000 means linear and that needs ASSETCHAINS_ENDSUBSIDY\n", i);
            }
            else if ( ASSETCHAINS_DECAY[i] > 100000000 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                LogPrintf("ERA%u: ASSETCHAINS_DECAY cant be more than 100000000\n", i);
            }
        }

        MAX_BLOCK_SIGOPS = 60000;
        ASSETCHAINS_TXPOW = GetArg("-ac_txpow",0) & 3;
        ASSETCHAINS_FOUNDERS = GetArg("-ac_founders",0);// & 1;
		ASSETCHAINS_FOUNDERS_REWARD = GetArg("-ac_founders_reward",0);
        ASSETCHAINS_SUPPLY = GetArg("-ac_supply",10);
        if ( ASSETCHAINS_SUPPLY > (uint64_t)90*1000*1000000 )
        {
            LogPrintf("-ac_supply must be less than 90 billion\n");
            StartShutdown();
        }
        LogPrintf("ASSETCHAINS_SUPPLY %llu\n",(long long)ASSETCHAINS_SUPPLY);
        
        ASSETCHAINS_COMMISSION = GetArg("-ac_perc",0);
        ASSETCHAINS_OVERRIDE_PUBKEY = GetArg("-ac_pubkey","");
        ASSETCHAINS_SCRIPTPUB = GetArg("-ac_script","");
        ASSETCHAINS_BEAMPORT = GetArg("-ac_beam",0);
        ASSETCHAINS_CODAPORT = GetArg("-ac_coda",0);
        ASSETCHAINS_CBMATURITY = GetArg("-ac_cbmaturity",0);
        ASSETCHAINS_ADAPTIVEPOW = GetArg("-ac_adaptivepow",0);
        std::string hexstr = GetArg("-ac_mineropret","");
        std::vector<uint8_t> Mineropret;
        if ( hexstr.size() != 0 )
        {
            Mineropret.resize(hexstr.size()/2);
            decode_hex(Mineropret.data(),hexstr.size()/2,(char *)hexstr.c_str());
            for (size_t i=0; i<Mineropret.size(); i++)
                LogPrintf("%02x",Mineropret[i]);
            LogPrintf(" Mineropret\n");
        }
        if ( ASSETCHAINS_COMMISSION != 0 && ASSETCHAINS_FOUNDERS_REWARD != 0 )
        {
            LogPrintf("cannot use founders reward and commission on the same chain.\n");
            StartShutdown();
        }
        if ( ASSETCHAINS_CC != 0 )
        {
            uint8_t prevCCi = 0;
            ASSETCHAINS_CCLIB = GetArg("-ac_cclib","");
            uint64_t ccenables[256];
            memset(ccenables, 0, sizeof(ccenables) );
            Split(GetArg("-ac_ccenable",""), sizeof(ccenables)/sizeof(*ccenables),  ccenables, 0);
            for (uint16_t i=0; i<256; i++)
            {
                if ( ccenables[i] != prevCCi && ccenables[i] != 0 )
                {
                    nonz++;
                    prevCCi = ccenables[i];
                    LogPrintf("%d ",(uint8_t)(ccenables[i] & 0xff));
                }
            }
            LogPrintf("nonz.%d ccenables[]\n",nonz);
            if ( nonz > 0 )
            {
                // disable all CCs
                for (uint16_t i=0; i<256; i++)
                {
                    ASSETCHAINS_CCDISABLES[i] = 1;
                    SETBIT(disablebits,i);
                }
                // enable chosen CCs
                for (uint16_t i=0; i<nonz; i++)
                {
                    CLEARBIT(disablebits,(ccenables[i] & 0xff));
                    ASSETCHAINS_CCDISABLES[ccenables[i] & 0xff] = 0;
                }
                CLEARBIT(disablebits,0); // always enable contract 0. Why is this here? -JMJ
            }
        }
        if ( ASSETCHAINS_BEAMPORT != 0 )
        {
            LogPrintf("can only have one of -ac_beam or -ac_coda\n");
            StartShutdown();
        }
        ASSETCHAINS_SELFIMPORT = GetArg("-ac_import",""); // BEAM, CODA, PUBKEY, GATEWAY
        if ( ASSETCHAINS_SELFIMPORT == "PUBKEY" )
        {
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) != 66 )
            {
                LogPrintf("invalid -ac_pubkey for -ac_import=PUBKEY\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "BEAM" )
        {
            if (ASSETCHAINS_BEAMPORT == 0)
            {
                LogPrintf("missing -ac_beam for BEAM rpcport\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "CODA" )
        {
            if (ASSETCHAINS_CODAPORT == 0)
            {
                LogPrintf("missing -ac_coda for CODA rpcport\n");
                StartShutdown();
            }
        }
        // else it can be gateway coin
        else if (!ASSETCHAINS_SELFIMPORT.empty() && (ASSETCHAINS_ENDSUBSIDY[0]!=1 || ASSETCHAINS_SUPPLY>0 || ASSETCHAINS_COMMISSION!=0))
        {
            LogPrintf("when using gateway import these must be set: -ac_end=1 -ac_supply=0 -ac_perc=0\n");
            StartShutdown();
        }
        

        if ( (ASSETCHAINS_STAKED= GetArg("-ac_staked",0)) > 100 )
            ASSETCHAINS_STAKED = 100;

        ASSETCHAINS_SAPLING = GetArg("-ac_sapling", -1);
        if (ASSETCHAINS_SAPLING == -1)
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", -1);
        }
        else
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", ASSETCHAINS_SAPLING);
        }
        if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 || ASSETCHAINS_SCRIPTPUB.size() > 1 )
        {
            if ( ASSETCHAINS_SUPPLY > 10000000000 )
            {
                LogPrintf("ac_pubkey or ac_script wont work with ac_supply over 10 billion\n");
                StartShutdown();
            }
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
            {
                LogPrintf("Assetchains NOTARY PAY cannot be used with ac_pubkey or ac_script.\n");
                StartShutdown();
            }
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 )
            {
                decode_hex(ASSETCHAINS_OVERRIDE_PUBKEY33,33,(char *)ASSETCHAINS_OVERRIDE_PUBKEY.c_str());
                calc_rmd160_sha256(ASSETCHAINS_OVERRIDE_PUBKEYHASH,ASSETCHAINS_OVERRIDE_PUBKEY33,33);
            }
            if ( ASSETCHAINS_COMMISSION == 0 && ASSETCHAINS_FOUNDERS != 0 )
            {
                if ( ASSETCHAINS_FOUNDERS_REWARD == 0 )
                {
                    ASSETCHAINS_COMMISSION = 53846154; // maps to 35%
                    LogPrintf("ASSETCHAINS_COMMISSION defaulted to 35%% when founders reward active\n");
                }
                else
                {
                    LogPrintf("ASSETCHAINS_FOUNDERS_REWARD set to %ld\n", ASSETCHAINS_FOUNDERS_REWARD);
                }
                /*else if ( ASSETCHAINS_SELFIMPORT.size() == 0 )
                {
                    //ASSETCHAINS_OVERRIDE_PUBKEY.clear();
                    LogPrintf("-ac_perc must be set with -ac_pubkey\n");
                }*/
            }
        }
        else
        {
            if ( ASSETCHAINS_COMMISSION != 0 )
            {
                ASSETCHAINS_COMMISSION = 0;
                LogPrintf("ASSETCHAINS_COMMISSION needs an ASSETCHAINS_OVERRIDE_PUBKEY and cant be more than 100000000 (100%%)\n");
            }
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                ASSETCHAINS_FOUNDERS = 0;
                LogPrintf("ASSETCHAINS_FOUNDERS needs an ASSETCHAINS_OVERRIDE_PUBKEY or ASSETCHAINS_SCRIPTPUB\n");
            }
        }
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 
                || ASSETCHAINS_REWARD[0] != 0 
                || ASSETCHAINS_HALVING[0] != 0 
                || ASSETCHAINS_DECAY[0] != 0 
                || ASSETCHAINS_COMMISSION != 0 
                || ASSETCHAINS_PUBLIC != 0 
                || ASSETCHAINS_PRIVATE != 0 
                || ASSETCHAINS_TXPOW != 0 
                || ASSETCHAINS_FOUNDERS != 0 
                || ASSETCHAINS_SCRIPTPUB.size() > 1 
                || ASSETCHAINS_SELFIMPORT.size() > 0 
                || ASSETCHAINS_OVERRIDE_PUBKEY33[0] != 0 
                || ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF
                || ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH 
                || ASSETCHAINS_LASTERA > 0 
                || ASSETCHAINS_BEAMPORT != 0 
                || ASSETCHAINS_CODAPORT != 0 
                || nonz > 0 
                || ASSETCHAINS_CCLIB.size() > 0 
                || ASSETCHAINS_FOUNDERS_REWARD != 0 
                || ASSETCHAINS_NOTARY_PAY[0] != 0 
                || ASSETCHAINS_BLOCKTIME != 60 
                || Mineropret.size() != 0 
                || (ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0) 
                || KOMODO_SNAPSHOT_INTERVAL != 0 
                || ASSETCHAINS_EARLYTXIDCONTRACT != 0 
                || ASSETCHAINS_CBMATURITY != 0 
                || ASSETCHAINS_ADAPTIVEPOW != 0 )
        {
            uint8_t extrabuf[32756];
            LogPrintf("perc %.4f%% ac_pub=[%02x%02x%02x...] acsize.%d\n",dstr(ASSETCHAINS_COMMISSION)*100,ASSETCHAINS_OVERRIDE_PUBKEY33[0],ASSETCHAINS_OVERRIDE_PUBKEY33[1],ASSETCHAINS_OVERRIDE_PUBKEY33[2],(int32_t)ASSETCHAINS_SCRIPTPUB.size());
            extraptr = extrabuf;
            memcpy(extraptr,ASSETCHAINS_OVERRIDE_PUBKEY33,33), extralen = 33;

            // if we have one era, this should create the same data structure as it used to, same if we increase _MAX_ERAS
            for ( int i = 0; i <= ASSETCHAINS_LASTERA; i++ )
            {
                LogPrintf("ERA%u: end.%llu reward.%llu halving.%llu decay.%llu notarypay.%llu\n", i,
                       (long long)ASSETCHAINS_ENDSUBSIDY[i],
                       (long long)ASSETCHAINS_REWARD[i],
                       (long long)ASSETCHAINS_HALVING[i],
                       (long long)ASSETCHAINS_DECAY[i],
                       (long long)ASSETCHAINS_NOTARY_PAY[i]);

                // TODO: Verify that we don't overrun extrabuf here, which is a 256 byte buffer
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ENDSUBSIDY[i]),(void *)&ASSETCHAINS_ENDSUBSIDY[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_REWARD[i]),(void *)&ASSETCHAINS_REWARD[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_HALVING[i]),(void *)&ASSETCHAINS_HALVING[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_DECAY[i]),(void *)&ASSETCHAINS_DECAY[i]);
                if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NOTARY_PAY[i]),(void *)&ASSETCHAINS_NOTARY_PAY[i]);
            }

            if (ASSETCHAINS_LASTERA > 0)
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_LASTERA),(void *)&ASSETCHAINS_LASTERA);
            }

            // hash in lock above for time locked coinbase transactions above a certain reward value only if the lock above
            // param was specified, otherwise, for compatibility, do nothing
            if ( ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMELOCKGTE),(void *)&ASSETCHAINS_TIMELOCKGTE);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKFROM),(void *)&ASSETCHAINS_TIMEUNLOCKFROM);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKTO),(void *)&ASSETCHAINS_TIMEUNLOCKTO);
            }

            if ( ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ALGO),(void *)&ASSETCHAINS_ALGO);
            }

            uint64_t val = ASSETCHAINS_COMMISSION | (((int64_t)ASSETCHAINS_STAKED & 0xff) << 32) | (((uint64_t)ASSETCHAINS_CC & 0xffff) << 40) | ((ASSETCHAINS_PUBLIC != 0) << 7) | ((ASSETCHAINS_PRIVATE != 0) << 6) | ASSETCHAINS_TXPOW;
            extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(val),(void *)&val);
            
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                uint8_t tmp = 1;
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(tmp),(void *)&tmp);
                if ( ASSETCHAINS_FOUNDERS > 1 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS),(void *)&ASSETCHAINS_FOUNDERS);
                if ( ASSETCHAINS_FOUNDERS_REWARD != 0 )
                {
                    LogPrintf( "set founders reward.%lld\n",(long long)ASSETCHAINS_FOUNDERS_REWARD);
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS_REWARD),(void *)&ASSETCHAINS_FOUNDERS_REWARD);
                }
            }
            if ( ASSETCHAINS_SCRIPTPUB.size() > 1 )
            {
                decode_hex(&extraptr[extralen],ASSETCHAINS_SCRIPTPUB.size()/2,(char *)ASSETCHAINS_SCRIPTPUB.c_str());
                extralen += ASSETCHAINS_SCRIPTPUB.size()/2;
                LogPrintf("append ac_script %s\n",ASSETCHAINS_SCRIPTPUB.c_str());
            }
            if ( ASSETCHAINS_SELFIMPORT.size() > 0 )
            {
                memcpy(&extraptr[extralen],(char *)ASSETCHAINS_SELFIMPORT.c_str(),ASSETCHAINS_SELFIMPORT.size());
                for (size_t i=0; i<ASSETCHAINS_SELFIMPORT.size(); i++)
                    LogPrintf("%c",extraptr[extralen+i]);
                LogPrintf(" selfimport\n");
                extralen += ASSETCHAINS_SELFIMPORT.size();
            }
            if ( ASSETCHAINS_BEAMPORT != 0 )
                extraptr[extralen++] = 'b';
            if ( ASSETCHAINS_CODAPORT != 0 )
                extraptr[extralen++] = 'c';
            LogPrintf("extralen.%d before disable bits\n",extralen);
            if ( nonz > 0 )
            {
                memcpy(&extraptr[extralen],disablebits,sizeof(disablebits));
                extralen += sizeof(disablebits);
            }
            if ( ASSETCHAINS_CCLIB.size() > 1 )
            {
                for (size_t i=0; i<ASSETCHAINS_CCLIB.size(); i++)
                {
                    extraptr[extralen++] = ASSETCHAINS_CCLIB[i];
                    LogPrintf("%c",ASSETCHAINS_CCLIB[i]);
                }
                LogPrintf(" <- CCLIB name\n");
            }
            if ( ASSETCHAINS_BLOCKTIME != 60 )
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_BLOCKTIME),(void *)&ASSETCHAINS_BLOCKTIME);
            if ( Mineropret.size() != 0 )
            {
                for (size_t i=0; i<Mineropret.size(); i++)
                    extraptr[extralen++] = Mineropret[i];
            }
            if ( ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[0]),(void *)&ASSETCHAINS_NK[0]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[1]),(void *)&ASSETCHAINS_NK[1]);
            }
            if ( KOMODO_SNAPSHOT_INTERVAL != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(KOMODO_SNAPSHOT_INTERVAL),(void *)&KOMODO_SNAPSHOT_INTERVAL);
            }
            if ( ASSETCHAINS_EARLYTXIDCONTRACT != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_EARLYTXIDCONTRACT),(void *)&ASSETCHAINS_EARLYTXIDCONTRACT);
            }
            if ( ASSETCHAINS_CBMATURITY != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_CBMATURITY),(void *)&ASSETCHAINS_CBMATURITY);
            }
            if ( ASSETCHAINS_ADAPTIVEPOW != 0 )
                extraptr[extralen++] = ASSETCHAINS_ADAPTIVEPOW;
        }
        
        //addn = GetArg("-seednode","");
        //if ( strlen(addn.c_str()) > 0 )
        //    ASSETCHAINS_SEED = 1;

        MAX_MONEY = komodo_max_money();
        int32_t baseid;
        if ( (baseid = komodo_baseid(chainName.symbol().c_str())) >= 0 && baseid < 32 )
        {
            LogPrintf("baseid.%d MAX_MONEY.%s %.8f\n",baseid,chainName.symbol().c_str(),(double)MAX_MONEY/SATOSHIDEN);
        }

        if ( ASSETCHAINS_CC >= KOMODO_FIRSTFUNGIBLEID && MAX_MONEY < 1000000LL*SATOSHIDEN )
            MAX_MONEY = 1000000LL*SATOSHIDEN;
        if ( KOMODO_BIT63SET(MAX_MONEY) != 0 )
            MAX_MONEY = KOMODO_MAXNVALUE;
        LogPrintf("MAX_MONEY %llu %.8f\n",(long long)MAX_MONEY,(double)MAX_MONEY/SATOSHIDEN);
        uint16_t tmpport = komodo_port(chainName.symbol().c_str(),ASSETCHAINS_SUPPLY,&ASSETCHAINS_MAGIC,extraptr,extralen);
        if ( GetArg("-port",0) != 0 )
        {
            ASSETCHAINS_P2PPORT = GetArg("-port",0);
            LogPrintf("set p2pport.%u\n",ASSETCHAINS_P2PPORT);
        } 
        else 
            ASSETCHAINS_P2PPORT = tmpport;

        char* dirname = nullptr;
        while ( (dirname= (char *)GetDataDir(false).string().c_str()) == 0 || dirname[0] == 0 )
        {
            LogPrintf("waiting for datadir (%s)\n",dirname);
#ifndef _WIN32
            sleep(3);
#else
            boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
#endif
        }
        if ( !chainName.isKMD() )
        {
            if ( chainName.isSymbol("KMD") )
            {
                LogPrintf("can't have assetchain named KMD\n");
                StartShutdown();
            }
            uint16_t port;
            if ( (port= komodo_userpass(ASSETCHAINS_USERPASS,chainName.symbol().c_str())) != 0 )
                ASSETCHAINS_RPCPORT = port;
            else komodo_configfile(chainName.symbol().c_str(),ASSETCHAINS_P2PPORT + 1);

            if (ASSETCHAINS_CBMATURITY != 0)
                Params().SetCoinbaseMaturity(ASSETCHAINS_CBMATURITY);
            else if (ASSETCHAINS_LASTERA == 0 || is_STAKED(chainName.symbol()) != 0)
                Params().SetCoinbaseMaturity(1);
            if (Params().CoinbaseMaturity() < 1)
            {
                LogPrintf("ac_cbmaturity must be >0, shutting down\n");
                StartShutdown();
            }
        }
        if ( ASSETCHAINS_RPCPORT == 0 )
            ASSETCHAINS_RPCPORT = ASSETCHAINS_P2PPORT + 1;
        uint8_t magic[4];
        iguana_rwnum(1,magic,sizeof(ASSETCHAINS_MAGIC),(void *)&ASSETCHAINS_MAGIC);
        char magicstr[9];
        for (i=0; i<4; i++)
            sprintf(&magicstr[i<<1],"%02x",magic[i]);
        magicstr[8] = 0;
#ifndef FROM_CLI
        char fname[512];
        sprintf(fname,"%s_7776",chainName.symbol().c_str());
        if ( (fp= fopen(fname,"wb")) != 0 )
        {
            int8_t notarypay = 0;
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                notarypay = 1;
            char *iguanafmtstr = (char *)"curl --url \"http://127.0.0.1:7776\" --data \"{\\\"conf\\\":\\\"%s.conf\\\",\\\"path\\\":\\\"${HOME#\"/\"}/.komodo/%s\\\",\\\"unitval\\\":\\\"20\\\",\\\"zcash\\\":1,\\\"RELAY\\\":-1,\\\"VALIDATE\\\":0,\\\"prefetchlag\\\":-1,\\\"poll\\\":100,\\\"active\\\":1,\\\"agent\\\":\\\"iguana\\\",\\\"method\\\":\\\"addcoin\\\",\\\"startpend\\\":4,\\\"endpend\\\":4,\\\"services\\\":129,\\\"maxpeers\\\":8,\\\"newcoin\\\":\\\"%s\\\",\\\"name\\\":\\\"%s\\\",\\\"hasheaders\\\":1,\\\"useaddmultisig\\\":0,\\\"netmagic\\\":\\\"%s\\\",\\\"p2p\\\":%u,\\\"rpc\\\":%u,\\\"pubval\\\":60,\\\"p2shval\\\":85,\\\"wifval\\\":188,\\\"txfee_satoshis\\\":\\\"10000\\\",\\\"isPoS\\\":0,\\\"minoutput\\\":10000,\\\"minconfirms\\\":2,\\\"genesishash\\\":\\\"027e3758c3a65b12aa1046462b486d0a63bfa1beae327897f56c5cfb7daaae71\\\",\\\"protover\\\":170002,\\\"genesisblock\\\":\\\"0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a000000000000000000000000000000000000000000000000000000000000000029ab5f490f0f0f200b00000000000000000000000000000000000000000000000000000000000000fd4005000d5ba7cda5d473947263bf194285317179d2b0d307119c2e7cc4bd8ac456f0774bd52b0cd9249be9d40718b6397a4c7bbd8f2b3272fed2823cd2af4bd1632200ba4bf796727d6347b225f670f292343274cc35099466f5fb5f0cd1c105121b28213d15db2ed7bdba490b4cedc69742a57b7c25af24485e523aadbb77a0144fc76f79ef73bd8530d42b9f3b9bed1c135ad1fe152923fafe98f95f76f1615e64c4abb1137f4c31b218ba2782bc15534788dda2cc08a0ee2987c8b27ff41bd4e31cd5fb5643dfe862c9a02ca9f90c8c51a6671d681d04ad47e4b53b1518d4befafefe8cadfb912f3d03051b1efbf1dfe37b56e93a741d8dfd80d576ca250bee55fab1311fc7b3255977558cdda6f7d6f875306e43a14413facdaed2f46093e0ef1e8f8a963e1632dcbeebd8e49fd16b57d49b08f9762de89157c65233f60c8e38a1f503a48c555f8ec45dedecd574a37601323c27be597b956343107f8bd80f3a925afaf30811df83c402116bb9c1e5231c70fff899a7c82f73c902ba54da53cc459b7bf1113db65cc8f6914d3618560ea69abd13658fa7b6af92d374d6eca9529f8bd565166e4fcbf2a8dfb3c9b69539d4d2ee2e9321b85b331925df195915f2757637c2805e1d4131e1ad9ef9bc1bb1c732d8dba4738716d351ab30c996c8657bab39567ee3b29c6d054b711495c0d52e1cd5d8e55b4f0f0325b97369280755b46a02afd54be4ddd9f77c22272b8bbb17ff5118fedbae2564524e797bd28b5f74f7079d532ccc059807989f94d267f47e724b3f1ecfe00ec9e6541c961080d8891251b84b4480bc292f6a180bea089fef5bbda56e1e41390d7c0e85ba0ef530f7177413481a226465a36ef6afe1e2bca69d2078712b3912bba1a99b1fbff0d355d6ffe726d2bb6fbc103c4ac5756e5bee6e47e17424ebcbf1b63d8cb90ce2e40198b4f4198689daea254307e52a25562f4c1455340f0ffeb10f9d8e914775e37d0edca019fb1b9c6ef81255ed86bc51c5391e0591480f66e2d88c5f4fd7277697968656a9b113ab97f874fdd5f2465e5559533e01ba13ef4a8f7a21d02c30c8ded68e8c54603ab9c8084ef6d9eb4e92c75b078539e2ae786ebab6dab73a09e0aa9ac575bcefb29e930ae656e58bcb513f7e3c17e079dce4f05b5dbc18c2a872b22509740ebe6a3903e00ad1abc55076441862643f93606e3dc35e8d9f2caef3ee6be14d513b2e062b21d0061de3bd56881713a1a5c17f5ace05e1ec09da53f99442df175a49bd154aa96e4949decd52fed79ccf7ccbce32941419c314e374e4a396ac553e17b5340336a1a25c22f9e42a243ba5404450b650acfc826a6e432971ace776e15719515e1634ceb9a4a35061b668c74998d3dfb5827f6238ec015377e6f9c94f38108768cf6e5c8b132e0303fb5a200368f845ad9d46343035a6ff94031df8d8309415bb3f6cd5ede9c135fdabcc030599858d803c0f85be7661c88984d88faa3d26fb0e9aac0056a53f1b5d0baed713c853c4a2726869a0a124a8a5bbc0fc0ef80c8ae4cb53636aa02503b86a1eb9836fcc259823e2692d921d88e1ffc1e6cb2bde43939ceb3f32a611686f539f8f7c9f0bf00381f743607d40960f06d347d1cd8ac8a51969c25e37150efdf7aa4c2037a2fd0516fb444525ab157a0ed0a7412b2fa69b217fe397263153782c0f64351fbdf2678fa0dc8569912dcd8e3ccad38f34f23bbbce14c6a26ac24911b308b82c7e43062d180baeac4ba7153858365c72c63dcf5f6a5b08070b730adb017aeae925b7d0439979e2679f45ed2f25a7edcfd2fb77a8794630285ccb0a071f5cce410b46dbf9750b0354aae8b65574501cc69efb5b6a43444074fee116641bb29da56c2b4a7f456991fc92b2\\\",\\\"debug\\\":0,\\\"seedipaddr\\\":\\\"%s\\\",\\\"sapling\\\":1,\\\"notarypay\\\":%i}\"";
            fprintf(fp,iguanafmtstr,name.c_str(),name.c_str(),name.c_str(),name.c_str(),magicstr,ASSETCHAINS_P2PPORT,ASSETCHAINS_RPCPORT,"78.47.196.146",notarypay);
            fclose(fp);
        } else LogPrintf("error creating (%s)\n",fname);
#endif
        if ( ASSETCHAINS_CC < 2 )
        {
            if ( KOMODO_CCACTIVATE != 0 )
            {
                ASSETCHAINS_CC = 2;
                LogPrintf("smart utxo CC contracts will activate at height.%d\n",KOMODO_CCACTIVATE);
            }
            else if ( ccEnablesHeight[0] != 0 )
            {
                ASSETCHAINS_CC = 2;
                LogPrintf("smart utxo CC contract %d will activate at height.%d\n",(int32_t)ccEnablesHeight[0],(int32_t)ccEnablesHeight[1]);
            }
        }
    }
    else
    {
        // -ac_name not passed, we are on the KMD chain
        set_kmd_user_password_port(ntz_dest_path);
    }
    int32_t dpowconfs = KOMODO_DPOWCONFS;
    if ( !chainName.isKMD() )
    {
        BITCOIND_RPCPORT = GetArg("-rpcport", ASSETCHAINS_RPCPORT);
        //LogPrintf("(%s) port.%u chain params initialized\n",chainName.symbol().c_str(),BITCOIND_RPCPORT);
        if ( chainName.isSymbol("PIRATE") && ASSETCHAINS_HALVING[0] == 77777 )
        {
            ASSETCHAINS_HALVING[0] *= 5;
            LogPrintf("PIRATE halving changed to %d %.1f days ASSETCHAINS_LASTERA.%llu\n",(int32_t)ASSETCHAINS_HALVING[0],(double)ASSETCHAINS_HALVING[0]/1440,(long long)ASSETCHAINS_LASTERA);
        }
        else if ( ASSETCHAINS_PRIVATE != 0 )
        {
            LogPrintf("-ac_private for a non-PIRATE chain is not supported. The only reason to have an -ac_private chain is for total privacy and that is best achieved with the largest anon set. PIRATE has that and it is recommended to just use PIRATE\n");
            StartShutdown();
        }
        // Set cc enables for all existing ac_cc chains here. 
        if ( chainName.isSymbol("AXO") )
        {
            // No CCs used on this chain yet.
            CCDISABLEALL;
        }
        if ( chainName.isSymbol("CCL") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_HEIR);
        }
        if ( chainName.isSymbol("COQUI") )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_CHANNELS);
            CCENABLE(EVAL_ORACLES);
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
        }
        if ( chainName.isSymbol("DION") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        
        if ( chainName.isSymbol("EQL") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( chainName.isSymbol("ILN") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( chainName.isSymbol("OUR") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( chainName.isSymbol("ZEXO") )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( chainName.isSymbol("SEC") )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_ORACLES);
        }
        if ( chainName.isSymbol("KMDICE") )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_FAUCET);
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_ORACLES);
        }
    } 
    else 
        BITCOIND_RPCPORT = GetArg("-rpcport", BaseParams().RPCPort());
    KOMODO_DPOWCONFS = GetArg("-dpowconfs",dpowconfs);
    if ( chainName.isKMD() 
            || chainName.isSymbol("SUPERNET") 
            || chainName.isSymbol("DEX") 
            || chainName.isSymbol("COQUI") 
            || chainName.isSymbol("PIRATE") 
            || chainName.isSymbol("KMDICE") )
        KOMODO_EXTRASATOSHI = 1;
}

void komodo_nameset(char *symbol,char *dest,const char *source)
{
    if ( source[0] == 0 )
    {
        strcpy(symbol,(char *)"KMD");
        strcpy(dest,(char *)"BTC");
    }
    else
    {
        strcpy(symbol,source);
        strcpy(dest,(char *)"KMD");
    }
}

/****
 * @brief get the right komodo_state
 * @param[in] base what to search for (nullptr == "KMD")
 * @returns the correct komodo_state object
 */
komodo_state *komodo_stateptrget(char *base)
{
    // "KMD" case
    if ( base == 0 || base[0] == 0 || strcmp(base,(char *)"KMD") == 0 )
        return &KOMODO_STATES[1];

    // evidently this asset chain
    return &KOMODO_STATES[0];
}

/****
 * @brief get the symbol and dest based on this chain's ASSETCHAINS_SYMBOL
 * @param[out] symbol this chain ("KMD" if ASSETCHAINS_SYMBOL is nullptr)
 * @param[out] dest the destination chain ("BTC" in the case of KMD, otherwise "KMD")
 * @returns the komodo_state object for symbol
 */
komodo_state *komodo_stateptr(char *symbol,char *dest)
{
    int32_t baseid;
    komodo_nameset(symbol,dest,chainName.symbol().c_str());
    return(komodo_stateptrget(symbol));
}

/***
 * @brief prefetch file contents, leave next read position where it started
 * @param fp the file to read
 */
void komodo_prefetch(FILE *fp)
{
    // I am not sure why we do this, perhaps looking for disk errors or
    // disk caching? - JMJ
    int32_t incr = 16*1024*1024;
    long fpos = ftell(fp); // store the current position
    fseek(fp,0,SEEK_END);
    if ( ftell(fp) > incr ) // if the file is greater than 16MB
    {
        char *ignore = (char *)malloc(incr);
        if ( ignore != 0 )
        {
            rewind(fp); // go back to the beginning
            while ( fread(ignore,1,incr,fp) == incr ) // prefetch in 16MB blocks
                LogPrintf(".");
            free(ignore);
        }
    }
    fseek(fp,fpos,SEEK_SET); // go to where we were when this function was called
}

// check if block timestamp is more than S5 activation time
// this function is to activate the ExtractDestination fix 
bool komodo_is_vSolutionsFixActive()
{
    return GetLatestTimestamp(komodo_currentheight()) > nS5Timestamp;
}
