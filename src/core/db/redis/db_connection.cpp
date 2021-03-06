/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fastonosql/core/db/redis/db_connection.h>

#include <hiredis/hiredis.h>

#include <fastonosql/core/db/redis/server_info.h>
#include <fastonosql/core/imodule_connection_client.h>
#include <fastonosql/core/value.h>

#include "core/db/redis/internal/commands_api.h"

#if defined(PRO_VERSION)
#include "core/db/redis_base/internal/cluster_infos.h"
#include "core/db/redis_base/internal/sentinel_info.h"
#include "core/db/redis_compatible/internal/modules.h"
#endif

#define GET_SERVER_TYPE "CLUSTER NODES"
#define GET_SENTINEL_MASTERS "SENTINEL MASTERS"
#define GET_SENTINEL_SLAVES_PATTERN_1ARGS_S "SENTINEL SLAVES %s"

namespace fastonosql {
namespace core {
namespace redis {
namespace {
const ConstantCommandsArray kCommands = {
    CommandHolder(GEN_CMD_STRING(DB_HELP_COMMAND),
                  "[command]",
                  "Return how to use command",
                  UNDEFINED_SINCE,
                  DB_HELP_COMMAND " " DB_GET_KEY_COMMAND,
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Help),
    CommandHolder(GEN_CMD_STRING(DB_INFO_COMMAND),
                  "[section]",
                  "Get information and statistics about the server",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_INFO_COMMAND " " DB_GET_KEY_COMMAND,
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Info),
    CommandHolder(GEN_CMD_STRING("APPEND"),
                  "<key> <value>",
                  "Append a value to a key",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "APPEND test word",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Append),
    CommandHolder(GEN_CMD_STRING("AUTH"),
                  "<password>",
                  "Authenticate to the server",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "AUTH test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Auth),
    CommandHolder(GEN_CMD_STRING("BGREWRITEAOF"),
                  "-",
                  "Asynchronously rewrite the append-only file",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "BGREWRITEAOF",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::BgRewriteAof),
    CommandHolder(GEN_CMD_STRING("BGSAVE"),
                  "-",
                  "Asynchronously save the dataset to disk",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "BGSAVE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::BgSave),
    CommandHolder(GEN_CMD_STRING("BITCOUNT"),
                  "<key> [start] [end]",
                  "Count set bits in a string",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "BITCOUNT test",
                  1,
                  2,
                  CommandInfo::Native,
                  &CommandsApi::BitCount),
    CommandHolder(
        GEN_CMD_STRING("BITFIELD"),
        "<key> [GET type offset] [SET type offset value] [INCRBY type offset increment] [OVERFLOW WRAP|SAT|FAIL]",
        "Perform arbitrary bitfield integer operations on strings",
        PROJECT_VERSION_GENERATE(3, 2, 0),
        "BITFIELD mykey incrby u2 100 1 OVERFLOW SAT incrby u2 102 1",
        1,
        8,
        CommandInfo::Native,
        &CommandsApi::BitField),
    CommandHolder(GEN_CMD_STRING("BITOP"),
                  "<operation> <destkey> <key> [key ...]",
                  "Perform bitwise operations between strings",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "BITOP AND dest key1 key2",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::BitOp),
    CommandHolder(GEN_CMD_STRING("BITPOS"),
                  "<key> <bit> [start] [end]",
                  "Find first bit set or clear in a string",
                  PROJECT_VERSION_GENERATE(2, 8, 7),
                  "BITPOS mykey 0",
                  2,
                  2,
                  CommandInfo::Native,
                  &CommandsApi::BitPos),
    CommandHolder(GEN_CMD_STRING("BLPOP"),
                  "<key> [key ...] timeout",
                  "Remove and get the first element in a list, or block until one is available",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "BLPOP list1 list2 0",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::BlPop),
    CommandHolder(GEN_CMD_STRING("BRPOP"),
                  "<key> [key ...] timeout",
                  "Remove and get the last element in a list, or block until one is available",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "BRPOP list1 list2 0",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::BrPop),
    CommandHolder(GEN_CMD_STRING("BRPOPLPUSH"),
                  "<source> <destination> <timeout>",
                  "Pop a value from a list, push it to another list and return it; or block until one is available",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "BRPOPLPUSH list otherlist 10",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::BrPopLpush),

    CommandHolder(GEN_CMD_STRING("CLIENT GETNAME"),
                  "-",
                  "Get the current connection name",
                  PROJECT_VERSION_GENERATE(2, 6, 9),
                  "CLIENT GETNAME",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClientGetName),
    CommandHolder(GEN_CMD_STRING("CLIENT KILL"),
                  "<filter> <value> ...",
                  "Kill the connection of a client",
                  PROJECT_VERSION_GENERATE(2, 8, 12),
                  "CLIENT KILL ADDR 127.0.0.1:4444",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ClientKill),
    CommandHolder(GEN_CMD_STRING("CLIENT LIST"),
                  "-",
                  "Get the list of client connections",
                  PROJECT_VERSION_GENERATE(2, 4, 0),
                  "CLIENT LIST",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClientList),
    CommandHolder(GEN_CMD_STRING("CLIENT PAUSE"),
                  "<timeout>",
                  "Stop processing commands from clients for some time",
                  PROJECT_VERSION_GENERATE(2, 9, 50),
                  "CLIENT PAUSE",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClientPause),
    CommandHolder(GEN_CMD_STRING("CLIENT REPLY"),
                  "<ON|OFF|SKIP>",
                  "Instruct the server whether to reply to commands",
                  PROJECT_VERSION_GENERATE(3, 2, 0),
                  "CLIENT REPLY OFF",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClientReply),
    CommandHolder(GEN_CMD_STRING("CLIENT SETNAME"),
                  "<connection-name>",
                  "Set the current connection name",
                  PROJECT_VERSION_GENERATE(2, 6, 9),
                  "CLIENT SETNAME FastoRedis",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClientSetName),
#if defined(PRO_VERSION)
    CommandHolder(GEN_CMD_STRING("CLUSTER ADDSLOTS"),
                  "<slot> [slot ...]",
                  "Assign new hash slots to receiving node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER ADDSLOTS slot",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ClusterAddSlots),
    CommandHolder(GEN_CMD_STRING("CLUSTER COUNT-FAILURE-REPORTS"),
                  "<node-id>",
                  "Return the number of failure reports active for a given node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER COUNT-FAILURE-REPORTS id",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterCountFailureReports),
    CommandHolder(GEN_CMD_STRING("CLUSTER COUNTKEYSINSLOT"),
                  "<slot>",
                  "Return the number of local keys in the specified hash slot",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER COUNTKEYSINSLOT 123",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterCountKeysSinSlot),
    CommandHolder(GEN_CMD_STRING("CLUSTER DELSLOTS"),
                  "<slot> [slot ...]",
                  "Set hash slots as unbound in receiving node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER DELSLOTS 123",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ClusterDelSlots),
    CommandHolder(GEN_CMD_STRING("CLUSTER FAILOVER"),
                  "[FORCE|TAKEOVER]",
                  "Forces a slave to perform a manual failover osyncf its master.",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER FAILOVER",
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::ClusterFailover),
    CommandHolder(GEN_CMD_STRING("CLUSTER FORGET"),
                  "<node-id>",
                  "Remove a node from the nodes table",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER FORGET id",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterForget),
    CommandHolder(GEN_CMD_STRING("CLUSTER GETKEYSINSLOT"),
                  "<slot> <count>",
                  "Return local key names in the specified hash slot",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER GETKEYSINSLOT 1 2",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterGetKeySinSlot),
    CommandHolder(GEN_CMD_STRING("CLUSTER INFO"),
                  "-",
                  "Provides info about Redis Cluster node state",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER INFO",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterInfo),
    CommandHolder(GEN_CMD_STRING("CLUSTER KEYSLOT"),
                  "<key>",
                  "Returns the hash slot of the specified key",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER KEYSLOT test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterKeySlot),
    CommandHolder(GEN_CMD_STRING("CLUSTER MEET"),
                  "<ip> <port>",
                  "Force a node cluster to handshake with another node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER MEET 127.0.0.1 5556",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterMeet),
    CommandHolder(GEN_CMD_STRING("CLUSTER NODES"),
                  "-",
                  "Get Cluster config for the node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER NODES",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterNodes),
    CommandHolder(GEN_CMD_STRING("CLUSTER REPLICATE"),
                  "<node-id>",
                  "Reconfigure a node as a slave of the specified master node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER REPLICATE 123",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterReplicate),
    CommandHolder(GEN_CMD_STRING("CLUSTER RESET"),
                  "[HARD|SOFT]",
                  "Reset a Redis Cluster node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER RESET HARD",
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::ClusterReset),
    CommandHolder(GEN_CMD_STRING("CLUSTER SAVECONFIG"),
                  "-",
                  "Forces the node to save cluster state on disk",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER SAVECONFIG",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterSaveConfig),
    CommandHolder(GEN_CMD_STRING("CLUSTER SET-CONFIG-EPOCH"),
                  "<config-epoch>",
                  "Set the configuration epoch in a new node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER SET-CONFIG-EPOCH 1234",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterSetConfigEpoch),
    CommandHolder(GEN_CMD_STRING("CLUSTER SETSLOT"),
                  "<slot> IMPORTING|MIGRATING|STABLE|NODE [node-id]",
                  "Bind a hash slot to a specific node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER SETSLOT 123 IMPORTING 311",
                  2,
                  2,
                  CommandInfo::Native,
                  &CommandsApi::ClusterSetSlot),
    CommandHolder(GEN_CMD_STRING("CLUSTER SLAVES"),
                  "<node-id>",
                  "Licommon_execst slave nodes of the specified master node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER SLAVES 123",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterSlaves),
    CommandHolder(GEN_CMD_STRING("CLUSTER SLOTS"),
                  "-",
                  "Get array of Cluster slot to node mappings",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "CLUSTER SLOTS",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ClusterSlots),
#endif
    CommandHolder(GEN_CMD_STRING("COMMAND COUNT"),
                  "-",
                  "Get total number of Redis commands",
                  PROJECT_VERSION_GENERATE(2, 8, 13),
                  "COMMAND COUNT",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::CommandCount),
    CommandHolder(GEN_CMD_STRING("COMMAND GETKEYS"),
                  "<command> [args]",
                  "Extract keys given a full Redis command",
                  PROJECT_VERSION_GENERATE(2, 8, 13),
                  "COMMAND GETKEYS MSET a b c d e f",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::CommandGetKeys),
    CommandHolder(GEN_CMD_STRING("COMMAND INFO"),
                  "<command-name> [command-name ...]",
                  "Get array of specific Redis command details",
                  PROJECT_VERSION_GENERATE(2, 8, 13),
                  "COMMAND INFO GET",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::CommandInfo),
    CommandHolder(GEN_CMD_STRING("COMMAND"),
                  "-",
                  "Get array of Redis command details",
                  PROJECT_VERSION_GENERATE(2, 8, 13),
                  "COMMAND",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Command),

    CommandHolder(GEN_CMD_STRING(DB_GET_CONFIG_COMMAND),
                  "<parameter>",
                  "Get the value of a configuration parameter",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  DB_GET_CONFIG_COMMAND " databases",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ConfigGet),
    CommandHolder(GEN_CMD_STRING("CONFIG RESETSTAT"),
                  "-",
                  "Reset the stats returned by INFO",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "CONFIG RESETSTAT",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ConfigResetStat),
    CommandHolder(GEN_CMD_STRING("CONFIG REWRITE"),
                  "-",
                  "Rewrite the configuration file with the "
                  "in memory configuration",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  "CONFIG REWRITE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ConfigRewrite),
    CommandHolder(GEN_CMD_STRING("CONFIG SET"),
                  "<parameter> <value>",
                  "Set a configuration parameter to the given value",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "CONFIG SET par val",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ConfigSet),

    CommandHolder(GEN_CMD_STRING(DB_DBKCOUNT_COMMAND),
                  "-",
                  "Return the number of keys in the selected database",
                  UNDEFINED_SINCE,
                  DB_DBKCOUNT_COMMAND,
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::DBKeysCount),
    CommandHolder(GEN_CMD_STRING("DBSIZE"),
                  "-",
                  "Return the number of keys in the selected database",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "DBSIZE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::DBSize),

    CommandHolder(GEN_CMD_STRING("DEBUG OBJECT"),
                  "<key>",
                  "Get debugging information about a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "DEBUG OBJECT test",
                  1,
                  0,
                  CommandInfo::Internal,
                  &CommandsApi::DebugObject),
    CommandHolder(GEN_CMD_STRING("DEBUG SEGFAULT"),
                  "-",
                  "Make the server crash",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "DEBUG SEGFAULT",
                  0,
                  0,
                  CommandInfo::Internal,
                  &CommandsApi::DebugSegFault),

    CommandHolder(GEN_CMD_STRING("DECR"),
                  "<key>",
                  "Decrement the integer value of a key by one",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "DECR test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Decr),
    CommandHolder(GEN_CMD_STRING("DECRBY"),
                  "<key> <decrement>",
                  "Decrement the integer value of a key by the given number",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "DECRBY test 11",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::DecrBy),
    CommandHolder(GEN_CMD_STRING(DB_DELETE_KEY_COMMAND),
                  "<key> [key ...]",
                  "Delete a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_DELETE_KEY_COMMAND " key",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Delete),
    CommandHolder(GEN_CMD_STRING("DISCARD"),
                  "-",
                  "Discard all commands issued after MULTI",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "DISCARD",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Discard),
    CommandHolder(GEN_CMD_STRING("DUMP"),
                  "<key>",
                  "Return a serialized version of the value stored at the specified key.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "DUMP test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Dump),
    CommandHolder(GEN_CMD_STRING("ECHO"),
                  "<message>",
                  "Echo the given string",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "ECHO msg",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Echo),
    CommandHolder(GEN_CMD_STRING("EVAL"),
                  "<script> <numkeys> <key> [key ...] <arg> [arg ...]",
                  "Execute a Lua script server side",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "eval \"return {KEYS[1],KEYS[2],ARGV[1],ARGV[2]}\" 2 key1 key2 first second",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Eval),
    CommandHolder(GEN_CMD_STRING("EVALSHA"),
                  "<sha1> <numkeys> <key> [key ...] <arg> [arg ...]",
                  "Execute a Lua script server side",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "EVALSHA 6b1bf486c81ceb7edf3c093f4c48582e38c0e791 0",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::EvalSha),
    CommandHolder(GEN_CMD_STRING("EXEC"),
                  "-",
                  "Execute all commands issued after MULTI",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "EXEC",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Exec),
    CommandHolder(GEN_CMD_STRING("EXISTS"),
                  "key [key ...]",
                  "Determine if a key exists",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "EXISTS test",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Exists),
    CommandHolder(GEN_CMD_STRING(DB_SET_TTL_COMMAND),
                  "<key> <seconds>",
                  "Set a key's time to live in seconds",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_SET_TTL_COMMAND " key 10",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SetTTL),
    CommandHolder(GEN_CMD_STRING("EXPIREAT"),
                  "<key> <timestamp>",
                  "Set the expiration for a key as a UNIX timestamp",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "EXPIREAT 12345",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ExpireAt),
    CommandHolder(GEN_CMD_STRING("FLUSHALL"),
                  "-",
                  "Remove all keys from all databases",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "FLUSHALL",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::FlushALL),
    CommandHolder(GEN_CMD_STRING(DB_FLUSHDB_COMMAND),
                  "[ASYNC]",
                  "Remove all keys from the current database",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_FLUSHDB_COMMAND,
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::FlushDB),
    CommandHolder(GEN_CMD_STRING("GEOADD"),
                  "<key> <longitude> <latitude> <member> [<longitude> <latitude> <member> ...]",
                  "Add one or more geospatial items in the geospatial index represented using a sorted set",
                  UNDEFINED_SINCE,
                  "GEOADD Sicily 13.361389 38.115556 \"Palermo\" 15.087269 37.502669 \"Catania\"",
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::GeoAdd),
    CommandHolder(GEN_CMD_STRING("GEODIST"),
                  "<key> <member1> <member2> [unit]",
                  "Returns the distance between two members of a geospatial index",
                  UNDEFINED_SINCE,
                  "GEODIST Sicily Palermo Catania",
                  3,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::GeoDist),
    CommandHolder(GEN_CMD_STRING("GEOHASH"),
                  "<key> <member> [member ...]",
                  "Returns members of a geospatial index as standard geohash strings",
                  UNDEFINED_SINCE,
                  "GEOHASH Sicily Palermo Catania",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::GeoHash),
    CommandHolder(GEN_CMD_STRING("GEOPOS"),
                  "<key> <member> [member ...]",
                  "Returns longitude and latitude of members of a geospatial index",
                  UNDEFINED_SINCE,
                  "GEOPOS Sicily Palermo Catania NonExisting",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::GeoPos),

    CommandHolder(
        GEN_CMD_STRING("GEORADIUS"),
        "<key> <longitude> <latitude> <radius> m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC]",
        "Query a sorted set representing a geospatial index to fetch members matching a given maximum "
        "distance from a point",
        UNDEFINED_SINCE,
        "GEORADIUS Sicily 15 37 200 km WITHDIST",
        5,
        6,
        CommandInfo::Native,
        &CommandsApi::GeoRadius),
    CommandHolder(GEN_CMD_STRING("GEORADIUSBYMEMBER"),
                  "<key> <member> <radius> m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC]",
                  "Query a sorted set representing a geospatial index to fetch members matching a given "
                  "maximum distance from a member",
                  UNDEFINED_SINCE,
                  "GEORADIUSBYMEMBER Sicily Agrigento 100 km",
                  4,
                  6,
                  CommandInfo::Native,
                  &CommandsApi::GeoRadiusByMember),
    CommandHolder(GEN_CMD_STRING(DB_GET_KEY_COMMAND),
                  "<key>",
                  "Gecommon_exect the value of a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_GET_KEY_COMMAND " key",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Get),
    CommandHolder(GEN_CMD_STRING(DB_GETUNI_KEY_COMMAND),
                  "<key>",
                  "Get the value of a key.",
                  UNDEFINED_SINCE,
                  DB_GETUNI_KEY_COMMAND " key",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetUni),
    CommandHolder(GEN_CMD_STRING("GETBIT"),
                  "<key> <offset>",
                  "Returns the bit value at offset in the string value stored at key",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "GETBIT mykey 0",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetBit),
    CommandHolder(GEN_CMD_STRING("GETRANGE"),
                  "<key> <start> <end>",
                  "Get a substring of the string stored at a key",
                  PROJECT_VERSION_GENERATE(2, 4, 0),
                  "GETRANGE mykey 0 3",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetRange),
    CommandHolder(GEN_CMD_STRING("GETSET"),
                  "<key> <value>",
                  "Set the string value of a key and return its old value",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "GETSET mycounter 0",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetSet),
    CommandHolder(GEN_CMD_STRING("HDEL"),
                  "<key> <field> [field ...]",
                  "Delete one or more hash fields",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HDEL myhash field1",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Hdel),
    CommandHolder(GEN_CMD_STRING("HEXISTS"),
                  "<key> <field>",
                  "Determine if a hash field exists",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HEXISTS myhash field1",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hexists),
    CommandHolder(GEN_CMD_STRING("HGET"),
                  "<key> <field>",
                  "Get the value of a hash field",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HGET myhash field1",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hget),
    CommandHolder(GEN_CMD_STRING("HGETALL"),
                  "<key>",
                  "Get all the fields and values in a hash",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HGETALL myhash",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hgetall),
    CommandHolder(GEN_CMD_STRING("HINCRBY"),
                  "<key> <field> <increment>",
                  "Increment the integer value of a hash field by the given number",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HINCRBY myhash field 1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::HincrByFloat),
    CommandHolder(GEN_CMD_STRING("HINCRBYFLOAT"),
                  "<key> <field> <increment>",
                  "Increment the float value of a hash field by the given amount",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "HINCRBYFLOAT mykey field 0.1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::HincrByFloat),
    CommandHolder(GEN_CMD_STRING("HKEYS"),
                  "<key>",
                  "Get all the fields in a hash",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HKEYS myhash",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hkeys),
    CommandHolder(GEN_CMD_STRING("HLEN"),
                  "<key>",
                  "Get the number of fields in a hash",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HLEN myhash",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hlen),
    CommandHolder(GEN_CMD_STRING("HMGET"),
                  "<key> <field> [field ...]",
                  "Get the values of all the given hash fields",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HMGET myhash field1 field2 nofield",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Hmget),
    CommandHolder(GEN_CMD_STRING("HMSET"),
                  "<key> <field> <value> [field value ...]",
                  "Set multiple hash fields to multiple values",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HMSET key kl value",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Hmset,

                  {&TestArgsInRange, &TestArgsModule2Equal1}),
    CommandHolder(GEN_CMD_STRING("HSCAN"),
                  "<key> <cursor> [MATCH pattern] [COUNT count]",
                  "Incrementally iterate hash fields and associated values",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  "HSCAN hash 0 MATCH * COUNT 10",
                  2,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::Hscan),
    CommandHolder(GEN_CMD_STRING("HSET"),
                  "<key> <field> <value>",
                  "Set the string value of a hash field",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HSET myhash field1 data",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hset),
    CommandHolder(GEN_CMD_STRING("HSETNX"),
                  "<key> <field> <value>",
                  "Set the value of a hash field, only if the field does not exist",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HSETNX myhash field1 data",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::HsetNX),
    CommandHolder(GEN_CMD_STRING("HSTRLEN"),
                  "<key> <field>",
                  "Get the length of the value of a hash field",
                  PROJECT_VERSION_GENERATE(3, 2, 0),
                  "HSTRLEN myhash f1",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hstrlen),
    CommandHolder(GEN_CMD_STRING("HVALS"),
                  "<key>",
                  "Get all the values in a hash",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HVALS myhash",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hvals),
    CommandHolder(GEN_CMD_STRING("INCR"),
                  "<key>",
                  "Increment the integer value of a key by one",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "INCR name",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Incr),
    CommandHolder(GEN_CMD_STRING("INCRBY"),
                  "<key> <increment>",
                  "Increment the integer value of a key by the given amount",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "INCRBY name 1",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::IncrBy),
    CommandHolder(GEN_CMD_STRING("INCRBYFLOAT"),
                  "<key> <increment>",
                  "Increment the float value of a key by the given amount",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "INCRBYFLOAT name 1",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::IncrByFloat),
    CommandHolder(GEN_CMD_STRING(DB_KEYS_COMMAND),
                  "<pattern>",
                  "Find all keys matching the given pattern",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_KEYS_COMMAND ALL_KEYS_PATTERNS,
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::RKeys),
    CommandHolder(GEN_CMD_STRING("LASTSAVE"),
                  "-",
                  "Get the UNIX time stamp of the last successful save to disk",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LASTSAVE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::LastSave),
    CommandHolder(GEN_CMD_STRING("LINDEX"),
                  "<key> <index>",
                  "Get an element from a list by its index",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LINDEX mylist 0",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Lindex),
    CommandHolder(GEN_CMD_STRING("LINSERT"),
                  "<key> <BEFORE|AFTER> <pivot> <value>",
                  "Insert an element before or after another element in a list",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "LINSERT mylist BEFORE World There",
                  4,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Linsert),
    CommandHolder(GEN_CMD_STRING("LLEN"),
                  "<key>",
                  "Get the length of a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LLEN mylist",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Llen),
    CommandHolder(GEN_CMD_STRING("LPOP"),
                  "<key>",
                  "Remove and get the first element in a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LPOP mylist",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Lpop),
    CommandHolder(GEN_CMD_STRING("LPUSH"),
                  "<key> <value> [value ...]",
                  "Prepend one or multiple values to a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LPUSH mylist hello",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Lpush),
    CommandHolder(GEN_CMD_STRING("LPUSHX"),
                  "<key> <value>",
                  "Prepend a value to a list, only if the list exists",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "LPUSHX myotherlist Hello",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::LpushX),
    CommandHolder(GEN_CMD_STRING("LRANGE"),
                  "<key> <start> <stop>",
                  "Get a range of elements from a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LRANGE mylist -3 2",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Lrange),
    CommandHolder(GEN_CMD_STRING("LREM"),
                  "<key> <count> <value>",
                  "Remove elements from a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LREM mylist -2 hello",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Lrem),
    CommandHolder(GEN_CMD_STRING("LSET"),
                  "<key> <index> <value>",
                  "Set the value of an element in a list by its index",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LSET mylist -2 five",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Lset),
    CommandHolder(GEN_CMD_STRING("LTRIM"),
                  "<key> <start> <stop>",
                  "Trim a list to the specified range",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LTRIM mylist 1 -1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Ltrim),
    CommandHolder(GEN_CMD_STRING("MGET"),
                  "<key> [key ...]",
                  "Get the values of all the given keys",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "MGET key1 key2 nonexisting",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Mget),
    CommandHolder(GEN_CMD_STRING("MIGRATE"),
                  "<host> <port> <key> <destination-db> <timeout> [COPY] [REPLACE] [KEYS key]",
                  "Atomically transfer a key from a Redis instance to another one.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "MIGRATE 192.168.1.34 6379 \"\" 0 5000 KEYS key1 key2 key3",
                  5,
                  2,
                  CommandInfo::Native,
                  &CommandsApi::Migrate),
    CommandHolder(GEN_CMD_STRING("MONITOR"),
                  "-",
                  "Listen for all requests received by the server in real time",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "MONITOR",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Monitor),
    CommandHolder(GEN_CMD_STRING("MOVE"),
                  "<key> <db>",
                  "Move a key to another database",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "MOVE name 7",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Move),
    CommandHolder(GEN_CMD_STRING("MSET"),
                  "<key> <value> [key value ...]",
                  "Set multiple keys to multiple values",
                  PROJECT_VERSION_GENERATE(1, 0, 1),
                  "MSET key1 Hello key2 World",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Mset),
    CommandHolder(GEN_CMD_STRING("MSETNX"),
                  "<key> <value> [key value ...]",
                  "Set multiple keys to multiple values, only if none of the keys exist",
                  PROJECT_VERSION_GENERATE(1, 0, 1),
                  "MSETNX key1 Hello key2 there",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::MsetNX),
    CommandHolder(GEN_CMD_STRING("MULTI"),
                  "-",
                  "Mark the start of a transaction block",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "MULTI",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Multi),
    CommandHolder(GEN_CMD_STRING("OBJECT"),
                  "<subcommand> [arguments [arguments ...]]",
                  "Inspect the internals of Redis objects",
                  PROJECT_VERSION_GENERATE(2, 2, 3),
                  "OBJECT refcount mylist",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Object),
    CommandHolder(GEN_CMD_STRING("PERSIST"),
                  "<key>",
                  "Remove the expiration from a key",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "PERSIST mykey",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Persist),
    CommandHolder(GEN_CMD_STRING(REDIS_CHANGE_PTTL_COMMAND),
                  "<key> <milliseconds>",
                  "Set a key's time to live in milliseconds",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  REDIS_CHANGE_PTTL_COMMAND " mykey 1500",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Pexpire),
    CommandHolder(GEN_CMD_STRING("PEXPIREAT"),
                  "<key> <milliseconds-timestamp>",
                  "Set the expiration for a key as a UNIX timestamp specified in milliseconds",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "PEXPIREAT mykey 1555555555005",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::PexpireAt),
    CommandHolder(GEN_CMD_STRING("PFADD"),
                  "<key> <element> [element ...]",
                  "Adds the specified elements to the specified HyperLogLog.",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "PFADD hll a b c d e f g",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Pfadd),
    CommandHolder(GEN_CMD_STRING("PFCOUNT"),
                  "<key> [key ...]",
                  "Return the approximated cardinality of the set(s) observed by the HyperLogLog at key(s).",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "PFCOUNT hll",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Pfcount),
    CommandHolder(GEN_CMD_STRING("PFMERGE"),
                  "<destkey> <sourcekey> [sourcekey ...]",
                  "Merge N different HyperLogLogs into a single one.",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "PFMERGE hll3 hll1 hll2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Pfmerge),
    CommandHolder(GEN_CMD_STRING("PING"),
                  "[message]",
                  "Ping the server",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "PING",
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Ping),
    CommandHolder(GEN_CMD_STRING("PSETEX"),
                  "<key> <milliseconds> <value>",
                  "Set the value and expiration in "
                  "milliseconds of a key",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "PSETEX mykey 1000 hi",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::PsetEx),
    CommandHolder(GEN_CMD_STRING("PSUBSCRIBE"),
                  "<pattern> [pattern ...]",
                  "Listen for messages published to channels matching the given patterns",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "PSUBSCRIBE h*ll",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Subscribe),
    CommandHolder(GEN_CMD_STRING(REDIS_GET_PTTL_COMMAND),
                  "<key>",
                  "Get the time to live for a key in milliseconds",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "PTTL mykey",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Pttl),
    CommandHolder(GEN_CMD_STRING(DB_PUBLISH_COMMAND),
                  "<channel> <message>",
                  "Post a message to a channel",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "PUBLISH chan pan",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Publish),
    CommandHolder(GEN_CMD_STRING("PUBSUB"),
                  "<subcommand> [argument [argument ...]]",
                  "Inspect the state of the Pub/Sub subsystem",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  "PUBSUB NUMSUB",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::PubSub),
    CommandHolder(GEN_CMD_STRING("PUNSUBSCRIBE"),
                  "[pattern [pattern ...]]",
                  "Stop listening for messages posted to channels matching the given patterns",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "PUNSUBSCRIBE *",
                  0,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::PunSubscribe),
    CommandHolder(GEN_CMD_STRING(DB_QUIT_COMMAND),
                  "-",
                  "Close the connection",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_QUIT_COMMAND,
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Quit),
    CommandHolder(GEN_CMD_STRING("RANDOMKEY"),
                  "-",
                  "Return a random key from the keyspace",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "RANDOMKEY",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::RandomKey),
    CommandHolder(GEN_CMD_STRING("READONLY"),
                  "-",
                  "Enables read queries for a connection to a cluster slave node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "READONLY",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ReadOnly),
    CommandHolder(GEN_CMD_STRING("READWRITE"),
                  "-",
                  "Disables read queries for a connection to a cluster slave node",
                  PROJECT_VERSION_GENERATE(3, 0, 0),
                  "READWRITE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ReadWrite),
    CommandHolder(GEN_CMD_STRING(DB_RENAME_KEY_COMMAND),
                  "<key> <newkey>",
                  "Rename a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_RENAME_KEY_COMMAND " old_name new_name",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Rename),
    CommandHolder(GEN_CMD_STRING("RENAMENX"),
                  "<key> <newkey>",
                  "Rename a key, only if the new key does not exist",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "RENAMENX mykey myotherkey",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::RenameNx),
    CommandHolder(GEN_CMD_STRING("RESTORE"),
                  "<key> <ttl> <serialized-value> [REPLACE]",
                  "Create a key using the provided serialized value, previously obtained using DUMP.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "RESTORE mykey 0 alex",
                  3,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Restore),
    CommandHolder(GEN_CMD_STRING("ROLE"),
                  "-",
                  "Return the role of the instance in the context of replication",
                  PROJECT_VERSION_GENERATE(2, 8, 12),
                  "ROLE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Role),
    CommandHolder(GEN_CMD_STRING("RPOP"),
                  "<key>",
                  "Remove and get the last element in a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "RPOP mykey",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Rpop),
    CommandHolder(GEN_CMD_STRING("RPOPLPUSH"),
                  "<source> <destination>",
                  "Remove the last element in a list, prepend it to another list and return it",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "RPOPLPUSH mylist myotherlist",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::RpopLpush),
    CommandHolder(GEN_CMD_STRING("RPUSH"),
                  "<key> <value> [value ...]",
                  "Append one or multiple values to a list",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "RPUSH mylist hello",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Rpush),
    CommandHolder(GEN_CMD_STRING("RPUSHX"),
                  "<key> <value>",
                  "Append a value to a list, only if the list exists",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "RPUSHX mylist World",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::RpushX),
    CommandHolder(GEN_CMD_STRING("SADD"),
                  "<key> <member> [member ...]",
                  "Add one or more members to a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SADD myset World",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Sadd),
    CommandHolder(GEN_CMD_STRING("SAVE"),
                  "-",
                  "Synchronously save the dataset to disk",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SAVE",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Save),
    CommandHolder(GEN_CMD_STRING(DB_SCAN_COMMAND),
                  "<cursor> [MATCH pattern] [COUNT count]",
                  "Incrementally iterate the keys space",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  DB_SCAN_COMMAND " 0 MATCH * COUNT 10",
                  1,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::Scan),
    CommandHolder(GEN_CMD_STRING(DB_JSONDUMP_COMMAND),
                  "<cursor> <PATH> absolute_path [MATCH pattern] [COUNT count]",
                  "Dump DB into json file by path.",
                  UNDEFINED_SINCE,
                  DB_JSONDUMP_COMMAND " 0 PATH ~/dump.json MATCH * COUNT 10",
                  3,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::JsonDump),
    CommandHolder(GEN_CMD_STRING(DB_CSVDUMP_COMMAND),
                  "<cursor> PATH <absolute_path> [MATCH pattern] [COUNT count]",
                  "Dump DB into csv file by path.",
                  UNDEFINED_SINCE,
                  DB_CSVDUMP_COMMAND " 0 PATH ~/dump.csv MATCH * COUNT 10",
                  3,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::CsvDump),
    CommandHolder(GEN_CMD_STRING(DB_STORE_VALUE_COMMAND),
                  "<key> PATH <absolute_path>",
                  "Save value to file by path.",
                  UNDEFINED_SINCE,
                  DB_STORE_VALUE_COMMAND " key PATH ~/key_raw",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::StoreValue),
    CommandHolder(GEN_CMD_STRING("SCARD"),
                  "<key>",
                  "Get the number of members in a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SCARD mykey",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Scard),

    CommandHolder(GEN_CMD_STRING("SCRIPT DEBUG"),
                  "<YES|SYNC|NO>",
                  "Set the debug mode for executed scripts.",
                  PROJECT_VERSION_GENERATE(3, 2, 0),
                  "SCRIPT DEBUG YES",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ScriptDebug),
    CommandHolder(GEN_CMD_STRING("SCRIPT EXISTS"),
                  "<script> [script ...]",
                  "Check existence of scripts in the script cache.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "SCRIPT EXISTS some",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ScriptExists),
    CommandHolder(GEN_CMD_STRING("SCRIPT FLUSH"),
                  "-",
                  "Remove all the scripts from the script cache.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "SCRIPT FLUSH",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ScriptFlush),
    CommandHolder(GEN_CMD_STRING("SCRIPT KILL"),
                  "-",
                  "Kill the script currently in execution.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "SCRIPT KILL",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ScriptKill),
    CommandHolder(GEN_CMD_STRING("SCRIPT LOAD"),
                  "<script>",
                  "Load the specified Lua script into the script cache.",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "SCRIPT LOAD some",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ScriptLoad),

    CommandHolder(GEN_CMD_STRING("SDIFF"),
                  "<key> [key ...]",
                  "Subtract multiple sets",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SDIFF mykey",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Sdiff),
    CommandHolder(GEN_CMD_STRING("SDIFFSTORE"),
                  "<destination> <key> [key ...]",
                  "Subtract multiple sets and store the resulting set in a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SDIFFSTORE key key1 key2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::SdiffStore),
    CommandHolder(GEN_CMD_STRING(DB_SELECTDB_COMMAND),
                  "<index>",
                  "Change the selected database for the current connection",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_SELECTDB_COMMAND " 0",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Select),
    CommandHolder(GEN_CMD_STRING(DB_SET_KEY_COMMAND),
                  "<key> <value> [EX seconds] [PX milliseconds] [NX|XX]",
                  "Set the string value of a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_SET_KEY_COMMAND " key value",
                  2,
                  5,
                  CommandInfo::Native,
                  &CommandsApi::Set),
    CommandHolder(GEN_CMD_STRING("SETBIT"),
                  "<key> <offset> <value>",
                  "Sets or clears the bit at offset in the string value stored at key",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "SETBIT mykey 7 1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SetBit),
    CommandHolder(GEN_CMD_STRING("SETEX"),
                  "<key> <seconds> <value>",
                  "Set the value and expiration of a key",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "SETEX key 10 value",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SetEx),
    CommandHolder(GEN_CMD_STRING("SETNX"),
                  "<key> <value>",
                  "Set the value of a key, only if the key does not exist",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SETNX mykey some",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SetNX),
    CommandHolder(GEN_CMD_STRING("SETRANGE"),
                  "<key> <offset> <value>",
                  "Overwrite part of a string at key starting at the specified offset",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "SETRANGE key1 6 Redis",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SetRange),
    CommandHolder(GEN_CMD_STRING("SHUTDOWN"),
                  "[NOSAVE|SAVE]",
                  "Synchronously save the dataset to disk and then shut down the server",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SHUTDOWN",
                  0,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Shutdown),
    CommandHolder(GEN_CMD_STRING("SINTER"),
                  "<key> [key ...]",
                  "Intersect multiple sets",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SINTER key1 key2",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Sinter),
    CommandHolder(GEN_CMD_STRING("SINTERSTORE"),
                  "<destination> <key> [key ...]",
                  "Intersect multiple sets and store the resulting set in a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SINTERSTORE key key1 key2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::SinterStore),
    CommandHolder(GEN_CMD_STRING("SISMEMBER"),
                  "<key> <member>",
                  "Determine if a given value is a member of a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SISMEMBER myset one",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SisMember),
    CommandHolder(GEN_CMD_STRING("SLAVEOF"),
                  "<host> <port>",
                  "Make the server a slave of another instance, or promote it as master",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SLAVEOF localhost 1111",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SlaveOf),
    CommandHolder(GEN_CMD_STRING("REPLICAOF"),
                  "<host> <port>",
                  "Make the server a slave of another instance, or promote it as master",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "REPLICAOF localhost 1111",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ReplicaOf),
    CommandHolder(GEN_CMD_STRING("SLOWLOG"),
                  "<subcommand> [argument]",
                  "Manages the Redis slow queries log",
                  PROJECT_VERSION_GENERATE(2, 2, 12),
                  "SLOWLOG get 2",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::SlowLog),
    CommandHolder(GEN_CMD_STRING("SMEMBERS"),
                  "<key>",
                  "Get all the members in a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SMEMBERS myset",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Smembers),
    CommandHolder(GEN_CMD_STRING("SMOVE"),
                  "<source> <destination> <member>",
                  "Move a member from one set to another",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SMOVE myset myotherset ale",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Smove),
    CommandHolder(GEN_CMD_STRING("SORT"),
                  "<key> [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE "
                  "destination]",
                  "Sort the elements in a list, set or sorted set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SORT mylist LIMIT 0 5 ALPHA DESC",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Sort),
    CommandHolder(GEN_CMD_STRING("SPOP"),
                  "<key> [count]",
                  "Remove and return one or multiple random members from a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SPOP myset",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Spop),
    CommandHolder(GEN_CMD_STRING("SRANDMEMBER"),
                  "<key> [count]",
                  "Get one or multiple random members from a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SRANDMEMBER myset 2",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::SRandMember),
    CommandHolder(GEN_CMD_STRING("SREM"),
                  "<key> <member> [member ...]",
                  "Remove one or more members from a set",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SREM myset one",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Srem),
    CommandHolder(GEN_CMD_STRING("SSCAN"),
                  "<key> <cursor> [MATCH pattern] [COUNT count]",
                  "Incrementally iterate Set elements",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  "SSCAN myset 0 MATCH * COUNT 100",
                  2,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::Sscan),
    CommandHolder(GEN_CMD_STRING("STRLEN"),
                  "<key>",
                  "Get the length of the value stored in a key",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "STRLEN mykey",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::StrLen),
    CommandHolder(GEN_CMD_STRING(DB_SUBSCRIBE_COMMAND),
                  "<channel> [channel ...]",
                  "Listen for messages published to the "
                  "given channels",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  DB_SUBSCRIBE_COMMAND " chan",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Subscribe),
    CommandHolder(GEN_CMD_STRING("SUNION"),
                  "<key> [key ...]",
                  "Add multiple sets",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SUNION key1 key2",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Sunion),
    CommandHolder(GEN_CMD_STRING("SUNIONSTORE"),
                  "<destination> <key> [key ...]",
                  "Add multiple sets and store the resulting set in a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SUNIONSTORE key key1 key2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::SunionStore),
    CommandHolder(GEN_CMD_STRING("SYNC"),
                  "-",
                  "Internal command used for replication",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SYNC",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Sync),
    CommandHolder(GEN_CMD_STRING("PSYNC"),
                  "-",
                  "Internal command used for replication",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "PSYNC",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Sync),
    CommandHolder(GEN_CMD_STRING("TIME"),
                  "-",
                  "Return the current server time",
                  PROJECT_VERSION_GENERATE(2, 6, 0),
                  "TIME",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Time),
    CommandHolder(GEN_CMD_STRING(DB_GET_TTL_COMMAND),
                  "<key>",
                  "Get the time to live for a key",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  DB_GET_TTL_COMMAND " key",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetTTL),
    CommandHolder(GEN_CMD_STRING(DB_KEY_TYPE_COMMAND),
                  "<key>",
                  "Determine the type stored at key",
                  UNDEFINED_SINCE,
                  DB_KEY_TYPE_COMMAND " key",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::GetType),
    CommandHolder(GEN_CMD_STRING("UNSUBSCRIBE"),
                  "[channel [channel ...]]",
                  "Stop listening for messages posted to the given channels",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "UNSUBSCRIBE chan",
                  0,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Unsubscribe),
    CommandHolder(GEN_CMD_STRING("UNWATCH"),
                  "-",
                  "Forget about all watched keys",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "UNWATCH",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Unwatch),
    CommandHolder(
        GEN_CMD_STRING("WAIT"),
        "<numslaves> <timeout>",
        "Wait for the synchronous replication of all the write commands sent in the context of the current connection",
        PROJECT_VERSION_GENERATE(3, 0, 0),
        "WAIT 1 0",
        2,
        0,
        CommandInfo::Native,
        &CommandsApi::Wait),
    CommandHolder(GEN_CMD_STRING("WATCH"),
                  "<key> [key ...]",
                  "Watch the given keys to determine execution of the MULTI/EXEC block",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "WATCH mykey",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Watch),
    CommandHolder(GEN_CMD_STRING("ZADD"),
                  "<key> [NX|XX] [CH] [INCR] <score> <member> [score member ...]",
                  "Add one or more members to a sorted set, or update its score if it already exists",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZADD myzset 2 two 3 three",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Zadd),
    CommandHolder(GEN_CMD_STRING("ZCARD"),
                  "<key>",
                  "Get the number of members in a sorted set",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZCARD some",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Zcard),
    CommandHolder(GEN_CMD_STRING("ZCOUNT"),
                  "<key> <min> <max>",
                  "Count the members in a sorted set with scores within the given values",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZCOUNT myzset -inf +inf",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Zcount),
    CommandHolder(GEN_CMD_STRING("ZINCRBY"),
                  "<key> <increment> <member>",
                  "Increment the score of a member in a sorted set",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZINCRBY myzset 2 one",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZincrBy),
    CommandHolder(GEN_CMD_STRING("ZINTERSTORE"),
                  "<destination> <numkeys> <key> [key ...] [WEIGHTS weight] [AGGREGATE SUM|MIN|MAX]",
                  "Intersect multiple sorted sets and store the resulting sorted set in a new key",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZINTERSTORE out 2 zset1 zset2 WEIGHTS 2 3",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ZincrStore),
    CommandHolder(GEN_CMD_STRING("ZLEXCOUNT"),
                  "<key> <min> <max>",
                  "Count the number of members in a sorted set between a given lexicographical range",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "ZLEXCOUNT myzset - +",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZlexCount),
    CommandHolder(GEN_CMD_STRING("ZRANGE"),
                  "<key> <start> <stop> [WITHSCORES]",
                  "Return a range of members in a sorted set, by index",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZRANGE myzset 0 -1",
                  3,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Zrange),
    CommandHolder(GEN_CMD_STRING("ZRANGEBYLEX"),
                  "<key> <min> <max> [LIMIT offset count]",
                  "Return a range of members in a sorted set, by lexicographical range",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "ZRANGEBYLEX myzset - (c",
                  3,
                  3,
                  CommandInfo::Native,
                  &CommandsApi::ZrangeByLex),
    CommandHolder(GEN_CMD_STRING("ZRANGEBYSCORE"),
                  "<key> <min> <max> [WITHSCORES] [LIMIT offset count]",
                  "Return a range of members in a sorted set, by score",
                  PROJECT_VERSION_GENERATE(1, 0, 5),
                  "ZRANGEBYSCORE myzset 1 2",
                  3,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::ZrangeByScore),
    CommandHolder(GEN_CMD_STRING("ZRANK"),
                  "<key> <member>",
                  "Determine the index of a member in a sorted set",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZRANK myzset fif",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Zrank),
    CommandHolder(GEN_CMD_STRING("ZREM"),
                  "<key> <member> [member ...]",
                  "Remove one or more members from a sorted set",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZREM myzset some",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Zrem),
    CommandHolder(GEN_CMD_STRING("ZREMRANGEBYLEX"),
                  "<key> <min> <max>",
                  "Remove all members in a sorted set between the given lexicographical range",
                  PROJECT_VERSION_GENERATE(2, 8, 9),
                  "ZREMRANGEBYLEX myzset [alpha [omega",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZremRangeByLex),
    CommandHolder(GEN_CMD_STRING("ZREMRANGEBYRANK"),
                  "<key> <start> <stop>",
                  "Remove all members in a sorted set within the given indexes",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZREMRANGEBYRANK myzset 0 1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZremRangeByRank),
    CommandHolder(GEN_CMD_STRING("ZREMRANGEBYSCORE"),
                  "<key> <min> <max>",
                  "Remove all members in a sorted set within the given scores",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZREMRANGEBYSCORE myzset -inf (2",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZremRangeByScore),
    CommandHolder(GEN_CMD_STRING("ZREVRANGE"),
                  "<key> <start> <stop> [WITHSCORES]",
                  "Return a range of members in a sorted set, by index, with scores ordered from high to low",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZREVRANGE myzset 2 3",
                  3,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::ZrevRange),
    CommandHolder(
        GEN_CMD_STRING("ZREVRANGEBYLEX"),
        "<key> <max> <min> [LIMIT offset count]",
        "Return a range of members in a sorted set, by lexicographical range, ordered from higher to lower strings.",
        PROJECT_VERSION_GENERATE(2, 8, 9),
        "ZREVRANGEBYLEX myzset (c -",
        3,
        3,
        CommandInfo::Native,
        &CommandsApi::ZrevRangeByLex),
    CommandHolder(GEN_CMD_STRING("ZREVRANGEBYSCORE"),
                  "<key> <max> <min> [WITHSCORES] [LIMIT offset count]",
                  "Return a range of members in a sorted set, by score, with scores ordered from high to low",
                  PROJECT_VERSION_GENERATE(2, 2, 0),
                  "ZREVRANGEBYSCORE myzset 2 1",
                  3,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::ZrevRangeByScore),
    CommandHolder(GEN_CMD_STRING("ZREVRANK"),
                  "<key> <member>",
                  "Determine the index of a member in a sorted set, with scores ordered from high to low",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZREVRANK myzset one",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::ZrevRank),
    CommandHolder(GEN_CMD_STRING("ZSCAN"),
                  "<key> <cursor> [MATCH pattern] [COUNT count]",
                  "Incrementally iterate sorted sets elements and associated scores",
                  PROJECT_VERSION_GENERATE(2, 8, 0),
                  "ZSCAN mylet 0",
                  2,
                  4,
                  CommandInfo::Native,
                  &CommandsApi::Zscan),
    CommandHolder(GEN_CMD_STRING("ZSCORE"),
                  "<key> <member>",
                  "Get the score associated with the given member in a sorted set",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZSCORE myzset one",
                  2,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Zscore),
    CommandHolder(GEN_CMD_STRING("ZUNIONSTORE"),
                  "<destination> <numkeys> <key> [key ...] [WEIGHTS weight] [AGGREGATE SUM|MIN|MAX]",
                  "Add multiple sorted sets and store the resulting sorted set in a new key",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "ZUNIONSTORE out 2 zset1 zset2 WEIGHTS 2 3",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::ZunionStore),
    CommandHolder(GEN_CMD_STRING("BZPOPMIN"),
                  "<key> [key ...] <timeout>",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "BZPOPMIN zset1 zset2 0",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::BZpopMin),
    CommandHolder(GEN_CMD_STRING("BZPOPMAX"),
                  "<key> [key ...] <timeout>",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "BZPOPMAX zset1 zset2 0",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::BZpopMax),
    CommandHolder(GEN_CMD_STRING("ZPOPMIN"),
                  "<key> [count]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "ZPOPMIN myzset",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::ZpopMin),
    CommandHolder(GEN_CMD_STRING("ZPOPMAX"),
                  "<key> [count]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "ZPOPMAX myzset",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::ZpopMax),
#if defined(PRO_VERSION)
    CommandHolder(GEN_CMD_STRING("XFASTOSET"),
                  "<sid ><key> <value> [sid key value ...]",
                  "Create stream with one or multiple values",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XFASTOSET sid1 0 1 sid2 2 3",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::XFastoSet),
    CommandHolder(GEN_CMD_STRING("XLEN"),
                  "<key> <arg>",
                  "Returns the number of entries inside a stream.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XLEN mystream",
                  1,
                  1,
                  CommandInfo::Native,
                  &CommandsApi::Xlen),
    CommandHolder(GEN_CMD_STRING("XRANGE"),
                  "<key> <arg> <arg> <arg> [options...]",
                  "The command returns the stream entries matching a given range of IDs.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XRANGE writers - + COUNT 2",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xrange),
    CommandHolder(GEN_CMD_STRING("XREVRANGE"),
                  "<key> <arg> <arg> <arg> [options...]",
                  "This command is exactly like XRANGE, but with the notable difference of returning the entries in "
                  "reverse order, and also taking the start-end range in reverse order: in XREVRANGE you need to state "
                  "the end ID and later the start ID, and the command will produce all the element between (or exactly "
                  "like) the two IDs, starting from the end side.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XREVRANGE writers + - COUNT 1",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xrevrange),
    CommandHolder(GEN_CMD_STRING("XREAD"),
                  "<key> <arg> [options...]",
                  "Read data from one or multiple streams, only returning entries with an ID greater than the last "
                  "received ID reported by the caller.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XREAD COUNT 2 STREAMS mystream writers 0-0 0-0",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xread),
    CommandHolder(GEN_CMD_STRING("XADD"),
                  "<key> <id> <field> <string> [field string...]",
                  "Appends the specified stream entry to the stream at the specified key.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XADD mystream * name Sara surname OConnor",
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xadd),
    CommandHolder(GEN_CMD_STRING("XACK"),
                  "<key> <group> <messageId1> [messageIdN...]",
                  "The XACK command removes one or multiple messages from the pending entries list (PEL) of a stream "
                  "consumer group.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XACK mystream mygroup 1526569495631-0",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xack),
    CommandHolder(GEN_CMD_STRING("XREADGROUP"),
                  "GROUP <name> <consumer> [BLOCK timeout] [COUNT n] [NOACK] "
                  "STREAMS key1 [keyN] offset1 [offsetN]",
                  "The XREADGROUP command is a special version of the XREAD command with support for consumer groups.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XREADGROUP GroupName ConsumerName BLOCK 2000 COUNT 10 STREAMS mystream",
                  6,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::XreadGroup),
    CommandHolder(
        GEN_CMD_STRING("XGROUP"),
        "CREATE <stream> <group>",
        "This command is used in order to manage the consumer groups associated with a stream data structure.",
        PROJECT_VERSION_GENERATE(5, 0, 0),
        "XGROUP CREATE mystream name",
        3,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Native,
        &CommandsApi::XGroup),
    CommandHolder(GEN_CMD_STRING("XCLAIM"),
                  "<key> <group-name> <consumer-name> <min-idle-time> <ID-1> <ID-2>",
                  "In the context of a stream consumer group, this command changes the ownership of a pending message, "
                  "so that the new owner is the consumer specified as the command argument.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XCLAIM mystream mygroup Alice 3600000 1526569498055-0",
                  6,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xclaim),
    CommandHolder(
        GEN_CMD_STRING("XTRIM"),
        "<stream> MAXLEN <couny>",
        "XTRIM trims the stream to a given number of items, evicting older items (items with lower IDs) if needed.",
        PROJECT_VERSION_GENERATE(5, 0, 0),
        "XTRIM mystream MAXLEN 2",
        3,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Native,
        &CommandsApi::XTrim),
    CommandHolder(GEN_CMD_STRING("XSETID"),
                  "<stream> <groupname> <id>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XSETID stream name 1",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::XSetID),
    CommandHolder(GEN_CMD_STRING("XPENDING"),
                  "<key> <group-name>",
                  "Fetching data from a stream via a consumer group, and not acknowledging such data, has the effect "
                  "of creating pending entries. ",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XPENDING mystream group55",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::XPending),
    CommandHolder(GEN_CMD_STRING("XINFO"),
                  "<command> <key>",
                  "This is an introspection command used in order to retrieve different information about the streams "
                  "and associated consumer groups. ",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XINFO CONSUMERS mystream mygroup",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xinfo),
    CommandHolder(GEN_CMD_STRING("XDEL"),
                  "<key> <ID>",
                  "Removes the specified entries from a stream, and returns the number of entries deleted, that may be "
                  "different from the number of IDs passed to the command in case certain IDs do not exist.",
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "XDEL mystream 1538561700640-0",
                  6,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Xdel),
    CommandHolder(GEN_CMD_STRING("SENTINEL MASTERS"),
                  "-",
                  "Show a list of monitored masters and their state.",
                  UNDEFINED_SINCE,
                  "SENTINEL MASTERS",
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelMasters),
    CommandHolder(GEN_CMD_STRING("SENTINEL MASTER"),
                  "<master_name>",
                  "Show the state and info of the specified master.",
                  UNDEFINED_SINCE,
                  "SENTINEL MASTER test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelMaster),
    CommandHolder(GEN_CMD_STRING("SENTINEL SLAVES"),
                  "<master_name>",
                  "Show a list of slaves for this master, "
                  "and their state.",
                  UNDEFINED_SINCE,
                  "SENTINEL SLAVES test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelSlaves),
    CommandHolder(GEN_CMD_STRING("SENTINEL SENTINELS"),
                  "<master_name>",
                  "Show a list of sentinel instances for "
                  "this master, and their state.",
                  UNDEFINED_SINCE,
                  "SENTINEL SENTINELS test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelSentinels),
    CommandHolder(GEN_CMD_STRING("SENTINEL GET-MASTER-ADDR-BY-NAME"),
                  "<master_name>",
                  "Return the ip and port number of the master with that name.\nIf a failover is in progress or "
                  "terminated successfully for this master it returns the address and port of the promoted slave.",
                  UNDEFINED_SINCE,
                  "SENTINEL GET-MASTER-ADDR-BY-NAME test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelGetMasterAddrByName),
    CommandHolder(GEN_CMD_STRING("SENTINEL RESET"),
                  "<pattern>",
                  "This command will reset all the masters with matching name.\nThe pattern argument is a glob-style "
                  "pattern.\nThe reset process clears any previous state in a master (including a failover in "
                  "progress), and removes every slave and sentinel already discovered and associated with the master.",
                  UNDEFINED_SINCE,
                  "SENTINEL RESET *",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelReset),
    CommandHolder(GEN_CMD_STRING("SENTINEL FAILOVER"),
                  "<master_name>",
                  "Force a failover as if the master was not reachable, and without asking for agreement to other "
                  "GetSentinels (however a new version of the configuration will be published so that the other "
                  "GetSentinels will update their configurations).",
                  UNDEFINED_SINCE,
                  "SENTINEL FAILOVER test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelFailover),
    CommandHolder(GEN_CMD_STRING("SENTINEL CKQUORUM"),
                  "<master_name>",
                  "Check if the current Sentinel configuration is able to reach the quorum needed to failover a "
                  "master, and the majority needed to authorize the failover.\nThis command should be used in "
                  "monitoring systems to check if a Sentinel deployment is ok.",
                  UNDEFINED_SINCE,
                  "SENTINEL CKQUORUM test",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelCkquorum),
    CommandHolder(
        GEN_CMD_STRING("SENTINEL FLUSHCONFIG"),
        "-",
        "Force Sentinel to rewrite its configuration on disk, including the current Sentinel state.\nNormally Sentinel "
        "rewrites the configuration every time something changes in its state (in the context of the subset of the "
        "state which is persisted on disk across restart).\nHowever sometimes it is possible that the configuration "
        "file is lost because of operation errors, disk failures, package upgrade scripts or configuration "
        "managers.\nIn those cases a way to to force Sentinel to rewrite the configuration file is handy.\nThis "
        "command works even if the previous configuration file is completely missing.",
        UNDEFINED_SINCE,
        "SENTINEL FLUSHCONFIG",
        0,
        0,
        CommandInfo::Native,
        &CommandsApi::SentinelFlushConfig),
    CommandHolder(
        GEN_CMD_STRING("SENTINEL MONITOR"),
        "<name> <ip> <port> <quorum>",
        "This command tells the Sentinel to start monitoring a new master with the specified name, ip, port, and "
        "quorum.\nIt is identical to the sentinel monitor configuration directive in sentinel.conf configuration file, "
        "with the difference that you can't use an hostname in as ip, but you need to provide an IPv4 or IPv6 address.",
        UNDEFINED_SINCE,
        "SENTINEL MONITOR test localhost 3333 1",
        4,
        0,
        CommandInfo::Native,
        &CommandsApi::SentinelMonitor),
    CommandHolder(GEN_CMD_STRING("SENTINEL REMOVE"),
                  "<name>",
                  "Used in order to remove the specified master: the master will no longer be monitored, and will "
                  "totally be removed from the internal state of the Sentinel, so it will no longer listed by SENTINEL "
                  "masters and so forth.",
                  UNDEFINED_SINCE,
                  "SENTINEL REMOVE name",
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelRemove),
    CommandHolder(GEN_CMD_STRING("SENTINEL SET"),
                  "<name> <option> <value>",
                  "The SET command is very similar to the CONFIG SET command of Redis, and is used in order to change "
                  "configuration parameters of a specific master.\nMultiple option / value pairs can be specified (or "
                  "none at all).\nAll the configuration parameters that can be configured via sentinel.conf are also "
                  "configurable using the SET command.",
                  UNDEFINED_SINCE,
                  "SENTINEL SET test password some",
                  3,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::SentinelSet),
#endif
    // extended
    CommandHolder(GEN_CMD_STRING("LFASTOSET"),
                  "<key> <value> [value ...]",
                  "Create list with one or multiple values",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "LFASTOSET list val1 val2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::LFastoSet),
    CommandHolder(GEN_CMD_STRING("SFASTOSET"),
                  "<key> <value> [value ...]",
                  "Create set with one or multiple values",
                  PROJECT_VERSION_GENERATE(1, 0, 0),
                  "SFASTOSET list val1 val2",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::SFastoSet),
    CommandHolder(GEN_CMD_STRING("ZFASTOSET"),
                  "<key> [NX|XX] [CH] [INCR] <score> <member> [score member ...]",
                  "Add one or more members to a sorted set, or update its score if it already exists",
                  PROJECT_VERSION_GENERATE(1, 2, 0),
                  "ZFASTOSET zset 2 two 3 three",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::ZFastoSet),
    CommandHolder(GEN_CMD_STRING("HFASTOSET"),
                  "<key> <field> <value> [field value ...]",
                  "Set multiple hash fields to multiple values",
                  PROJECT_VERSION_GENERATE(2, 0, 0),
                  "HFASTOSET hash kl value",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::HFastoSet),
    CommandHolder(GEN_CMD_STRING("LATENCY"),
                  "<arg> <arg>  [options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::Latency),
    CommandHolder(GEN_CMD_STRING("PFDEBUG"),
                  "<arg> <arg> <arg> [options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::PFDebug),
    CommandHolder(GEN_CMD_STRING("REPLCONF"),
                  "<arg> [options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::ReplConf),
    CommandHolder(GEN_CMD_STRING("SUBSTR"),
                  "<key> <arg> <arg> <arg>",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  4,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::Substr),
    CommandHolder(GEN_CMD_STRING("PFSELFTEST"),
                  "<arg>",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::PFSelfTest),
    CommandHolder(GEN_CMD_STRING("LOLWUT"),
                  "-",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::LolWut),
    CommandHolder(GEN_CMD_STRING("ACL"),
                  "<subcommand> [arg ...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  "ACL <subcommand> arg arg ...",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Native,
                  &CommandsApi::Acl),
    CommandHolder(GEN_CMD_STRING("HELLO"),
                  "-",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(5, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  0,
                  CommandInfo::Native,
                  &CommandsApi::Hello),
#if defined(PRO_VERSION)
    CommandHolder(GEN_CMD_STRING("MODULE LIST"),
                  "[options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "MODULE LIST",
                  0,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::ModuleList),
    CommandHolder(GEN_CMD_STRING("MODULE LOAD"),
                  "<module_path> [options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "MODULE LOAD /home/sasha/Downloads/redis-graph/src/redisgraph.so",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::ModuleLoad),
    CommandHolder(GEN_CMD_STRING("MODULE UNLOAD"),
                  "<module_name> [options ...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  "MODULE UNLOAD graph",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::ModuleUnLoad),
#endif
    CommandHolder(GEN_CMD_STRING("MEMORY DOCTOR"),
                  "-",
                  "Outputs memory problems report",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::MemoryDoctor),
    CommandHolder(GEN_CMD_STRING("MEMORY USAGE"),
                  "<key>",
                  "Estimate memory usage of key",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::MemoryUsage),
    CommandHolder(GEN_CMD_STRING("MEMORY STATS"),
                  "-",
                  "Show memory usage details",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::MemoryStats),
    CommandHolder(GEN_CMD_STRING("MEMORY PURGE"),
                  "-",
                  "Ask the allocator to release memory",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::MemoryPurge),
    CommandHolder(GEN_CMD_STRING("MEMORY MALLOC-STATS"),
                  "-",
                  "Show allocator internal stats",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::MemoryMallocStats),
    CommandHolder(GEN_CMD_STRING("SWAPDB"),
                  "<db1> <db2> [arg]",
                  "Swap db",
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::SwapDB),
    CommandHolder(GEN_CMD_STRING("UNLINK"),
                  "<key> [key ...]",
                  "Delete a key",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  "UNLINK key",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::Unlink),
    CommandHolder(GEN_CMD_STRING("TOUCH"),
                  "<key> <arg> [options...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::Touch),
    CommandHolder(GEN_CMD_STRING("ASKING"),
                  "<key> <arg> [options...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::Asking),
    CommandHolder(GEN_CMD_STRING("RESTORE-ASKING"),
                  "<key> <arg> <arg> <arg> [options...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::RestoreAsking),

    CommandHolder(GEN_CMD_STRING("GEORADIUS_RO"),
                  "<key> <arg> <arg> <arg> <arg> <arg> [options...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  6,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::GeoRadius_ro),
    CommandHolder(GEN_CMD_STRING("GEORADIUSBYMEMBER_RO"),
                  "<key> <arg> <arg> <arg> <arg> [options...]",
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  5,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::GeoRadiusByMember_ro),
#if defined(PRO_VERSION)
    // redis-graph api
    CommandHolder(GEN_CMD_STRING(REDIS_GRAPH_MODULE_COMMAND("QUERY")),
                  "<Graph name> <Query>",
                  "Executes the given query against a specified graph.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_GRAPH_MODULE_COMMAND("QUERY") " us_government \"MATCH (p:president)-[:born]->(:state "
                                                      "{name:Hawaii}) RETURN p\"",
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::GraphQuery),
    CommandHolder(GEN_CMD_STRING(REDIS_GRAPH_MODULE_COMMAND("EXPLAIN")),
                  "<Graph name> <Query>",
                  "Constructs a query execution plan but does not run it. Inspect this execution plan to better "
                  "understand how your query will get executed.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_GRAPH_MODULE_COMMAND("EXPLAIN") " us_government \"MATCH (p:president)-[:born]->(h:state "
                                                        "{name:Hawaii}) RETURN p\"",
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::GraphExplain),
    CommandHolder(GEN_CMD_STRING(REDIS_GRAPH_MODULE_COMMAND("DELETE")),
                  "<Graph name>",
                  "Delete graph by name",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_GRAPH_MODULE_COMMAND("DELETE") " us_government",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::GraphDelete),
    // redisearch
    CommandHolder(
        GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("CREATE")),
        "<index_name> [field weight ...]",
        "Creates an index with the given spec. The index name will be used in all the key names so keep it short!",
        PROJECT_VERSION_GENERATE(4, 0, 0),
        REDIS_SEARCH_MODULE_COMMAND("CREATE") " docs title 2.0 body 1.0 url 1.5",
        3,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Extended,
        &CommandsApi::FtCreate),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("ADD")),
                  "<index_name> <doc_id> <score> [NOSAVE] FIELDS [field content ...]",
                  "Add a documet to the index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("ADD") " docs doc1 1.0 FIELDS title ???war and peace??? body \"Well, "
                                                     "Prince, so Genoa and Lucca are now???\"",
                  6,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtAdd),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("ADDHASH")),
                  "<index> <docId> <score> [LANGUAGE language] [REPLACE]",
                  "Add a documet to the index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  3,
                  CommandInfo::Extended,
                  &CommandsApi::FtAddHash),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("INFO")),
                  "<index>",
                  "Return information and statistics on the index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("INFO") " wik{0}",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtInfo),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SEARCH")),
                  "<index> <query> [NOCONTENT] [LIMIT offset num]",
                  "Search the index with a textual query, returning either documents or just ids.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("SEARCH") " idx \"hello world\" LIMIT 0 1",
                  5,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::FtSearch),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("EXPLAIN")),
                  "<index> <query>",
                  "Return the execution plan for a complex query.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtExplain),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DEL")),
                  "<index> <doc_id>",
                  "Delete a document from the index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtDel),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("GET")),
                  "<index> <doc_id>",
                  "Returns the full contents of a document.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtGet),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("MGET")),
                  "<index> <doc_id>",
                  "Returns the full contents of multiple documents.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtMGet),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DROP")),
                  "<index>",
                  "Deletes all the keys associated with the index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtDrop),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SUGADD")),
                  "<key> <string> <score> [INCR] [PAYLOAD payload]",
                  "Add a suggestion string to an auto-complete suggestion dictionary.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  3,
                  CommandInfo::Extended,
                  &CommandsApi::FtSugadd),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SUGGET")),
                  "<key> <prefix> [FUZZY] [WITHPAYLOADS] [MAX num]",
                  "Get completion suggestions for a prefix.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  4,
                  CommandInfo::Extended,
                  &CommandsApi::FtSugget),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SUGDEL")),
                  "<key> <string>",
                  "Delete a string from a suggestion index.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtSugdel),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SUGLEN")),
                  "<key>",
                  "Get the size of an autoc-complete suggestion dictionary.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtSuglen),
    CommandHolder(
        GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("OPTIMIZE")),
        "<index>",
        "This command is deprecated. Index optimizations are done by the internal garbage collector in the background. "
        "Client libraries should not implement this command, and remove it if they haven't already.",
        PROJECT_VERSION_GENERATE(4, 0, 0),
        UNDEFINED_EXAMPLE_STR,
        1,
        0,
        CommandInfo::Extended,
        &CommandsApi::FtOptimize),

    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("CONFIG")),
                  "SET <GET|HELP> options",
                  "Retrieves, describes and sets runtime configuration options.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  0,
                  3,
                  CommandInfo::Extended,
                  &CommandsApi::FtConfig),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SYNUPDATE")),
                  "<index name> <synonym group id> <term1> <term2> ...",
                  "Adds a synonym group.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("SYNUPDATE") " <index name> <synonym group id> <term1> <term2> ...",
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtSynupdate),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("ALTER")),
                  "{index} SCHEMA ADD {field} {options} ...",
                  "Alters an existing index. Currently, adding fields to the index is the only supported alteration.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("ALTER") " {index} SCHEMA ADD {field} {options} ...",
                  5,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtAlter),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DICTDUMP")),
                  "{dict}",
                  "Dumps all terms in the given dictionary.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("DICTDUMP") " {dict}",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtDictdump),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DEBUG")),
                  "",
                  "Debug.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("DEBUG"),
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtDebug),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DICTDEL")),
                  "{dict} {term} [{term} ...]",
                  "Deletes terms from a dictionary.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("DICTDEL") " {dict} {term} [{term} ...]",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtDictdel),
    CommandHolder(
        GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("AGGREGATE")),
        "{index_name} {query_string}[WITHSCHEMA][VERBATIM][LOAD{nargs} {property}...][GROUPBY{nargs} {property}... "
        "REDUCE{func} {nargs} {arg}...[AS{name : string}]...]...[SORTBY{nargs} {property}[ASC | "
        "DESC]...[MAX{num}]][APPLY{expr} AS{alias}]...[LIMIT{offset} {num}]...[FILTER{expr}]... ",
        "Runs a search query on an index, and performs aggregate transformations on the results, "
        "extracting statistics etc from them.",
        PROJECT_VERSION_GENERATE(4, 0, 0),
        REDIS_SEARCH_MODULE_COMMAND(
            "AGGREGATE") " {index_name} {query_string}[WITHSCHEMA][VERBATIM][LOAD{nargs} {property}...][GROUPBY{nargs} "
                         "{property}... REDUCE{func} {nargs} {arg}...[AS{name : string}]...]...[SORTBY{nargs} "
                         "{property}[ASC | DESC]...[MAX{num}]][APPLY{expr} AS{alias}]...[LIMIT{offset} "
                         "{num}]...[FILTER{expr}]... ",
        INFINITE_COMMAND_ARGS,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Extended,
        &CommandsApi::FtAggregate),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SYNFORCEUPDATE")),
                  "<index name> <synonym group id> <term1> <term2> ...",
                  "Adds a synonym group.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("SYNFORCEUPDATE") " <index name> <synonym group id> <term1> <term2> ...",
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtSynforceupdate),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("EXPLAINCLI")),
                  "",
                  "",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("EXPLAINCLI"),
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtExplaincli),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SPELLCHECK")),
                  "{index} {query} [DISTANCE dist] [TERMS {INCLUDE | EXCLUDE} {dict} [TERMS ...]]",
                  "Performs spelling correction on a query, returning suggestions for misspelled terms.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND(
                      "SPELLCHECK") " {index} {query} [DISTANCE dist] [TERMS {INCLUDE | EXCLUDE} {dict} [TERMS ...]]",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtSpellcheck),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SYNDUMP")),
                  "<index name>",
                  "Dumps the contents of a synonym group.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("SYNDUMP") " <index name>",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtSyndump),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SYNADD")),
                  "<index name> <term1> <term2> ...",
                  "Adds a synonym group.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("SYNADD") " <index name> <term1> <term2> ...",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtSynadd),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DICTADD")),
                  "{dict} {term} [{term} ...]",
                  "Adds terms to a dictionary.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("DICTADD") " {dict} {term} [{term} ...]",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::FtDictadd),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("TAGVALS")),
                  "{index} {field_name}",
                  "Returns the distinct tags indexed in a Tag field.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("TAGVALS") " {index} {field_name}",
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtTagvals),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("CURSOR")),
                  "",
                  "Cursor.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_SEARCH_MODULE_COMMAND("CURSOR"),
                  0,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::FtCursor),

    // json
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("DEL")),
                  "<key> [path]",
                  "Delete a value.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_JSON_MODULE_COMMAND("DEL") " obj",
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonDel),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("GET")),
                  "<key> [INDENT indentation-string][NEWLINE line-break-string][SPACE space-string] [path...]",
                  "Return the value at path in JSON serialized form.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_JSON_MODULE_COMMAND("GET") " myjsonkey",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::JsonGet),
    CommandHolder(
        GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("MGET")),
        "<key> [key ...] <path>",
        "Returns the values at path from multiple keys. Non-existing keys and non-existing paths are reported as null.",
        PROJECT_VERSION_GENERATE(4, 0, 0),
        UNDEFINED_EXAMPLE_STR,
        2,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Extended,
        &CommandsApi::JsonMget),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("SET")),
                  "<key> <path> <json> [NX | XX]",
                  "Sets the JSON value at path in key.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_JSON_MODULE_COMMAND("SET") " obj . {\"name\":\"Leonard\",\"seen\":1478476,\"loggedOut\": true}",
                  3,
                  2,
                  CommandInfo::Extended,
                  &CommandsApi::JsonSet),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("TYPE")),
                  "<key> [path]",
                  "Report the type of JSON value at path.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonType),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("NUMINCRBY")),
                  "<key> <path> <number>",
                  "Increments the number value stored at path by number.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::JsonNumIncrBy),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("NUMMULTBY")),
                  "<key> <path> <number>",
                  "Multiplies the number value stored at path by number.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::JsonNumMultBy),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("STRAPPEND")),
                  "<key> [path] <json-string>",
                  "Append the json-string value(s) the string at path.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonStrAppend),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("STRLEN")),
                  "<key> [path]",
                  "Report the length of the JSON String at path in key.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonStrlen),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRAPPEND")),
                  "<key> <path> <json> [json ...]",
                  "Append the json value(s) into the array at path after the last element in it.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrAppend),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRINDEX")),
                  "<key> <path> <json-scalar> [start [stop]]",
                  "Search for the first occurrence of a scalar JSON value in an array.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  2,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrIndex),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRINSERT")),
                  "<key> <path> <index> <json> [json ...]",
                  "Insert the json value(s) into the array at path before the index (shifts to the right).",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  4,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrInsert),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRLEN")),
                  "<key> [path]",
                  "Report the length of the JSON Array at path in key.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrLen),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRPOP")),
                  "<key> [path [index]]",
                  "Remove and return element from the index in the array.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  2,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrPop),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("ARRTRIM")),
                  "<key> <path> <start> <stop>",
                  "Trim an array so that it contains only the specified inclusive range of elements.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  4,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::JsonArrTrim),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("OBJKEYS")),
                  "<key> [path]",
                  "Return the keys in the object that's referenced by path.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonObjKeys),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("OBJLEN")),
                  "<key> [path]",
                  "Report the number of keys in the JSON Object at path in key.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonObjLen),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("DEBUG")),
                  "<subcommand & arguments>",
                  "Report information.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::JsonObjLen),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("FORGET")),
                  "<key> <path>",
                  "Delete a value.",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::JsonForget),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("RESP")),
                  "<key> [path]",
                  "Return the JSON in key in Redis Serialization Protocol (RESP).",
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::JsonResp),
    // nr
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("RESET")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrReset),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("INFO")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrInfo),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("GETDATA")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrGetData),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("RUN")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrRun),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("CLASS")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrClass),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("CREATE")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrCreate),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("OBSERVE")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrObserve),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("TRAIN")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrTrain),
    CommandHolder(GEN_CMD_STRING(REDIS_NR_MODULE_COMMAND("THREADS")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::NrThreads),
    // bf
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("DEBUG")),
                  "<key> [options...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::BfDebug),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("EXISTS")),
                  "<key> <item>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::BfExists),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("SCANDUMP")),
                  "<key> <iter>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::BfScanDump),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("ADD")),
                  "<key> <item>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::BfAdd),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("MEXISTS")),
                  "<key> <item> [items...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::BfMexists),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("LOADCHUNK")),
                  "<key> <iter> <data>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::BfLoadChunk),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("MADD")),
                  "<key> <item> [items...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::BfMadd),
    CommandHolder(GEN_CMD_STRING(REDIS_BLOOM_MODULE_COMMAND("RESERVE")),
                  "<key> <error_rate> <size>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  3,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::BfReserve),
    // CL
    CommandHolder(GEN_CMD_STRING(REDIS_CL_MODULE_COMMAND("THROTTLE")),
                  "<key> <max_burst> <count per period> <period> [<quantity>]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_CL_MODULE_COMMAND("THROTTLE") " user123 15 30 60 1",
                  4,
                  1,
                  CommandInfo::Extended,
                  &CommandsApi::ClThrottle),
    // TS
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("CREATE")),
                  "<key> [RETENTION retentionTime] [LABELS field value...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("CREATE") " temperature RETENTION 60 LABELS sensor_id 2 area_id 32",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSCreate),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("ALTER")),
                  "<key> [RETENTION retentionTime] [LABELS field value..]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("ALTER") " temperature LABELS sensor_id 2 area_id 32 sub_area_id 15",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSAlter),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("ADD")),
                  "<key> <timestamp> <value> [RETENTION retentionTime] [LABELS field value..]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("ADD") " temperature:3:11 1548149183000 27 RETENTION 3600",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSAdd),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("MADD")),
                  "<key> <timestamp> <value> [key timestamp value ...]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("MADD") " temperature:2:32 1548149181000 45 cpu:2:32 1548149180000 30",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSMadd),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("INCRBY")),
                  "<key> <value> [RESET time-bucket] [RETENTION retentionTime] [LABELS field value..]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("INCRBY") " temperature:2:32 1",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSIncrBy),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("DECRBY")),
                  "<key> <value> [RESET time-bucket] [RETENTION retentionTime] [LABELS field value..]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("DECRBY") " temperature:2:32 1",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSDecrBy),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("CREATERULE")),
                  "<sourceKey> <destKey> [AGGREGATION aggType timeBucket]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("CREATERULE") " temperature:2:32 temperature:2:33 AGGREGATION avg 2",
                  2,
                  3,
                  CommandInfo::Extended,
                  &CommandsApi::TSCreateRule),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("DELETERULE")),
                  "<sourceKey> <destKey>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("DELETERULE") " temperature:2:32 temperature:2:33",
                  2,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::TSDeleteRule),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("RANGE")),
                  "<key> <fromTimestamp> <toTimestamp> [AGGREGATION aggregationType timeBucket]",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("RANGE") " temperature:3:32 1548149180000 1548149210000 AGGREGATION avg 5000",
                  3,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSRange),
    CommandHolder(
        GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("MRANGE")),
        "<key> <fromTimestamp> <toTimestamp> [AGGREGATION aggregationType timeBucket]",
        UNDEFINED_SUMMARY,
        PROJECT_VERSION_GENERATE(4, 0, 0),
        REDIS_TS_MODULE_COMMAND("MRANGE") " temperature:3:32 1548149180000 1548149210000 AGGREGATION avg 5000",
        2,
        INFINITE_COMMAND_ARGS,
        CommandInfo::Extended,
        &CommandsApi::TSMrange),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("GET")),
                  "<key>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("GET") " temperature:3:32",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::TSGet),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("MGET")),
                  "FILTER filter...",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("MGET") " FILTER area_id=32",
                  2,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSMget),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("INFO")),
                  "<key>",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("INFO") " temperature:3:32",
                  1,
                  0,
                  CommandInfo::Extended,
                  &CommandsApi::TSInfo),
    CommandHolder(GEN_CMD_STRING(REDIS_TS_MODULE_COMMAND("QUERYINDEX")),
                  "filter...",
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  REDIS_TS_MODULE_COMMAND("QUERYINDEX") " area_id=32",
                  1,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Extended,
                  &CommandsApi::TSQueryIndex)
#endif
};

const ConstantCommandsArray kInternalCommands = {
#if defined(PRO_VERSION)
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("_cacheinit")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_JSON_MODULE_COMMAND("_cacheinfo")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SETPAYLOAD")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SAFEADD")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("SAFEADDHASH")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("DTADD")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING(REDIS_SEARCH_MODULE_COMMAND("TERMADD")),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  PROJECT_VERSION_GENERATE(4, 0, 0),
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
#endif
    CommandHolder(GEN_CMD_STRING("post"),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr),
    CommandHolder(GEN_CMD_STRING("host:"),
                  UNDEFINED_ARGS,
                  UNDEFINED_SUMMARY,
                  UNDEFINED_SINCE,
                  UNDEFINED_EXAMPLE_STR,
                  INFINITE_COMMAND_ARGS,
                  INFINITE_COMMAND_ARGS,
                  CommandInfo::Internal,
                  nullptr)};

}  // namespace
}  // namespace redis
template <>
const char* ConnectionTraits<REDIS>::GetBasedOn() {
  return "hiredis";
}

template <>
const char* ConnectionTraits<REDIS>::GetVersionApi() {
  return redis_compatible::GetHiredisVersion();
}

template <>
const ConstantCommandsArray& ConnectionCommandsTraits<REDIS>::GetCommands() {
  return redis::kCommands;
}

namespace internal {
template <>
common::Error Connection<redis::NativeConnection, redis::RConfig>::Connect(const redis::RConfig& config,
                                                                           redis::NativeConnection** hout) {
  redis::NativeConnection* context = nullptr;
  common::Error err = redis::CreateConnection(config, &context);
  if (err) {
    return err;
  }

  *hout = context;
  // redisEnableKeepAlive(context);
  return common::Error();
}

template <>
common::Error Connection<redis::NativeConnection, redis::RConfig>::Disconnect(redis::NativeConnection** handle) {
  redis::NativeConnection* lhandle = *handle;
  if (lhandle) {
    redisFree(lhandle);
  }
  lhandle = nullptr;
  return common::Error();
}

template <>
bool Connection<redis::NativeConnection, redis::RConfig>::IsConnected(redis::NativeConnection* handle) {
  if (!handle) {
    return false;
  }

  return true;
}

}  // namespace internal
namespace redis {

common::Error CreateConnection(const RConfig& config, NativeConnection** context) {
  return redis_compatible::CreateConnection(config, config.ssh_info, context);
}

common::Error TestConnection(const RConfig& config) {
  return redis_compatible::TestConnection(config, config.ssh_info);
}

#if defined(PRO_VERSION)
common::Error DiscoveryClusterConnection(const RConfig& config, std::vector<ServerDiscoveryClusterInfoSPtr>* infos) {
  if (!infos) {
    return common::make_error_inval();
  }

  redisContext* context = nullptr;
  common::Error err = CreateConnection(config, config.ssh_info, &context);
  if (err) {
    return err;
  }

  err = redis_compatible::AuthContext(
      context, common::ConvertToCharBytes(config.auth));  // convert from std::string to char bytes
  if (err) {
    redisFree(context);
    return err;
  }

  /* Send the GET CLUSTER command. */
  redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(context, GET_SERVER_TYPE));
  if (!reply) {
    err = common::make_error("I/O error");
    redisFree(context);
    return err;
  }

  if (reply->type == REDIS_REPLY_STRING) {
    err = redis_base::MakeDiscoveryClusterInfo(config.host, std::string(reply->str, reply->len), infos);
  } else if (reply->type == REDIS_REPLY_ERROR) {
    err = common::make_error(std::string(reply->str, reply->len));
  } else {
    DNOTREACHED();
  }

  freeReplyObject(reply);
  redisFree(context);
  return err;
}

common::Error DiscoverySentinelConnection(const RConfig& config, std::vector<ServerDiscoverySentinelInfoSPtr>* infos) {
  if (!infos) {
    return common::make_error_inval();
  }

  redisContext* context = nullptr;
  common::Error err = CreateConnection(config, config.ssh_info, &context);
  if (err) {
    return err;
  }

  err = redis_compatible::AuthContext(
      context, common::ConvertToCharBytes(config.auth));  // convert from std::string to char bytes
  if (err) {
    redisFree(context);
    return err;
  }

  /* Send the GET MASTERS command. */
  redisReply* masters_reply = reinterpret_cast<redisReply*>(redisCommand(context, GET_SENTINEL_MASTERS));
  if (!masters_reply) {
    redisFree(context);
    return common::make_error("I/O error");
  }

  for (size_t i = 0; i < masters_reply->elements; ++i) {
    redisReply* master_info = masters_reply->element[i];
    ServerCommonInfo sinf;
    common::Error lerr = redis_base::MakeServerCommonInfo(master_info, &sinf);
    if (lerr) {
      continue;
    }

    const char* master_name = sinf.name.c_str();
    ServerDiscoverySentinelInfoSPtr sent(new redis_base::DiscoverySentinelInfo(sinf));
    infos->push_back(sent);
    /* Send the GET SLAVES command. */
    redisReply* reply =
        reinterpret_cast<redisReply*>(redisCommand(context, GET_SENTINEL_SLAVES_PATTERN_1ARGS_S, master_name));
    if (!reply) {
      freeReplyObject(masters_reply);
      redisFree(context);
      return common::make_error("I/O error");
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
      for (size_t j = 0; j < reply->elements; ++j) {
        redisReply* server_info = reply->element[j];
        ServerCommonInfo slsinf;
        lerr = redis_base::MakeServerCommonInfo(server_info, &slsinf);
        if (lerr) {
          continue;
        }
        ServerDiscoverySentinelInfoSPtr lsent(new redis_base::DiscoverySentinelInfo(slsinf));
        infos->push_back(lsent);
      }
    } else if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      freeReplyObject(masters_reply);
      redisFree(context);
      return common::make_error(std::string(reply->str, reply->len));
    } else {
      DNOTREACHED();
    }
    freeReplyObject(reply);
  }

  freeReplyObject(masters_reply);
  redisFree(context);
  return common::Error();
}
#endif

#if defined(PRO_VERSION)
DBConnection::DBConnection(CDBConnectionClient* client, IModuleConnectionClient* mclient)
    : base_class(client, new CommandTranslator(base_class::GetCommands())), mclient_(mclient) {}
#else
DBConnection::DBConnection(CDBConnectionClient* client)
    : base_class(client, new CommandTranslator(base_class::GetCommands())) {}
#endif

common::Error DBConnection::GetUniImpl(const NKey& key, NDbKValue* loaded_key) {
  readable_string_t type_str;
  common::Error err = base_class::GetType(key, &type_str);
  if (err) {
    return err;
  }

  if (type_str == GEN_CMD_STRING("string")) {
    return GetImpl(key, loaded_key);
  } else if (type_str == GEN_CMD_STRING("list")) {
    return LrangeImpl(key, 0, -1, loaded_key);
  } else if (type_str == GEN_CMD_STRING("set")) {
    return SmembersImpl(key, loaded_key);
  } else if (type_str == GEN_CMD_STRING("hash")) {
    return HgetallImpl(key, loaded_key);
  } else if (type_str == GEN_CMD_STRING("zset")) {
    return ZrangeImpl(key, 0, -1, true, loaded_key);
  }
#if defined(PRO_VERSION)
  else if (type_str == GEN_CMD_STRING("stream")) {
    return XRangeImpl2(key, loaded_key);
  } else if (type_str == GEN_CMD_STRING("ReJSON-RL")) {
    return JsonGetImpl(key, loaded_key);
  }
#endif
  /* else if (type_str == GEN_CMD_STRING("trietype1")) {
    lout = core::GraphValue::TYPE_GRAPH;
  } else if (type_str == GEN_CMD_STRING("MBbloom--")) {
    lout = core::BloomValue::TYPE_BLOOM;
  } else if (type_str == GEN_CMD_STRING("ft_invidx")) {
    lout = core::SearchValue::TYPE_FT_TERM;
  } else if (type_str == GEN_CMD_STRING("ft_index0")) {
    lout = core::SearchValue::TYPE_FT_INDEX;
  }*/

  std::stringstream wr;
  wr << "Unknown type: " << common::ConvertToString(type_str);
  return common::make_error(wr.str());
}

#if defined(PRO_VERSION)
common::Error DBConnection::JsonSetImpl(const NDbKValue& key) {
  command_buffer_t set_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->CreateKeyCommand(key, &set_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, set_cmd, &reply);
  if (err) {
    return err;
  }

  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::JsonGetImpl(const NKey& key, NDbKValue* loaded_key) {
  command_buffer_t get_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->LoadKeyCommand(key, JsonValue::TYPE_JSON, &get_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, get_cmd, &reply);
  if (err) {
    return err;
  }

  if (reply->type == REDIS_REPLY_NIL) {
    // key_t key_str = key.GetKey();
    freeReplyObject(reply);
    return GenerateError(REDIS_JSON_MODULE_COMMAND("GET"), "key not found.");
  }

  if (reply->type != REDIS_REPLY_STRING) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  common::Value* val = new JsonValue(GEN_CMD_STRING_SIZE(reply->str, reply->len));
  *loaded_key = NDbKValue(key, NValue(val));
  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::JsonDelImpl(const NKey& key, long long* deleted) {
  command_buffer_t del_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->DeleteKeyCommand(key, &del_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, del_cmd, &reply);
  if (err) {
    return err;
  }

  if (reply->type != REDIS_REPLY_INTEGER) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  *deleted = reply->integer;
  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::XAddImpl(const NDbKValue& key, command_buffer_t* gen_id) {
  command_buffer_t set_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->Xadd(key, &set_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, set_cmd, &reply);
  if (err) {
    return err;
  }

  bool is_right_reply_type = reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_STRING;
  if (!is_right_reply_type) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  *gen_id = GEN_CMD_STRING_SIZE(reply->str, reply->len);
  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::XRangeImpl(const NKey& key, NDbKValue* loaded_key, fastonosql::core::FastoObject* out) {
  command_buffer_t get_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->LoadKeyCommand(key, StreamValue::TYPE_STREAM, &get_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, get_cmd, &reply);
  if (err) {
    return err;
  }

  if (reply->type == REDIS_REPLY_NIL) {
    // key_t key_str = key.GetKey();
    return GenerateError("XRANGE", "key not found.");
  }

  if (reply->type != REDIS_REPLY_ARRAY) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  err = CliFormatReplyRaw(out, reply);
  if (err) {
    return err;
  }

  StreamValue* val = new StreamValue;
  FastoObject::childs_t child = out->GetChildrens();
  common::ArrayValue* arr = static_cast<common::ArrayValue*>(child[0]->GetValue().get());
  StreamValue::streams_t streams;
  for (size_t i = 0; i < arr->GetSize(); ++i) {
    StreamValue::stream_id sid;
    std::vector<StreamValue::Entry> entr;
    common::Value* lval = nullptr;
    common::ArrayValue* inner_arr = nullptr;
    if (arr->Get(i, &lval) && lval->GetAsList(&inner_arr)) {
      common::Value* vid = nullptr;
      common::Value* lentries = nullptr;
      common::ArrayValue* entries = nullptr;
      CHECK_EQ(inner_arr->GetSize(), 2);
      if (inner_arr->Get(0, &vid) && vid->GetAsString(&sid) && inner_arr->Get(1, &lentries) &&
          lentries->GetAsList(&entries)) {
        for (size_t j = 0; j < entries->GetSize(); j += 2) {
          common::Value* entr_key = nullptr;
          common::Value* entr_val = nullptr;
          if (entries->Get(j, &entr_key) && entries->Get(j + 1, &entr_val)) {
            convert_to_t key;
            convert_to_t value;
            if (entr_key->GetAsString(&key) && entr_val->GetAsString(&value)) {
              entr.push_back(StreamValue::Entry{key, value});
            }
          }
        }
      }
      streams.push_back(StreamValue::Stream{sid, entr});
    }
  }
  val->SetStreams(streams);

  *loaded_key = NDbKValue(key, NValue(val));
  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::XRangeImpl2(const NKey& key, NDbKValue* loaded_key) {
  command_buffer_t get_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  common::Error err = tran->LoadKeyCommand(key, StreamValue::TYPE_STREAM, &get_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, get_cmd, &reply);
  if (err) {
    return err;
  }

  if (reply->type == REDIS_REPLY_NIL) {
    // key_t key_str = key.GetKey();
    return GenerateError("XRANGE", "key not found.");
  }

  if (reply->type != REDIS_REPLY_ARRAY) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  common::Value* val = nullptr;
  err = redis_compatible::ValueFromReplay(reply, &val);
  if (err) {
    delete val;
    freeReplyObject(reply);
    return err;
  }

  common::ArrayValue* arr = nullptr;
  if (!val->GetAsList(&arr)) {
    delete val;
    freeReplyObject(reply);
    return common::make_error("Conversion error array to hash");
  }

  StreamValue* stream = new StreamValue;
  StreamValue::streams_t streams;
  for (size_t i = 0; i < arr->GetSize(); ++i) {
    StreamValue::stream_id sid;
    std::vector<StreamValue::Entry> entr;
    common::Value* lval = nullptr;
    common::ArrayValue* inner_arr = nullptr;
    if (arr->Get(i, &lval) && lval->GetAsList(&inner_arr)) {
      common::Value* vid = nullptr;
      common::Value* lentries = nullptr;
      common::ArrayValue* entries = nullptr;
      CHECK_EQ(inner_arr->GetSize(), 2);
      if (inner_arr->Get(0, &vid) && vid->GetAsString(&sid) && inner_arr->Get(1, &lentries) &&
          lentries->GetAsList(&entries)) {
        for (size_t j = 0; j < entries->GetSize(); j += 2) {
          common::Value* entr_key = nullptr;
          common::Value* entr_val = nullptr;
          if (entries->Get(j, &entr_key) && entries->Get(j + 1, &entr_val)) {
            convert_to_t key;
            convert_to_t value;
            if (entr_key->GetAsString(&key) && entr_val->GetAsString(&value)) {
              entr.push_back(StreamValue::Entry{key, value});
            }
          }
        }
      }
      streams.push_back(StreamValue::Stream{sid, entr});
    }
  }
  stream->SetStreams(streams);

  delete val;
  *loaded_key = NDbKValue(key, NValue(stream));
  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::ModuleLoadImpl(const ModuleInfo& module) {
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  command_buffer_t module_load_cmd;
  common::Error err = tran->ModuleLoad(module, &module_load_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, module_load_cmd, &reply);
  if (err) {
    return err;
  }

  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::ModuleUnLoadImpl(const ModuleInfo& module) {
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  command_buffer_t module_unload_cmd;
  common::Error err = tran->ModuleUnload(module, &module_unload_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, module_unload_cmd, &reply);
  if (err) {
    return err;
  }

  freeReplyObject(reply);
  return common::Error();
}

common::Error DBConnection::GraphQuery(const commands_args_t& argv, FastoObject* out) {
  return CommonExec(argv, out);
}

common::Error DBConnection::GraphExplain(const commands_args_t& argv, FastoObject* out) {
  return CommonExec(argv, out);
}

common::Error DBConnection::GraphDelete(const commands_args_t& argv, FastoObject* out) {
  return CommonExec(argv, out);
}

common::Error DBConnection::JsonSet(const NDbKValue& key) {
  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = JsonSetImpl(key);
  if (err) {
    return err;
  }

  if (client_) {
    client_->OnAddedKey(key);
  }

  return common::Error();
}

common::Error DBConnection::JsonGet(const NKey& key, NDbKValue* loaded_key) {
  if (!loaded_key) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = JsonGetImpl(key, loaded_key);
  if (err) {
    return err;
  }

  if (client_) {
    client_->OnLoadedKey(*loaded_key);
  }

  return common::Error();
}

common::Error DBConnection::JsonDel(const NKey& key, long long* deleted) {
  if (!deleted) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = JsonDelImpl(key, deleted);
  if (err) {
    return err;
  }

  if (client_ && *deleted == 1) {
    client_->OnRemovedKeys({key});
  }

  return common::Error();
}

common::Error DBConnection::XAdd(const NDbKValue& key, readable_string_t* gen_id) {
  if (!gen_id) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = XAddImpl(key, gen_id);
  if (err) {
    return err;
  }

  if (client_) {
    client_->OnAddedKey(key);
  }

  return common::Error();
}

common::Error DBConnection::XRange(const NKey& key, NDbKValue* loaded_key, FastoObject* out) {
  if (!loaded_key || !out) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = XRangeImpl(key, loaded_key, out);
  if (err) {
    return err;
  }

  if (client_) {
    client_->OnLoadedKey(*loaded_key);
  }

  return common::Error();
}

common::Error DBConnection::XFastoSet(const NKey& key, NValue stream) {
  if (!stream || stream->GetType() != StreamValue::TYPE_STREAM) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  ttl_t ttl;
  err = base_class::GetTTL(key, &ttl);
  if (err) {
    return err;
  }

  NKeys keys;
  err = base_class::Delete({key}, &keys);
  if (err) {
    return err;
  }

  StreamValue* value = static_cast<StreamValue*>(stream.get());
  const auto streams = value->GetStreams();
  for (size_t i = 0; i < streams.size(); ++i) {
    readable_string_t gen_id;
    StreamValue* st = new StreamValue;
    st->SetStreams({streams[i]});
    err = XAddImpl(NDbKValue(key, NValue(st)), &gen_id);
    if (err) {
      return err;
    }
  }

  if (client_) {
    client_->OnAddedKey(NDbKValue(key, stream));
  }

  if (ttl == NO_TTL || ttl == EXPIRED_TTL) {
    return common::Error();
  }

  return base_class::SetTTL(key, ttl);
}

common::Error DBConnection::ModuleLoad(const ModuleInfo& module) {
  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = ModuleLoadImpl(module);
  if (err) {
    return err;
  }

  if (mclient_) {
    mclient_->OnLoadedModule(module);
  }

  return common::Error();
}

common::Error DBConnection::ModuleUnLoad(const ModuleInfo& module) {
  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  err = ModuleUnLoadImpl(module);
  if (err) {
    return err;
  }

  if (mclient_) {
    mclient_->OnUnLoadedModule(module);
  }

  return common::Error();
}

common::Error DBConnection::ClThrottle(const NKey& key,
                                       redis_int_t max_burst,
                                       redis_int_t count_per_period,
                                       redis_int_t period,
                                       redis_int_t quantity,
                                       common::ArrayValue** result) {
  if (!result) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  common::Error err = TestIsAuthenticated();
  if (err) {
    return err;
  }

  command_buffer_t throttle_cmd;
  redis_translator_t tran = GetSpecificTranslator<CommandTranslator>();
  err = tran->ClThrottle(key, max_burst, count_per_period, period, quantity, &throttle_cmd);
  if (err) {
    return err;
  }

  redisReply* reply = nullptr;
  err = redis_compatible::ExecRedisCommand(connection_.handle_, throttle_cmd, &reply);
  if (err) {
    return err;
  }

  if (reply->type != REDIS_REPLY_ARRAY) {
    DNOTREACHED() << "Unexpected type: " << reply->type;
    freeReplyObject(reply);
    return common::make_error("I/O error");
  }

  common::Value* val = nullptr;
  err = redis_compatible::ValueFromReplay(reply, &val);
  if (err) {
    delete val;
    freeReplyObject(reply);
    return err;
  }

  common::ArrayValue* arr = nullptr;
  if (!val->GetAsList(&arr)) {
    delete val;
    freeReplyObject(reply);
    return common::make_error("Conversion error");
  }

  NDbKValue dbv;
  err = GetImpl(key, &dbv);
  if (err) {
    freeReplyObject(reply);
    return err;
  }

  DCHECK(dbv.GetType() == common::Value::TYPE_STRING);
  if (client_) {
    client_->OnLoadedKey(dbv);
  }

  ttl_t ttl;
  if (client_ && arr->GetInteger64(arr->GetSize() - 1, &ttl)) {
    client_->OnLoadedKeyTTL(key, ttl);
  }

  freeReplyObject(reply);
  *result = arr;
  return common::Error();
}
#endif

bool DBConnection::IsInternalCommand(const command_buffer_t& command_name) {
  if (command_name.empty()) {
    return false;
  }

  for (size_t i = 0; i < kInternalCommands.size(); ++i) {
    const CommandHolder& cmd = kInternalCommands[i];
    if (cmd.IsEqualFirstName(command_name)) {
      return true;
    }
  }

  return false;
}

IServerInfo* DBConnection::MakeServerInfo(const std::string& content) const {
  return new ServerInfo(content);
}

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
