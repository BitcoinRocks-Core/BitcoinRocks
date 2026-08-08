// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/amount.h>
#include <kernel/chainparams.h>
#include <logging.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <txgraph.h>
#include <util/moneystr.h>
#include <util/translation.h>

#include <chrono>
#include <memory>
#include <string>

using common::AmountErrMsg;
using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

//! Maximum mempool size on 32-bit systems.
static constexpr int MAX_32BIT_MEMPOOL_MB{500};

namespace {

util::Result<void> ApplyPolicyProfile(
    const ArgsManager& argsman,
    MemPoolOptions& mempool_opts)
{
    const std::string profile{
        argsman.GetArg("-policyprofile").value_or("core")
    };

    mempool_opts.policy_profile = profile;

    if (profile == "core") {
        mempool_opts.max_datacarrier_bytes = MAX_OP_RETURN_RELAY;
        mempool_opts.permit_bare_multisig =
            DEFAULT_PERMIT_BAREMULTISIG;
        mempool_opts.max_tapscript_bytes = std::nullopt;
        mempool_opts.policy_log = false;
        mempool_opts.policy_log_details = false;
    } else if (profile == "conservative") {
        mempool_opts.max_datacarrier_bytes = 83;
        mempool_opts.permit_bare_multisig = false;
        mempool_opts.max_tapscript_bytes =
            MAX_STANDARD_P2WSH_SCRIPT_SIZE;
        mempool_opts.policy_log = true;
        mempool_opts.policy_log_details = false;
    } else if (profile == "strict") {
        mempool_opts.max_datacarrier_bytes = std::nullopt;
        mempool_opts.permit_bare_multisig = false;
        mempool_opts.max_tapscript_bytes = 0;
        mempool_opts.policy_log = true;
        mempool_opts.policy_log_details = false;
    } else {
        return util::Error{Untranslated(strprintf(
            "Unknown policyprofile '%s'. Valid profiles are: "
            "core, conservative, strict.",
            profile))};
    }

    // Individual arguments override the selected profile.
    if (argsman.IsArgSet("-permitbaremultisig")) {
        mempool_opts.permit_bare_multisig =
            argsman.GetBoolArg(
                "-permitbaremultisig",
                mempool_opts.permit_bare_multisig);
    }

    if (argsman.IsArgSet("-datacarrier")) {
        const bool enabled{
            argsman.GetBoolArg(
                "-datacarrier",
                mempool_opts.max_datacarrier_bytes.has_value())
        };

        if (!enabled) {
            mempool_opts.max_datacarrier_bytes = std::nullopt;
        } else if (!mempool_opts.max_datacarrier_bytes) {
            mempool_opts.max_datacarrier_bytes =
                MAX_OP_RETURN_RELAY;
        }
    }

    if (argsman.IsArgSet("-datacarriersize")) {
        const auto value{
            argsman.GetIntArg("-datacarriersize")
        };

        if (!value ||
            *value < 0 ||
            *value > static_cast<int64_t>(MAX_OP_RETURN_RELAY)) {
            return util::Error{Untranslated(strprintf(
                "-datacarriersize must be between 0 and %u",
                MAX_OP_RETURN_RELAY))};
        }

        if (mempool_opts.max_datacarrier_bytes) {
            mempool_opts.max_datacarrier_bytes =
                static_cast<unsigned>(*value);
        }
    }

    if (argsman.IsArgSet("-maxtapscriptsize")) {
        const auto value{
            argsman.GetIntArg("-maxtapscriptsize")
        };

        if (!value ||
            *value < 0 ||
            *value > static_cast<int64_t>(
                         MAX_STANDARD_TX_WEIGHT)) {
            return util::Error{Untranslated(strprintf(
                "-maxtapscriptsize must be between 0 and %d",
                MAX_STANDARD_TX_WEIGHT))};
        }

        mempool_opts.max_tapscript_bytes =
            static_cast<unsigned>(*value);
    }

    if (argsman.IsArgSet("-policylog")) {
        mempool_opts.policy_log =
            argsman.GetBoolArg(
                "-policylog",
                mempool_opts.policy_log);
    }

    if (argsman.IsArgSet("-policylogdetails")) {
        mempool_opts.policy_log_details =
            argsman.GetBoolArg(
                "-policylogdetails",
                mempool_opts.policy_log_details);

        if (mempool_opts.policy_log_details &&
            !argsman.IsArgSet("-policylog")) {
            mempool_opts.policy_log = true;
        }
    }

    return {};
}

void ApplyArgsManOptions(
    const ArgsManager& argsman,
    MemPoolLimits& mempool_limits)
{
    mempool_limits.cluster_count = argsman.GetIntArg("-limitclustercount", mempool_limits.cluster_count);

    if (auto vkb = argsman.GetIntArg("-limitclustersize")) mempool_limits.cluster_size_vbytes = *vkb * 1'000;

    mempool_limits.ancestor_count = argsman.GetIntArg("-limitancestorcount", mempool_limits.ancestor_count);

    mempool_limits.descendant_count = argsman.GetIntArg("-limitdescendantcount", mempool_limits.descendant_count);
}
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = argsman.GetIntArg("-checkmempool", mempool_opts.check_ratio);

    if (auto mb = argsman.GetIntArg("-maxmempool")) {
        constexpr bool is_32bit{sizeof(void*) == 4};
        if (is_32bit && *mb > MAX_32BIT_MEMPOOL_MB) {
            return util::Error{Untranslated(strprintf("-maxmempool is set to %i but can't be over %i MB on 32-bit systems", *mb, MAX_32BIT_MEMPOOL_MB))};
        }
        mempool_opts.max_size_bytes = *mb * 1'000'000;
    }

    if (auto hours = argsman.GetIntArg("-mempoolexpiry")) mempool_opts.expiry = std::chrono::hours{*hours};

    if (auto result{
            ApplyPolicyProfile(argsman, mempool_opts)};
        !result) {
        return result;
    }

    // incremental relay fee sets the minimum feerate increase necessary for replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (const auto arg{argsman.GetArg("-incrementalrelayfee")}) {
        if (std::optional<CAmount> inc_relay_fee = ParseMoney(*arg)) {
            mempool_opts.incremental_relay_feerate = CFeeRate{inc_relay_fee.value()};
        } else {
            return util::Error{AmountErrMsg("incrementalrelayfee", *arg)};
        }
    }

    static_assert(DEFAULT_MIN_RELAY_TX_FEE == DEFAULT_INCREMENTAL_RELAY_FEE);
    if (const auto arg{argsman.GetArg("-minrelaytxfee")}) {
        if (std::optional<CAmount> min_relay_feerate = ParseMoney(*arg)) {
            // High fee check is done afterward in CWallet::Create()
            mempool_opts.min_relay_feerate = CFeeRate{min_relay_feerate.value()};
        } else {
            return util::Error{AmountErrMsg("minrelaytxfee", *arg)};
        }
    } else if (mempool_opts.incremental_relay_feerate > mempool_opts.min_relay_feerate) {
        // Allow only setting incremental fee to control both
        mempool_opts.min_relay_feerate = mempool_opts.incremental_relay_feerate;
        LogInfo("Increasing minrelaytxfee to %s to match incrementalrelayfee", mempool_opts.min_relay_feerate.ToString());
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (const auto arg{argsman.GetArg("-dustrelayfee")}) {
        if (std::optional<CAmount> parsed = ParseMoney(*arg)) {
            mempool_opts.dust_relay_feerate = CFeeRate{parsed.value()};
        } else {
            return util::Error{AmountErrMsg("dustrelayfee", *arg)};
        }
    }

    mempool_opts.require_standard = !argsman.GetBoolArg("-acceptnonstdtxn", DEFAULT_ACCEPT_NON_STD_TXN);
    if (!chainparams.IsTestChain() && !mempool_opts.require_standard) {
        return util::Error{Untranslated(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.GetChainTypeString()))};
    }

    mempool_opts.persist_v1_dat = argsman.GetBoolArg("-persistmempoolv1", mempool_opts.persist_v1_dat);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    if (mempool_opts.limits.cluster_count > MAX_CLUSTER_COUNT_LIMIT) {
        return util::Error{Untranslated(strprintf("limitclustercount must be less than or equal to %d", MAX_CLUSTER_COUNT_LIMIT))};
    }

    LogInfo(
        "Transaction policy profile: %s",
        mempool_opts.policy_profile);
    LogInfo(
        "* OP_RETURN relay: %s",
        mempool_opts.max_datacarrier_bytes
            ? "enabled"
            : "disabled");
    LogInfo(
        "* Maximum aggregate OP_RETURN script size: %u bytes",
        mempool_opts.max_datacarrier_bytes.value_or(0));
    LogInfo(
        "* Bare multisig relay: %s",
        mempool_opts.permit_bare_multisig
            ? "enabled"
            : "disabled");

    if (mempool_opts.max_tapscript_bytes) {
        LogInfo(
            "* Maximum tapscript size: %u bytes",
            *mempool_opts.max_tapscript_bytes);
    } else {
        LogInfo(
            "* Maximum tapscript size: unlimited "
            "(Bitcoin Core behavior)");
    }

    LogInfo(
        "* Policy rejection logging: %s%s",
        mempool_opts.policy_log
            ? "enabled"
            : "disabled",
        mempool_opts.policy_log_details
            ? " with details"
            : "");

    return {};
}
