// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>

#include <chainparamsseeds.h>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "1May launch of guncoin";
    const CScript genesisOutputScript = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 1000000;
        consensus.BIP34Height = 120000;
        consensus.BIP34Hash = uint256S("0x585ccf2029f8a59442db6872cb914b1c19838024cc40e6ed9fd0992dda71771f");
        consensus.BIP65Height = 845000; // fe1250d69e8541ceb38eff15cecc632353df02c8efdb12de612e498b160eeeb7
        consensus.BIP66Height = 845000; // fe1250d69e8541ceb38eff15cecc632353df02c8efdb12de612e498b160eeeb7
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powNeoScryptLimit = uint256S("0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nDiffChange = 850000;
        consensus.nNeoScryptHeight = 120000;
        consensus.nNeoScryptFork = 1414482565;
        consensus.nPowTargetTimespan = 2 * 60; // two weeks
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.checkpointPubKey = "0405f97712fd241d9af801b5839955c6a55ce742a34eb7074610a703c18202faa769e08ba30e055b925fc5b546393e1b7374e5c54f0f095104da8f96124acf4c40";
        consensus.vAlertPubKey = ParseHex("045a8927ef4e11ac24f4e65de016ae415fb3eb7ac0396b2dcf9f8451d96caf58908f8925811dea37f6c5b5789a2b4f78c7a1b5f56f019554061bbd01924612b4a0");
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 7560; // 75% of 10080
        consensus.nMinerConfirmationWindow = 10080; // Two weeks
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1517443200; // Feb 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1548979200; // Feb 1st, 2019

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1517443200; // Feb 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1548979200; // Feb 1st, 2019

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000014e4d48f2315c6");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x59dcac31362e356ca2ec583b15b7f952d6a442b934fab7b7b4e3c0eccca9310c"); //935461

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xaa;
        pchMessageStart[1] = 0xc3;
        pchMessageStart[2] = 0xc6;
        pchMessageStart[3] = 0xab;
        nDefaultPort = 42954;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1397923502, 2085244361, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x55a9b3b171a6dfc95d59e07fa23736f44cd745f896e86491982ad6d043c58ba6"));
        assert(genesis.hashMerkleRoot == uint256S("0x9279661f2031143951417ef97ea70bcd7172f1bff7831c339c97e0e8392a926a"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they dont support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seed.guncoin.info");
        vSeeds.emplace_back("seed2.guncoin.info");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,39);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,38);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,167);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "gc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                { 0, uint256S("0x55a9b3b171a6dfc95d59e07fa23736f44cd745f896e86491982ad6d043c58ba6")},
                { 800000, uint256S("0x35d92e51ad6cf189662c332fcdd61c1d90fee7c2914b42fa29d22c271159c066")},
            }
        };

        chainTxData = ChainTxData{
            // Data as of block 0000000000000000002d6cca6761c99b3c2e936f9a0e304b7c7651a993f461de (height 506081).
            1510084820, // * UNIX timestamp of last known number of transactions
            1141114,  // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.001         // * estimated number of transactions per second after that timestamp
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 1000000;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x765d9ec322c494d36d36f300c209f13b787b6f67bf978c5b9ab09bc9e10f52b8");
        consensus.BIP65Height = 751;
        consensus.BIP66Height = 751;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powNeoScryptLimit = uint256S("0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nNeoScryptHeight = 1;
        consensus.nNeoScryptFork = 1397925815;
        consensus.nDiffChange = 100;
        consensus.nPowTargetTimespan = 2 * 60; // two weeks
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.checkpointPubKey = "04b0c74b4334f0fd96f09070fbc28dc61a7dc1fbe8988ac98321f45fdd8ce8fed848f04ecaa398bfadb51b5f5adf706e9507f403ab5dce3c57bccf6c3a7db7e7a9";
        consensus.vAlertPubKey = ParseHex("04e4f58b6a870d4ac13b35ca00f390c674561fea1c161b0c28b34c22ebb34afa5c8d874d12106c34a06c06d20a32d863079a4162003961a88bae4655ebd6a0440f");
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 375; // 75% of 500
        consensus.nMinerConfirmationWindow = 500;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1517011200; // Jan 27th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1548547200; // Jan 27th, 2019

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1517011200; // Jan 27th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1548547200; // Jan 27th, 2019

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000001000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x765d9ec322c494d36d36f300c209f13b787b6f67bf978c5b9ab09bc9e10f52b8"); //0

        pchMessageStart[0] = 0xdd;
        pchMessageStart[1] = 0xbb;
        pchMessageStart[2] = 0xcc;
        pchMessageStart[3] = 0xad;
        nDefaultPort = 52954;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1397925814, 385915966, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x765d9ec322c494d36d36f300c209f13b787b6f67bf978c5b9ab09bc9e10f52b8"));
        assert(genesis.hashMerkleRoot == uint256S("0x9279661f2031143951417ef97ea70bcd7172f1bff7831c339c97e0e8392a926a"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.guncoin.info");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,127);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tg";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {
                {0, uint256S("765d9ec322c494d36d36f300c209f13b787b6f67bf978c5b9ab09bc9e10f52b8")},
            }
        };

        chainTxData = ChainTxData{
            1397925814,
            1,
            0.001
        };

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 751; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 751; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powNeoScryptLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nNeoScryptHeight = 1;
        consensus.nDiffChange = 1;
        consensus.nPowTargetTimespan = 2 * 60; // two weeks
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.checkpointPubKey = "04b0c74b4334f0fd96f09070fbc28dc61a7dc1fbe8988ac98321f45fdd8ce8fed848f04ecaa398bfadb51b5f5adf706e9507f403ab5dce3c57bccf6c3a7db7e7a9";
        consensus.vAlertPubKey = ParseHex("04e4f58b6a870d4ac13b35ca00f390c674561fea1c161b0c28b34c22ebb34afa5c8d874d12106c34a06c06d20a32d863079a4162003961a88bae4655ebd6a0440f");
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1296688602, 1, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x2d9964969932dde0b499c5c7c073c7c7df3435e31b00135f5c9088ac5d2e1035"));
        assert(genesis.hashMerkleRoot == uint256S("0x9279661f2031143951417ef97ea70bcd7172f1bff7831c339c97e0e8392a926a"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,127);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "gcrt";
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
