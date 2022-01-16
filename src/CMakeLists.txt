IF (DEVELOPER_ENABLE_COVERALLS)
  INCLUDE(Coveralls)
  #COVERALLS_TURN_ON_COVERAGE()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
ENDIF(DEVELOPER_ENABLE_COVERALLS)

SET(INTERNAL_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/internal/command_handler.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/internal/connection.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/internal/db_connection.h
)

SET(INTERNAL_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/internal/commands_api.h

  ${CMAKE_SOURCE_DIR}/src/core/internal/command_handler.cpp
  ${CMAKE_SOURCE_DIR}/src/core/internal/commands_api.cpp
  ${CMAKE_SOURCE_DIR}/src/core/internal/connection.cpp
  ${CMAKE_SOURCE_DIR}/src/core/internal/db_connection.cpp
)

SET(CONFIG_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/config/config.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/config/local_config.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/config/remote_config.h
)

SET(CONFIG_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/config/config.cpp
  ${CMAKE_SOURCE_DIR}/src/core/config/local_config.cpp
  ${CMAKE_SOURCE_DIR}/src/core/config/remote_config.cpp
)

SET(SERVER_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/server/iserver_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/server/server_discovery_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/server/state_field.h
)

SET(SERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/server/iserver_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/server/server_discovery_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/server/state_field.cpp
)

SET(SENTINEL_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/sentinel/sentinel_discovery_info.h
)

SET(SENTINEL_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/sentinel/sentinel_discovery_info.cpp
)

SET(CLUSTER_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/cluster/cluster_discovery_info.h
)

SET(CLUSTER_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/cluster/cluster_discovery_info.cpp
)

SET(DATABASE_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/database/idatabase_info.h
)

SET(DATABASE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/database/idatabase_info.cpp
)

# core engines
#redis or pika (compatible with redis protocol)
IF(BUILD_WITH_REDIS OR BUILD_WITH_PIKA OR BUILD_WITH_DYNOMITE OR BUILD_WITH_KEYDB)
  FIND_PACKAGE(OpenSSL REQUIRED)
  FIND_PACKAGE(Libssh2 REQUIRED CONFIG)
  FIND_PACKAGE(Hiredis REQUIRED)
  #ADD_SUBDIRECTORY(third-party/redis)

  SET(HEADERS_CORE_DB_REDIS_COMPATIBLE
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_compatible/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_compatible/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_compatible/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_compatible/database_info.h

    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_base/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis_base/config.h
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/internal/cluster_infos.h
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/internal/sentinel_info.h
  )
  SET(SOURCES_CORE_DB_REDIS_COMPATIBLE
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/internal/commands_api.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/internal/modules.h

    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_compatible/database_info.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/internal/cluster_infos.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis_base/internal/sentinel_info.cpp
  )

  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} $<TARGET_PROPERTY:Libssh2::libssh2,INTERFACE_INCLUDE_DIRECTORIES> ${OPENSSL_INCLUDE_DIR} ${HIREDIS_INCLUDE_DIR})
  SET(DB_LIBS ${DB_LIBS} ${HIREDIS_LIBRARIES} Libssh2::libssh2 ${OPENSSL_LIBRARIES})
  SET(DB_DEFINES ${HIREDIS_DEFINITIONS})
  IF(OS_WINDOWS)
    SET(DB_LIBS ${DB_LIBS} crypt32)
  ENDIF(OS_WINDOWS)
ENDIF(BUILD_WITH_REDIS OR BUILD_WITH_PIKA OR BUILD_WITH_DYNOMITE OR BUILD_WITH_KEYDB)

#redis
IF(BUILD_WITH_REDIS)
  SET(HAVE_REDIS ON)
  SET(HEADERS_CORE_DB_REDIS
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/redis/command_translator.h
  )
  SET(SOURCES_CORE_DB_REDIS
    ${CMAKE_SOURCE_DIR}/src/core/db/redis/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/redis/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/redis/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/redis/command_translator.cpp
  )
ENDIF(BUILD_WITH_REDIS)

#pika
IF(BUILD_WITH_PIKA)
  SET(HAVE_PIKA ON)
  SET(HEADERS_CORE_DB_PIKA
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/pika/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/pika/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/pika/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/pika/db_connection.h
  )
  SET(SOURCES_CORE_DB_PIKA
    ${CMAKE_SOURCE_DIR}/src/core/db/pika/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/pika/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/pika/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/pika/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/pika/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/pika/db_connection.cpp
  )
ENDIF(BUILD_WITH_PIKA)

#dynomite
IF(BUILD_WITH_DYNOMITE)
  SET(HAVE_DYNOMITE ON)
  SET(HEADERS_CORE_DB_DYNOMITE
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/dynomite/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/dynomite/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/dynomite/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/dynomite/server_info.h # redis server info dynomite - proxy
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/dynomite/db_connection.h
  )
  SET(SOURCES_CORE_DB_DYNOMITE
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/dynomite/db_connection.cpp
  )
ENDIF(BUILD_WITH_DYNOMITE)

#keydb
IF(BUILD_WITH_KEYDB)
  SET(HAVE_KEYDB ON)
  SET(HEADERS_CORE_DB_KEYDB
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/keydb/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/keydb/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/keydb/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/keydb/command_translator.h
  )
  SET(SOURCES_CORE_DB_KEYDB
    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/keydb/command_translator.cpp
  )
ENDIF(BUILD_WITH_KEYDB)

#memcached
IF(BUILD_WITH_MEMCACHED)
  FIND_PACKAGE(Sasl2 QUIET)
  SET(USE_SASL2 ${SASL2_FOUND})
  SET(HAVE_MEMCACHED ON)
  IF(SASL2_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${SASL2_INCLUDE_DIR})
  ENDIF(SASL2_FOUND)
  FIND_LIBRARY(MEMCACHED_LIBRARY libmemcached)
  FIND_LIBRARY(MEMCACHED_UTIL_LIBRARY libmemcachedutil)
  FIND_PATH(MEMCACHED_INCLUDE_DIR NAMES libmemcached/memcached.h PATH_SUFFIXES include)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${MEMCACHED_INCLUDE_DIR} third-party/memcached/src)

  #core
  SET(HEADERS_CORE_DB_MEMCACHED
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/memcached/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/memcached/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/memcached/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/memcached/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/memcached/database_info.h
  )
  SET(SOURCES_CORE_DB_MEMCACHED
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/memcached/database_info.cpp
  )

  SET(DB_LIBS ${DB_LIBS} ${MEMCACHED_LIBRARY} ${MEMCACHED_UTIL_LIBRARY})
  IF(USE_SASL2)
    SET(DB_LIBS ${DB_LIBS} ${SASL2_LIBRARIES})
  ENDIF(USE_SASL2)
ENDIF(BUILD_WITH_MEMCACHED)

#ssdb
IF(BUILD_WITH_SSDB)
  ADD_SUBDIRECTORY(third-party/ssdb)
  SET(HAVE_SSDB ON)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} third-party/ssdb/src)

  #core
  SET(HEADERS_CORE_DB_SSDB
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/ssdb/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/ssdb/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/ssdb/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/ssdb/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/ssdb/database_info.h
  )
  SET(SOURCES_CORE_DB_SSDB
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/ssdb/database_info.cpp
  )
  SET(DB_OBJECTS_LIBS ${DB_OBJECTS_LIBS} $<TARGET_OBJECTS:ssdb>)
ENDIF(BUILD_WITH_SSDB)

#leveldb
IF(BUILD_WITH_LEVELDB)
  FIND_PACKAGE(leveldb REQUIRED)
  SET(HAVE_LEVELDB ON)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${LEVELDB_INCLUDE_DIR})

  #core
  SET(HEADERS_CORE_DB_LEVELDB
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/database_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/leveldb/comparators/indexed_db.h
  )
  SET(SOURCES_CORE_DB_LEVELDB
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/database_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db/indexed_db_leveldb_coding.h
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db/indexed_db_key.h
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db/WebIDBTypes.h
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db/indexed_db_leveldb_coding.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/leveldb/comparators/indexed_db/indexed_db_key.cpp
  )

  SET(DB_LIBS ${DB_LIBS} leveldb::leveldb)
  IF(SNAPPY_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${SNAPPY_INCLUDE_DIRS})
    SET(DB_LIBS ${DB_LIBS} ${SNAPPY_LIBRARIES})
  ENDIF(SNAPPY_FOUND)
ENDIF(BUILD_WITH_LEVELDB)

#rocksdb
IF(BUILD_WITH_ROCKSDB)
  FIND_PACKAGE(RocksDB REQUIRED)
  SET(HAVE_ROCKSDB ON)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIR})

  #core
  SET(HEADERS_CORE_DB_ROCKSDB
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/rocksdb/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/rocksdb/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/rocksdb/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/rocksdb/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/rocksdb/database_info.h
  )
  SET(SOURCES_CORE_DB_ROCKSDB
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/rocksdb/database_info.cpp
  )

  SET(DB_LIBS ${DB_LIBS} RocksDB::rocksdb)
  IF(SNAPPY_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${SNAPPY_INCLUDE_DIRS})
    SET(DB_LIBS ${DB_LIBS} ${SNAPPY_LIBRARIES})
  ENDIF(SNAPPY_FOUND)
  IF(ZLIB_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
    SET(DB_LIBS ${DB_LIBS} ${ZLIB_LIBRARIES})
  ENDIF(ZLIB_FOUND)
  IF(LZ4_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${LZ4_INCLUDE_DIRS})
    SET(DB_LIBS ${DB_LIBS} ${LZ4_LIBRARIES})
  ENDIF(LZ4_FOUND)
  IF(BZIP2_FOUND)
    SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${BZIP2_INCLUDE_DIRS})
    SET(DB_LIBS ${DB_LIBS} ${BZIP2_LIBRARIES})
  ENDIF(BZIP2_FOUND)
  IF(OS_WINDOWS)
    SET(DB_LIBS ${DB_LIBS} shlwapi Rpcrt4)
  ENDIF(OS_WINDOWS)
ENDIF(BUILD_WITH_ROCKSDB)

#unqlite
IF(BUILD_WITH_UNQLITE)
  FIND_LIBRARY(UNQLITE_LIBRARY unqlite)
  FIND_PATH(UNQLITE_INCLUDE_DIR NAMES unqlite.h PATH_SUFFIXES include)
  SET(HAVE_UNQLITE ON)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${UNQLITE_INCLUDE_DIR})

  #core
  SET(HEADERS_CORE_DB_UNQLITE
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/unqlite/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/unqlite/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/unqlite/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/unqlite/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/unqlite/database_info.h
  )
  SET(SOURCES_CORE_DB_UNQLITE
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/unqlite/database_info.cpp
  )

  SET(DB_LIBS ${DB_LIBS} ${UNQLITE_LIBRARY})
ENDIF(BUILD_WITH_UNQLITE)

#lmdb
IF(BUILD_WITH_LMDB)
  FIND_LIBRARY(LMDB_LIBRARY lmdb)
  FIND_PATH(LMDB_INCLUDE_DIR NAMES lmdb.h PATH_SUFFIXES include)
  SET(HAVE_LMDB ON)
  SET(DB_INCLUDE_DIRS ${DB_INCLUDE_DIRS} ${LMDB_INCLUDE_DIR})

  #core
  SET(HEADERS_CORE_DB_LMDB
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/lmdb/config.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/lmdb/command_translator.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/lmdb/server_info.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/lmdb/db_connection.h
    ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db/lmdb/database_info.h)
  SET(SOURCES_CORE_DB_LMDB
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/internal/commands_api.h
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/internal/commands_api.cpp

    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/command_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/server_info.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/db_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/db/lmdb/database_info.cpp
  )

  SET(DB_LIBS ${DB_LIBS} ${LMDB_LIBRARY})
  IF(OS_WINDOWS)
    SET(DB_LIBS ${DB_LIBS} ntdll)
  ENDIF(OS_WINDOWS)
ENDIF(BUILD_WITH_LMDB)

SET(CORE_HEADERS
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/cdb_connection_client.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/cdb_connection.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/imodule_connection_client.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/command_holder.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/command_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/connection_commands_traits.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/connection_types.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/constant_commands_array.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db_key.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/db_traits.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/global.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/icommand_translator_base.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/icommand_translator.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/logger.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/module_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/server_property_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/ssh_info.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/types.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/value.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/macros.h
  ${CMAKE_SOURCE_DIR}/include/fastonosql/core/basic_types.h

  ${INTERNAL_HEADERS}
  ${CONFIG_HEADERS}
  ${SERVER_HEADERS}
  ${SENTINEL_HEADERS}
  ${CLUSTER_HEADERS}
  ${DATABASE_HEADERS}

  ${HEADERS_CORE_DB_REDIS_COMPATIBLE}
  ${HEADERS_CORE_DB_REDIS}
  ${HEADERS_CORE_DB_PIKA}
  ${HEADERS_CORE_DB_DYNOMITE}
  ${HEADERS_CORE_DB_KEYDB}
  ${HEADERS_CORE_DB_MEMCACHED}
  ${HEADERS_CORE_DB_SSDB}
  ${HEADERS_CORE_DB_LEVELDB}
  ${HEADERS_CORE_DB_ROCKSDB}
  ${HEADERS_CORE_DB_UNQLITE}
  ${HEADERS_CORE_DB_LMDB}
)

SET(CORE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/core/cdb_connection_client.cpp
  ${CMAKE_SOURCE_DIR}/src/core/cdb_connection.cpp
  ${CMAKE_SOURCE_DIR}/src/core/imodule_connection_client.cpp
  ${CMAKE_SOURCE_DIR}/src/core/command_holder.cpp
  ${CMAKE_SOURCE_DIR}/src/core/command_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/connection_commands_traits.cpp
  ${CMAKE_SOURCE_DIR}/src/core/connection_types.cpp
  ${CMAKE_SOURCE_DIR}/src/core/constant_commands_array.cpp
  ${CMAKE_SOURCE_DIR}/src/core/db_key.cpp
  ${CMAKE_SOURCE_DIR}/src/core/db_traits.cpp
  ${CMAKE_SOURCE_DIR}/src/core/global.cpp
  ${CMAKE_SOURCE_DIR}/src/core/icommand_translator_base.cpp
  ${CMAKE_SOURCE_DIR}/src/core/icommand_translator.cpp
  ${CMAKE_SOURCE_DIR}/src/core/logger.cpp
  ${CMAKE_SOURCE_DIR}/src/core/module_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/server_property_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/ssh_info.cpp
  ${CMAKE_SOURCE_DIR}/src/core/types.cpp
  ${CMAKE_SOURCE_DIR}/src/core/value.cpp
  ${CMAKE_SOURCE_DIR}/src/core/basic_types.cpp

  ${INTERNAL_SOURCES}
  ${CONFIG_SOURCES}
  ${SERVER_SOURCES}
  ${SENTINEL_SOURCES}
  ${CLUSTER_SOURCES}
  ${DATABASE_SOURCES}

  ${SOURCES_CORE_DB_REDIS_COMPATIBLE}
  ${SOURCES_CORE_DB_REDIS}
  ${SOURCES_CORE_DB_PIKA}
  ${SOURCES_CORE_DB_DYNOMITE}
  ${SOURCES_CORE_DB_KEYDB}
  ${SOURCES_CORE_DB_MEMCACHED}
  ${SOURCES_CORE_DB_SSDB}
  ${SOURCES_CORE_DB_LEVELDB}
  ${SOURCES_CORE_DB_ROCKSDB}
  ${SOURCES_CORE_DB_UNQLITE}
  ${SOURCES_CORE_DB_LMDB}
)

#dependencies ${ZLIB_LIBRARIES} ${SNAPPY_LIBRARIES} ${LZ4_LIBRARIES} ${BZIP2_LIBRARIES}
IF(ZLIB_FOUND)
  SET(CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
  SET(CORE_LIBS ${CORE_LIBS} ${ZLIB_LIBRARIES})
ENDIF(ZLIB_FOUND)

IF(BZIP2_FOUND)
  SET(CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIRS} ${BZIP2_INCLUDE_DIRS})
  SET(CORE_LIBS ${CORE_LIBS} ${BZIP2_LIBRARIES})
ENDIF(BZIP2_FOUND)

IF(SNAPPY_FOUND)
  SET(CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIRS} ${SNAPPY_INCLUDE_DIRS})
  SET(CORE_LIBS ${CORE_LIBS} ${SNAPPY_LIBRARIES})
ENDIF(SNAPPY_FOUND)

IF(LZ4_FOUND)
  SET(CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIRS} ${LZ4_INCLUDE_DIRS})
  SET(CORE_LIBS ${CORE_LIBS} ${LZ4_LIBRARIES})
ENDIF(LZ4_FOUND)

IF(OS_WINDOWS)
  SET(CORE_PLATFORM_HEADERS)
  SET(CORE_PLATFORM_SOURCES)
  SET(CORE_PLATFORM_LIBRARIES ws2_32)
ELSEIF(OS_MACOSX)
  SET(CORE_PLATFORM_HEADERS)
  SET(CORE_PLATFORM_SOURCES)
  SET(CORE_PLATFORM_LIBRARIES)
ELSEIF(OS_LINUX)
  SET(CORE_PLATFORM_HEADERS)
  SET(CORE_PLATFORM_SOURCES)
  SET(CORE_PLATFORM_LIBRARIES dl)
ELSEIF(OS_FREEBSD)
  SET(CORE_PLATFORM_HEADERS)
  SET(CORE_PLATFORM_SOURCES)
  SET(CORE_PLATFORM_LIBRARIES util)
ELSE()
  MESSAGE(FATAL_ERROR "Not have system_info impl for: ${CMAKE_SYSTEM_NAME}")
ENDIF(OS_WINDOWS)
SET(CORE_PLATFORM_LIBRARIES ${CORE_PLATFORM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# core base
SET(CONFIG_GEN_PATH ${CMAKE_SOURCE_DIR}/include/fastonosql/config.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/config.h.in ${CONFIG_GEN_PATH} @ONLY IMMEDIATE)

FIND_PACKAGE(Common REQUIRED)
FIND_PACKAGE(JSON-C REQUIRED)

# ADD_SUBDIRECTORY(third-party/cpuid)
SET(CORE_PLATFORM_DEFINITIONS ${CORE_PLATFORM_DEFINITIONS} ${DB_DEFINES})
SET(CORE_INCLUDE_DIRS ${CORE_INCLUDE_DIRS} ${COMMON_INCLUDE_DIR} ${JSONC_INCLUDE_DIRS} ${DB_INCLUDE_DIRS} ${CORE_PLATFORM_INCLUDE_DIRS})
SET(CORE_LIBS ${CORE_LIBS} ${COMMON_BASE_LIBRARY} ${JSONC_LIBRARIES} ${CORE_PLATFORM_LIBRARIES} ${DB_LIBS})

SET(CORE_PROJECT_NAME ${PROJECT_NAME_LOWERCASE})
SET(CORE_PRO_PROJECT_NAME ${PROJECT_NAME_LOWERCASE}_pro)

SET(PRIVATE_INCLUDE_DIRECTORIES ${CORE_INCLUDE_DIRS} third-party ${CMAKE_CURRENT_LIST_DIR})
SET(PRIVATE_COMPILE_DEFINITIONS ${CORE_PLATFORM_DEFINITIONS})
SET(PRIVATE_LIBS ${CORE_LIBS})

SET(HEADERS ${CONFIG_GEN_PATH} ${CORE_HEADERS} ${CORE_PLATFORM_HEADERS})
SET(SOURCES ${CORE_SOURCES} ${CORE_PLATFORM_SOURCES} ${DB_OBJECTS_LIBS})

#
ADD_LIBRARY(${CORE_PROJECT_NAME} ${HEADERS} ${SOURCES})
TARGET_INCLUDE_DIRECTORIES(${CORE_PROJECT_NAME}
  PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
)
TARGET_COMPILE_DEFINITIONS(${CORE_PROJECT_NAME} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})
#SET_TARGET_PROPERTIES(${COMMON_PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${COMMON_HEADERS}")
TARGET_LINK_LIBRARIES(${CORE_PROJECT_NAME} PRIVATE ${PRIVATE_LIBS})

# pro
ADD_LIBRARY(${CORE_PRO_PROJECT_NAME} ${HEADERS} ${SOURCES})
TARGET_INCLUDE_DIRECTORIES(${CORE_PRO_PROJECT_NAME}
  PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
)
TARGET_COMPILE_DEFINITIONS(${CORE_PRO_PROJECT_NAME} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS} -DPRO_VERSION)
#SET_TARGET_PROPERTIES(${COMMON_PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${COMMON_HEADERS}")
TARGET_LINK_LIBRARIES(${CORE_PRO_PROJECT_NAME} PRIVATE ${PRIVATE_LIBS})

# bins

SET(TARGET_INSTALL_DESTINATION bin)
SET(LIBRARIES_INSTALL_DESTINATION lib)
SET(HEADERS_INSTALL_DESTINATION include)

INSTALL(TARGETS ${CORE_PROJECT_NAME}
    LIBRARY DESTINATION ${LIBRARIES_INSTALL_DESTINATION}
    ARCHIVE DESTINATION ${LIBRARIES_INSTALL_DESTINATION}
#   PUBLIC_HEADER DESTINATION include/common
)
INSTALL(TARGETS ${CORE_PRO_PROJECT_NAME}
    LIBRARY DESTINATION ${LIBRARIES_INSTALL_DESTINATION}
    ARCHIVE DESTINATION ${LIBRARIES_INSTALL_DESTINATION}
#   PUBLIC_HEADER DESTINATION include/common
)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/fastonosql DESTINATION ${HEADERS_INSTALL_DESTINATION})
#INSTALL(FILES  ${COMMON_HEADERS} DESTINATION ${HEADERS_INSTALL_DESTINATION})

IF(PROJECT_BUILD_TYPE_VERSION STREQUAL "release" AND CMAKE_BUILD_TYPE_STABLE MATCHES "RELEASE")
  IF(NOT OS_WIN)
    STRIP_TARGET(${CORE_PROJECT_NAME})
    STRIP_TARGET(${CORE_PRO_PROJECT_NAME})
  ENDIF(NOT OS_WIN)
ENDIF(PROJECT_BUILD_TYPE_VERSION STREQUAL "release" AND CMAKE_BUILD_TYPE_STABLE MATCHES "RELEASE")

IF (DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES ${CORE_HEADERS} ${CORE_PLATFORM_HEADERS} ${CORE_SOURCES} ${CORE_PLATFORM_SOURCES})
  REGISTER_CHECK_STYLE_TARGET(check_style "${CHECK_SOURCES}")
  REGISTER_CHECK_INCLUDES_TARGET(${CORE_PROJECT_NAME})
  REGISTER_CHECK_INCLUDES_TARGET(${CORE_PRO_PROJECT_NAME})
ENDIF(DEVELOPER_CHECK_STYLE)

IF (DEVELOPER_ENABLE_COVERALLS)
  SET(COVERAGE_SRCS ${CORE_HEADERS} ${CORE_SOURCES})

  # Create the coveralls target.
  COVERALLS_SETUP("${COVERAGE_SRCS}" ${DEVELOPER_COVERALLS_UPLOAD})
ENDIF(DEVELOPER_ENABLE_COVERALLS)

IF(DEVELOPER_ENABLE_TESTS)
  FIND_PACKAGE(GTest REQUIRED)

  SET(UNIT_TEST unit_tests)
  ADD_EXECUTABLE(${UNIT_TEST}
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_configs.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_values.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_fasto_objects.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_readable_string.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_command_holder.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_keys_ranges.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_parse_command.cpp
  )

  TARGET_INCLUDE_DIRECTORIES(${UNIT_TEST}
    PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
  )
  TARGET_COMPILE_DEFINITIONS(${UNIT_TEST} PRIVATE ${CORE_PLATFORM_DEFINITIONS} -DPROJECT_TEST_SOURCES_DIR="${CMAKE_SOURCE_DIR}/tests")
  TARGET_LINK_LIBRARIES(${UNIT_TEST} ${GTEST_BOTH_LIBRARIES} ${CORE_PROJECT_NAME} ${COMMON_BASE_LIBRARY} ${CORE_PLATFORM_LIBRARIES} ${DB_LIBS})
  ADD_TEST_TARGET(${UNIT_TEST})
  SET_PROPERTY(TARGET ${UNIT_TEST} PROPERTY FOLDER "Unit tests")

  #Mock tests
  SET(MOCK_TEST mock_tests)
  ADD_EXECUTABLE(${MOCK_TEST}
    ${CMAKE_SOURCE_DIR}/tests/mock_tests/test_connections.cpp
  )
  TARGET_INCLUDE_DIRECTORIES(${MOCK_TEST}
    PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
  )
  TARGET_COMPILE_DEFINITIONS(${MOCK_TEST} PRIVATE ${CORE_PLATFORM_DEFINITIONS} -DPROJECT_TEST_SOURCES_DIR="${CMAKE_SOURCE_DIR}/tests")
  TARGET_LINK_LIBRARIES(${MOCK_TEST} ${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES} ${CORE_PROJECT_NAME} ${COMMON_BASE_LIBRARY} ${CORE_PLATFORM_LIBRARIES} ${DB_LIBS})
  ADD_TEST_TARGET(${MOCK_TEST})
  SET_PROPERTY(TARGET ${MOCK_TEST} PROPERTY FOLDER "Mock tests")
ENDIF(DEVELOPER_ENABLE_TESTS)
