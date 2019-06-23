// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MESSAGESIGNER_H
#define MESSAGESIGNER_H

#include <key.h>

/** Helper class for signing messages and checking their signatures
 */
class CMessageSigner
{
public:
    /// Set the private/public key values, returns true if successful
    static bool GetKeysFromSecret(const std::string& strSecret, CKey& keyRet, CPubKey& pubkeyRet);
    /// Sign the message, returns true if successful
    static bool SignMessage(const std::string& strMessage, std::vector<unsigned char>& vchSigRet, const CKey& key);
    /// Verify the message signature, returns true if succcessful
    static bool VerifyMessage(const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet);
    /// Verify the message signature, returns true if succcessful
    static bool VerifyMessage(const CKeyID& keyID, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet);
};

#endif
