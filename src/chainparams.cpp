// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//

unsigned int pnSeed[] =
{
    0x15c6f1c0, 0x0e4b8368, 0xbf7e1fb0, 0x39bb1dc1, 0xc375bb2f, 0x6831e23e, 0x03398b68, 0x46d9795b,
    0xf5453fb2, 0x133d6d46, 0x03398b68, 0x9dda556a, 0xadda556a, 0x2f41eb6d, 0x1e8f9484, 0x5df8459e,
    0xd841e3a5, 0xe35035ae, 0x99200cb0, 0xbf7e1fb0, 0xad6222b1, 0xf45adbb2, 0xf5453fb2, 0xca81a4b8,
    0x186189b9, 0x1c2d91b9, 0x11cf17bc, 0x7b7569bd, 0x056caabd, 0xe217fcbd, 0x0c2cfcbd, 0x39bb1dc1,
    0x5a3b1bcc, 0x840620d5, 0xd1966517, 0x2523e618, 0xda3a212d, 0xc375bb2f, 0x752dfe33, 0x671cdc34,
    0x6831e23e, 0x133d6d46, 0x66317b50, 0xc7d9da50, 0xd198dc50, 0xb1330750, 0x7ef58d51, 0x40319859,
    0xfd35355c, 0x5d35355c, 0xa80e825e,
};

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xaa;
        pchMessageStart[1] = 0xc3;
        pchMessageStart[2] = 0xc6;
        pchMessageStart[3] = 0xab;
        vAlertPubKey = ParseHex("045a8927ef4e11ac24f4e65de016ae415fb3eb7ac0396b2dcf9f8451d96caf58908f8925811dea37f6c5b5789a2b4f78c7a1b5f56f019554061bbd01924612b4a0");
        nNeoScryptHeight = 120000;
        nNeoScryptFork = 1414482565;
        nDefaultPort = 42954;
        nRPCPort = 42953;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);
        bnNeoScryptSwitch = CBigNum(~uint256(0) >> 28);

        const char* pszTimestamp = "1May launch of guncoin";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1397923502;
        genesis.nBits    = 0x1e0ffff0;
        genesis.nNonce   = 2085244361;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x55a9b3b171a6dfc95d59e07fa23736f44cd745f896e86491982ad6d043c58ba6"));
        assert(genesis.hashMerkleRoot == uint256("0x9279661f2031143951417ef97ea70bcd7172f1bff7831c339c97e0e8392a926a"));

        vSeeds.push_back(CDNSSeedData("guncoin.info", "seed.guncoin.info"));
        vSeeds.push_back(CDNSSeedData("guncoin.info", "seed2.guncoin.info"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(39);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(5);
        base58Prefixes[SECRET_KEY] =     list_of(167);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

        // Convert the pnSeeds array into usable address objects.
        for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
        {
            // It'll only connect to one or two seed nodes because once it connects,
            // it'll get a pile of addresses with newer timestamps.
            // Seed nodes are given a random 'last seen time' of between one and two
            // weeks ago.
            const int64_t nOneWeek = 7*24*60*60;
            struct in_addr ip;
            memcpy(&ip, &pnSeed[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
        }
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet (v3)
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xdd;
        pchMessageStart[1] = 0xbb;
        pchMessageStart[2] = 0xcc;
        pchMessageStart[3] = 0xad;
        vAlertPubKey = ParseHex("04e4f58b6a870d4ac13b35ca00f390c674561fea1c161b0c28b34c22ebb34afa5c8d874d12106c34a06c06d20a32d863079a4162003961a88bae4655ebd6a0440f");
        nDefaultPort = 52954;
        nRPCPort = 52953;
        strDataDir = "testnet3";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1397925814;
        genesis.nNonce = 385915966;
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x765d9ec322c494d36d36f300c209f13b787b6f67bf978c5b9ab09bc9e10f52b8"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("guncoin.info", "testnet-seed1.guncoin.info"));
        vSeeds.push_back(CDNSSeedData("guncoin.info", "testnet-seed2.guncoin.info"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(111);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(196);
        base58Prefixes[SECRET_KEY]     = list_of(239);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94);
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 1;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        strDataDir = "regtest";
        assert(hashGenesisBlock == uint256("0x2d9964969932dde0b499c5c7c073c7c7df3435e31b00135f5c9088ac5d2e1035"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
