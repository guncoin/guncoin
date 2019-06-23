// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode-sync.h>

#include <activemasternode.h>
#include <checkpoints.h>
#include <validation.h>
#include <masternode.h>
#include <masternode-payments.h>
#include <masternodeman.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <ui_interface.h>
#include <util.h>

class CMasternodeSync;
CMasternodeSync masternodeSync;

void CMasternodeSync::Fail()
{
    nTimeLastFailure = GetTime();
    nRequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
}

void CMasternodeSync::Reset()
{
    nRequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    nRequestedMasternodeAttempt = 0;
    nTimeAssetSyncStarted = GetTime();
    nTimeLastBumped = GetTime();
    nTimeLastFailure = 0;
}

void CMasternodeSync::BumpAssetLastTime(const std::string& strFuncName)
{
    if(IsSynced() || IsFailed()) return;
    nTimeLastBumped = GetTime();
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::BumpAssetLastTime -- %s\n", strFuncName);
}

std::string CMasternodeSync::GetAssetName()
{
    switch(nRequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_INITIAL):      return "MASTERNODE_SYNC_INITIAL";
        case(MASTERNODE_SYNC_WAITING):      return "MASTERNODE_SYNC_WAITING";
        case(MASTERNODE_SYNC_LIST):         return "MASTERNODE_SYNC_LIST";
        case(MASTERNODE_SYNC_MNW):          return "MASTERNODE_SYNC_MNW";
        case(MASTERNODE_SYNC_FAILED):       return "MASTERNODE_SYNC_FAILED";
        case MASTERNODE_SYNC_FINISHED:      return "MASTERNODE_SYNC_FINISHED";
        default:                            return "UNKNOWN";
    }
}

void CMasternodeSync::SwitchToNextAsset(CConnman& connman)
{
    switch(nRequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_FAILED):
            throw std::runtime_error("Can't switch to next asset from failed, should use Reset() first!");
            break;
        case(MASTERNODE_SYNC_INITIAL):
            nRequestedMasternodeAssets = MASTERNODE_SYNC_WAITING;
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_WAITING):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nRequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_LIST):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nRequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_MNW):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nRequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);

            //try to activate our masternode if possible
            activeMasternode.ManageState(connman);

            connman.ForEachNode(CConnman::AllNodes, [](CNode* pnode) {
                netfulfilledman.AddFulfilledRequest(pnode->addr, "full-sync");
            });
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Sync has finished\n");

            break;
    }
    nRequestedMasternodeAttempt = 0;
    nTimeAssetSyncStarted = GetTime();
    BumpAssetLastTime("CMasternodeSync::SwitchToNextAsset");
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.nRequestedMasternodeAssets) {
        case MASTERNODE_SYNC_INITIAL:       return _("Synchroning blockchain...");
        case MASTERNODE_SYNC_WAITING:       return _("Synchronization pending...");
        case MASTERNODE_SYNC_LIST:          return _("Synchronizing masternodes...");
        case MASTERNODE_SYNC_MNW:           return _("Synchronizing masternode payments...");
        case MASTERNODE_SYNC_FAILED:        return _("Synchronization failed");
        case MASTERNODE_SYNC_FINISHED:      return _("Synchronization finished");
        default:                            return "";
    }
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::SYNCSTATUSCOUNT) { //Sync status count

        //do not care about stats if sync process finished or failed
        if(IsSynced() || IsFailed()) return;

        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        LogPrintf("SYNCSTATUSCOUNT -- got inventory count: nItemID=%d  nCount=%d  peer=%d\n", nItemID, nCount, pfrom->GetId());
    }
}

void CMasternodeSync::ProcessTick(CConnman& connman)
{
    static int nTick = 0;
    if(nTick++ % MASTERNODE_SYNC_TICK_SECONDS != 0) return;

    // reset the sync process if the last call to this function was more than 60 minutes ago (client was in sleep mode)
    static int64_t nTimeLastProcess = GetTime();
    if(GetTime() - nTimeLastProcess > 60*60) {
        LogPrintf("CMasternodeSync::%s -- WARNING: no actions for too long, restarting sync...\n", __func__);
        Reset();
        SwitchToNextAsset(connman);
        nTimeLastProcess = GetTime();
        return;
    }
    nTimeLastProcess = GetTime();

    // reset sync status in case of any other sync failure
    if(IsFailed()) {
        if(nTimeLastFailure + (1*60) < GetTime()) { // 1 minute cooldown after failed sync
            LogPrintf("CMasternodeSync::%s -- WARNING: failed to sync, trying again...\n", __func__);
            Reset();
            SwitchToNextAsset(connman);
        }
        return;
    }

    if(IsSynced())
        return;

    // Calculate "progress" for LOG reporting / GUI notification
    double nSyncProgress = double(nRequestedMasternodeAttempt + (nRequestedMasternodeAssets - 1) * 8) / (8*4);
    LogPrintf("CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d nRequestedMasternodeAttempt %d nSyncProgress %f\n", __func__, nTick, nRequestedMasternodeAssets, nRequestedMasternodeAttempt, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    std::vector<CNode*> vNodesCopy = connman.CopyNodeVector(CConnman::FullyConnectedOnly);

    for (auto& pnode : vNodesCopy)
    {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());

        // Don't try to sync any data from outbound masternode connections -
        // they are temporary and should be considered unreliable for a sync process.
        // Inbound connection this early is most likely a masternode connection
        // initiated from another node, so skip it too.
        if (pnode->fMasternode || (fMasternodeMode && pnode->fInbound))
            continue;

        // QUICK MODE (REGTEST ONLY!)
        if(Params().NetworkIDString() == CBaseChainParams::REGTEST)
        {
            if(nRequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode, connman);
            } else if(nRequestedMasternodeAttempt < 6) {
                //sync payment votes
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MASTERNODEPAYMENTSYNC));
            } else {
                nRequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            nRequestedMasternodeAttempt++;
            connman.ReleaseNodeVector(vNodesCopy);
            return;
        }

        // NORMAL NETWORK MODE - TESTNET/MAINNET
        {
            // INITIAL TIMEOUT

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_WAITING) {
                if(GetTime() - nTimeLastBumped > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    // At this point we know that:
                    // a) there are peers (because we are looping on at least one of them);
                    // b) we waited for at least MASTERNODE_SYNC_TIMEOUT_SECONDS since we reached
                    //    the headers tip the last time (i.e. since we switched from
                    //     MASTERNODE_SYNC_INITIAL to MASTERNODE_SYNC_WAITING and bumped time);
                    // c) there were no blocks (UpdatedBlockTip, NotifyHeaderTip) or headers (AcceptedBlockHeader)
                    //    for at least MASTERNODE_SYNC_TIMEOUT_SECONDS.
                    // We must be at the tip already, let's move to the next asset.
                    SwitchToNextAsset(connman);
                }
            }

            // MNLIST : SYNC MASTERNODE LIST FROM OTHER CONNECTED CLIENTS

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                LogPrint(BCLog::MASTERNODE, "CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d nTimeLastBumped %lld GetTime() %lld diff %lld\n", __func__, nTick, nRequestedMasternodeAssets, nTimeLastBumped, GetTime(), GetTime() - nTimeLastBumped);
                // check for timeout first
                if(GetTime() - nTimeLastBumped > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrintf("CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d -- timeout\n", __func__, nTick, nRequestedMasternodeAssets);
                    if (nRequestedMasternodeAttempt == 0) {
                        LogPrintf("CMasternodeSync::%s -- ERROR: failed to sync %s\n", __func__, GetAssetName());
                        // there is no way we can continue without masternode list, fail here and try later
                        Fail();
                        connman.ReleaseNodeVector(vNodesCopy);
                        return;
                    }
                    SwitchToNextAsset(connman);
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // request from three peers max
                if (nRequestedMasternodeAttempt > 2) {
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // only request once from each peer
                if (netfulfilledman.HasFulfilledRequest(pnode->addr, "masternode-list-sync"))
                    continue;
                netfulfilledman.AddFulfilledRequest(pnode->addr, "masternode-list-sync");

                if (pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) {
                    LogPrintf("%s: FAILURE: nVersion: %d GetMinMasternodePaymentsProto: %d\n", __func__, pnode->nVersion, mnpayments.GetMinMasternodePaymentsProto());
                    continue;
                }
                nRequestedMasternodeAttempt++;

                mnodeman.DsegUpdate(pnode, connman);

                connman.ReleaseNodeVector(vNodesCopy);
                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // MNW : SYNC MASTERNODE PAYMENT VOTES FROM OTHER CONNECTED CLIENTS

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                LogPrint(BCLog::MNPAYMENTS, "CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d nTimeLastBumped %lld GetTime() %lld diff %lld\n", __func__, nTick, nRequestedMasternodeAssets, nTimeLastBumped, GetTime(), GetTime() - nTimeLastBumped);
                // check for timeout first
                // This might take a lot longer than MASTERNODE_SYNC_TIMEOUT_SECONDS due to new blocks,
                // but that should be OK and it should timeout eventually.
                if(GetTime() - nTimeLastBumped > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrintf("CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d -- timeout\n", __func__, nTick, nRequestedMasternodeAssets);
                    if (nRequestedMasternodeAttempt == 0) {
                        LogPrintf("CMasternodeSync::%s -- ERROR: failed to sync %s\n", __func__, GetAssetName());
                        // probably not a good idea to proceed without winner list
                        Fail();
                        connman.ReleaseNodeVector(vNodesCopy);
                        return;
                    }
                    SwitchToNextAsset(connman);
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // check for data
                // if mnpayments already has enough blocks and votes, switch to the next asset
                // try to fetch data from at least two peers though
                if(nRequestedMasternodeAttempt > 1 && mnpayments.IsEnoughData()) {
                    LogPrintf("CMasternodeSync::%s -- nTick %d nRequestedMasternodeAssets %d -- found enough data\n", __func__, nTick, nRequestedMasternodeAssets);
                    SwitchToNextAsset(connman);
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // request from three peers max
                if (nRequestedMasternodeAttempt > 2) {
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // only request once from each peer
                if(netfulfilledman.HasFulfilledRequest(pnode->addr, "masternode-payment-sync")) continue;
                netfulfilledman.AddFulfilledRequest(pnode->addr, "masternode-payment-sync");

                if(pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) continue;
                nRequestedMasternodeAttempt++;

                // sync payment votes, ask node for all payment votes it has
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MASTERNODEPAYMENTSYNC));
                // ask node for missing pieces only (old nodes will not be asked)
                mnpayments.RequestLowDataPaymentBlocks(pnode, connman);

                connman.ReleaseNodeVector(vNodesCopy);
                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }
        }
    }
    // looped through all nodes, release them
    connman.ReleaseNodeVector(vNodesCopy);
}

void CMasternodeSync::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::AcceptedBlockHeader -- pindexNew->nHeight: %d\n", pindexNew->nHeight);

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block header arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::AcceptedBlockHeader");
    }
}

void CMasternodeSync::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::NotifyHeaderTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);

    if (IsFailed() || IsSynced() || !pindexBestHeader)
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::NotifyHeaderTip");
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);

    if (IsFailed() || IsSynced() || !pindexBestHeader)
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::UpdatedBlockTip");
    }

    if (fInitialDownload) {
        // switched too early
        if (IsBlockchainSynced()) {
            Reset();
        }

        // no need to check any further while still in IBD mode
        return;
    }

    // Note: since we sync headers first, it should be ok to use this
    static bool fReachedBestHeader = false;
    bool fReachedBestHeaderNew = pindexNew->GetBlockHash() == pindexBestHeader->GetBlockHash();

    if (fReachedBestHeader && !fReachedBestHeaderNew) {
        // Switching from true to false means that we previousely stuck syncing headers for some reason,
        // probably initial timeout was not enough,
        // because there is no way we can update tip not having best header
        Reset();
        fReachedBestHeader = false;
        return;
    }

    fReachedBestHeader = fReachedBestHeaderNew;

    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d pindexBestHeader->nHeight: %d fInitialDownload=%d fReachedBestHeader=%d\n",
                pindexNew->nHeight, pindexBestHeader->nHeight, fInitialDownload, fReachedBestHeader);

    if (!IsBlockchainSynced() && fReachedBestHeader) {
        if (fLiteMode) {
            // nothing to do in lite mode, just finish the process immediately
            nRequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            return;
        }
        // Reached best header while being in initial mode.
        // We must be at the tip already, let's move to the next asset.
        SwitchToNextAsset(connman);
    }
}
