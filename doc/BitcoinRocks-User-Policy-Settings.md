BitcoinRocks User Policy Configuration

BitcoinRocks 31.1.1 adds configurable transaction relay-policy profiles for users who want either stock Bitcoin Core behavior or more restrictive handling of OP_RETURN data, bare multisig, Taproot script-path data, Ordinals, and inscriptions.

These settings are local node policy only. They affect mempool acceptance, transaction relay, testmempoolaccept, and transactions offered to local block-template creation. They do not change Bitcoin consensus rules. A transaction rejected by the local policy can still be mined by another miner, and BitcoinRocks will still validate and accept a block containing it when the transaction is consensus-valid.

Quick Start

Add one profile to bitcoinrocks.conf:

policyprofile=core

policyprofile=conservative

policyprofile=strict

Individual settings can override the selected profile:

policyprofile=conservative
datacarrier=1
datacarriersize=160
permitbaremultisig=1
maxtapscriptsize=10000
policylog=1
policylogdetails=1

Configuration precedence:

BitcoinRocks defaults
→ selected policy profile
→ explicit bitcoinrocks.conf settings
→ command-line settings

Policy Profiles

policyprofile=core

Preserves Bitcoin Core 31.1 relay-policy behavior.

OP_RETURN relay:             enabled
Maximum OP_RETURN bytes:     100000
Bare multisig relay:         enabled
Maximum tapscript size:      unlimited
Policy rejection logging:    disabled

policyprofile=conservative

Restricts common data-carrying transaction forms while retaining ordinary Taproot script-path use.

OP_RETURN relay:             enabled
Maximum OP_RETURN bytes:     83
Bare multisig relay:         disabled
Maximum tapscript size:      3600 bytes
Policy rejection logging:    enabled
Detailed rejection logging:  disabled

The 3,600-byte tapscript limit matches Bitcoin Core's standard P2WSH witnessScript-size limit.

policyprofile=strict

Applies the most restrictive built-in profile.

OP_RETURN relay:             disabled
Bare multisig relay:         disabled
Taproot script-path spends:  disabled
Taproot key-path spends:     allowed
Policy rejection logging:    enabled
Detailed rejection logging:  disabled

strict rejects every Taproot script-path spend from the local mempool, including legitimate contracts and future or unknown Taproot leaf versions. It does not affect Taproot key-path spends.

Configuration Options

policyprofile=<profile>

Selects the base relay-policy profile.

Valid values:

core
conservative
strict

Default:

core

Unknown values cause startup to fail with a configuration error.

datacarrier=<0|1>

Controls relay and mining-policy acceptance of OP_RETURN data-carrier outputs.

datacarrier=1

enables OP_RETURN relay.

datacarrier=0

disables OP_RETURN relay.

This existing Bitcoin Core option overrides the selected profile.

When re-enabling data carrier from strict, BitcoinRocks restores the Core allowance unless datacarriersize is also set:

policyprofile=strict
datacarrier=1
datacarriersize=83

datacarriersize=<bytes>

Sets the maximum aggregate raw script size allowed across all OP_RETURN outputs in one transaction.

Valid range:

0 through 100000

This option only takes effect while OP_RETURN relay is enabled. Under strict, set datacarrier=1 as well.

permitbaremultisig=<0|1>

Controls relay of transactions creating non-P2SH bare multisig outputs.

permitbaremultisig=1

allows bare multisig.

permitbaremultisig=0

rejects bare multisig from the local mempool and relay path.

This existing Bitcoin Core option overrides the selected profile.

maxtapscriptsize=<bytes>

Sets the maximum tapscript size accepted into the local mempool.

Valid range:

0 through 400000

Behavior:

Option unset under core:  no additional tapscript-size limit
0:                        reject all Taproot script-path spends
Positive value:           reject tapscripts larger than this many bytes

Examples:

maxtapscriptsize=3600

maxtapscriptsize=10000

maxtapscriptsize=0

This option does not restrict Taproot key-path spends.

policylog=<0|1>

Controls logging for transactions rejected specifically by the selected BitcoinRocks policy.

Defaults:

core:          disabled
conservative:  enabled
strict:        enabled

BitcoinRocks does not credit the profile for unrelated failures such as invalid consensus data, missing inputs, fee rejection, conflicts, or ordinary non-policy errors.

policylogdetails=<0|1>

Adds detailed rejection information to policy log entries.

Detailed fields can include:

input index
actual tapscript or data-carrier size
configured limit
inscription-like classification

When policylogdetails=1 is set without an explicit policylog option, policy logging is automatically enabled.

An explicit policylog=0 still disables logging.

Rejection Logging

Basic example:

User-selected policy profile "conservative" refused to relay transaction txid=<txid> wtxid=<wtxid> category=Taproot-script-path reason=tapscript-size

Detailed example:

User-selected policy profile "conservative" refused to relay transaction txid=<txid> wtxid=<wtxid> category=Ordinals/inscriptions reason=tapscript-size input=1 actual_size=148721 limit=3600 classification=inscription-like-ordinals-envelope

Profile-specific reasons include:

datacarrier-size-or-disabled
bare-multisig-disabled
tapscript-disabled
tapscript-size

BitcoinRocks logs both txid and wtxid because inscription data is carried in witness data.

Ordinals and Inscription Classification

BitcoinRocks labels a rejected tapscript as inscription-like when it contains the common Ordinals envelope marker:

OP_FALSE OP_IF OP_PUSHBYTES_3 "ord"

This is a structural classification, not a claim that every matching script is definitively an Ordinal inscription.

Duplicate and Rate-Limit Protection

Policy logging includes abuse protection:

Each rejected wtxid is logged once
Maximum 100 policy-rejection lines per minute
Duplicate and rate-limited entries are counted
A suppression summary is emitted for the previous window
The duplicate set is cleared after 100000 entries

Example:

User-selected policy profile "strict" suppressed 2417 duplicate or rate-limited policy rejection log entries during the previous 60 seconds

Startup Log

At startup, BitcoinRocks reports the effective resolved policy:

Transaction policy profile: conservative
* OP_RETURN relay: enabled
* Maximum aggregate OP_RETURN script size: 83 bytes
* Bare multisig relay: disabled
* Maximum tapscript size: 3600 bytes
* Policy rejection logging: enabled with details

These lines show the final values after the profile and all explicit overrides have been applied.

RPC Visibility

getmempoolinfo reports the active policy:

{
  "permitbaremultisig": false,
  "maxdatacarriersize": 83,
  "policyprofile": "conservative",
  "maxtapscriptsize": "3600",
  "policylog": true,
  "policylogdetails": true
}

For the Core profile with no tapscript limit:

{
  "maxtapscriptsize": "unlimited"
}

maxtapscriptsize is returned as a string so the RPC can represent both a numeric limit and unlimited.

Example Configurations

Stock Bitcoin Core Policy

policyprofile=core

Conservative Anti-Spam Policy

policyprofile=conservative
policylog=1
policylogdetails=1

Strict Local Relay Policy

policyprofile=strict
policylog=1
policylogdetails=1

Conservative Profile With Larger OP_RETURN Allowance

policyprofile=conservative
datacarrier=1
datacarriersize=160
policylog=1
policylogdetails=1

Strict Profile With OP_RETURN Re-enabled

policyprofile=strict
datacarrier=1
datacarriersize=83
policylog=1
policylogdetails=1

Taproot script-path spends remain disabled unless maxtapscriptsize is explicitly overridden.

Custom Taproot Limit

policyprofile=conservative
maxtapscriptsize=10000
policylog=1
policylogdetails=1

Important Limitations

These settings do not remove transaction data from confirmed blocks and do not prevent other nodes or miners from relaying or mining transactions rejected by the local node.

BitcoinRocks continues to:

download valid blocks
validate all consensus rules
accept consensus-valid blocks
store confirmed transaction and witness data
remain compatible with the Bitcoin network

The feature changes local transaction admission, relay, and mining policy only.
