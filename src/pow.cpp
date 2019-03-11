// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    int64_t nTargetTimespan = params.nPowTargetTimespan;

    int nHeight = pindexLast->nHeight + 1;

    if (nHeight < 30)
        return pindexLast->nBits;

    if (nHeight >= params.nDiffChange)
        return GetNextWorkRequired_eHRC(pindexLast, params);

    // Difficulty reset after the switch
    if (nHeight >= params.nNeoScryptHeight && nHeight < params.nNeoScryptHeight + 10)
        return UintToArith256(params.powNeoScryptLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* nTargetSpacing minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < params.DifficultyAdjustmentInterval(); i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    nActualTimespan = nTargetTimespan + (nActualTimespan - nTargetTimespan)/8;
    if (nActualTimespan < (nTargetTimespan - (nTargetTimespan/4)))
        nActualTimespan = (nTargetTimespan - (nTargetTimespan/4));
    if (nActualTimespan > (nTargetTimespan + (nTargetTimespan/2)))
        nActualTimespan = (nTargetTimespan + (nTargetTimespan/2));

    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    bool fShift = bnNew.bits() > bnPowLimit.bits() - 1;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;
    if (fShift)
        bnNew <<= 1;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

/**
 * eHRC difficulty adjust
 * Short, medium and long samples averaged together and compared against the target time span.
 * Adjust every block but limted to 9% change maximum.
*/
unsigned int GetNextWorkRequired_eHRC(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    int nHeight = pindexLast->nHeight + 1;
    int nTargetTimespan = 90;
    int shortSample = 15;
    int mediumSample = 180;
    int longSample = 720;
    int pindexFirstShortTime = 0;
    int pindexFirstMediumTime = 0;
    int nActualTimespan = 0;
    int nActualTimespanShort = 0;
    int nActualTimespanMedium = 0;
    int nActualTimespanLong = 0;

    // Genesis block or new chain
    if (pindexLast == NULL || nHeight <= longSample + 1)
        return UintToArith256(params.powLimit).GetCompact();

    const CBlockIndex* pindexFirstLong = pindexLast;
    for(int i = 0; pindexFirstLong && i < longSample; i++) {
        pindexFirstLong = pindexFirstLong->pprev;
        if (i == shortSample - 1)
            pindexFirstShortTime = pindexFirstLong->GetBlockTime();

        if (i == mediumSample - 1)
            pindexFirstMediumTime = pindexFirstLong->GetBlockTime();
    }

    if (pindexLast->GetBlockTime() - pindexFirstShortTime != 0)
        nActualTimespanShort = (pindexLast->GetBlockTime() - pindexFirstShortTime) / shortSample;

    if (pindexLast->GetBlockTime() - pindexFirstMediumTime != 0)
        nActualTimespanMedium = (pindexLast->GetBlockTime() - pindexFirstMediumTime)/ mediumSample;

    if (pindexLast->GetBlockTime() - pindexFirstLong->GetBlockTime() != 0)
        nActualTimespanLong = (pindexLast->GetBlockTime() - pindexFirstLong->GetBlockTime()) / longSample;

    int nActualTimespanSum = nActualTimespanShort + nActualTimespanMedium + nActualTimespanLong;

    if (nActualTimespanSum != 0)
        nActualTimespan = nActualTimespanSum / 3;

    // Apply .25 damping
    nActualTimespan = nActualTimespan + (3 * nTargetTimespan);
    nActualTimespan /= 4;

    // 9% difficulty limiter
    int nActualTimespanMax = nTargetTimespan * 494 / 453;
    int nActualTimespanMin = nTargetTimespan * 453 / 494;

    if(nActualTimespan < nActualTimespanMin)
        nActualTimespan = nActualTimespanMin;

    if(nActualTimespan > nActualTimespanMax)
        nActualTimespan = nActualTimespanMax;

    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > UintToArith256(params.powLimit))
        bnNew = UintToArith256(params.powLimit);

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
