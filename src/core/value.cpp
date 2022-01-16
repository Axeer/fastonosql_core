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

#include <fastonosql/core/value.h>

#include <json-c/json_tokener.h>

#include <algorithm>

#include <common/convert2string.h>

#include <fastonosql/core/macros.h>
#include <fastonosql/core/types.h>

namespace {
const char* kValueTypes[] = {"TYPE_NULL",       "TYPE_BOOLEAN", "TYPE_INTEGER32", "TYPE_UINTEGER3232", "TYPE_INTEGER64",
                             "TYPE_UINTEGER64", "TYPE_DOUBLE",  "TYPE_TIME",      "TYPE_STRING",       "TYPE_ARRAY",
                             "TYPE_BYTE_ARRAY", "TYPE_SET",     "TYPE_ZSET",      "TYPE_HASH"};
static_assert(arraysize(kValueTypes) == static_cast<size_t>(common::Value::Type::TYPE_HASH) + 1,
              "kValueTypes Has Wrong Size");

struct json_object* json_tokener_parse_hacked(const char* str, int len) {
  struct json_tokener* tok = json_tokener_new();
  if (!tok) {
    return nullptr;
  }

  struct json_object* obj = json_tokener_parse_ex(tok, str, len);
  if (tok->err != json_tokener_success) {
    if (obj) {
      json_object_put(obj);
    }
    obj = nullptr;
  }

  json_tokener_free(tok);
  return obj;
}

}  // namespace

namespace fastonosql {
namespace core {

bool StreamValue::Entry::Equals(const Entry& other) const {
  return name == other.name && value == other.value;
}

bool StreamValue::Stream::Equals(const Stream& other) const {
  return sid == other.sid && entries == other.entries;
}

StreamValue::StreamValue() : Value(TYPE_STREAM), streams_() {}

StreamValue::~StreamValue() {}

StreamValue* StreamValue::DeepCopy() const {
  StreamValue* str = new StreamValue();
  str->SetStreams(streams_);
  return str;
}

bool StreamValue::GetAsString(string_t* out_value) const {
  if (out_value) {
    command_buffer_writer_t wr;
    StreamValue::streams_t streams = GetStreams();
    for (size_t i = 0; i < streams.size(); ++i) {
      StreamValue::Stream cur_str = streams[i];
      if (i != 0) {
        wr << SPACE_CHAR;
      }

      wr << streams[i].sid;
      for (size_t j = 0; j < cur_str.entries.size(); ++j) {
        wr << SPACE_CHAR << cur_str.entries[j].name << SPACE_CHAR << cur_str.entries[j].value;
      }
    }

    *out_value = wr.str();
  }

  return true;
}

bool StreamValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  const StreamValue* other_stream = static_cast<const StreamValue*>(other);
  return GetStreams() == other_stream->GetStreams();
}

StreamValue::streams_t StreamValue::GetStreams() const {
  return streams_;
}

void StreamValue::SetStreams(const streams_t& streams) {
  streams_ = streams;
}

JsonValue::JsonValue(const string_t& json_value) : Value(TYPE_JSON), value_(json_value) {}

JsonValue::~JsonValue() {}

bool JsonValue::GetAsString(string_t* out_value) const {
  if (out_value) {
    *out_value = value_;
  }

  return true;
}

JsonValue* JsonValue::DeepCopy() const {
  return new JsonValue(value_);
}

bool JsonValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  string_t lhs, rhs;
  return GetAsString(&lhs) && other->GetAsString(&rhs) && lhs == rhs;
}

bool JsonValue::IsValidJson(const string_t& json) {
  if (json.empty()) {
    return false;
  }

  json_object* obj = json_tokener_parse_hacked(json.data(), json.size());
  if (!obj) {
    return false;
  }

  json_object_put(obj);
  return true;
}

GraphValue::GraphValue() : Value(TYPE_GRAPH) {}

GraphValue::~GraphValue() {}

GraphValue* GraphValue::DeepCopy() const {
  return new GraphValue;
}

bool GraphValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  return true;
}

BloomValue::BloomValue() : Value(TYPE_BLOOM) {}

BloomValue::~BloomValue() {}

BloomValue* BloomValue::DeepCopy() const {
  return new BloomValue;
}

bool BloomValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  return true;
}

SearchValue* SearchValue::CreateSearchIndex() {
  return new SearchValue(TYPE_FT_INDEX);
}

SearchValue* SearchValue::CreateSearchDocument() {
  return new SearchValue(TYPE_FT_TERM);
}

SearchValue::~SearchValue() {}

SearchValue* SearchValue::DeepCopy() const {
  return new SearchValue(GetType());
}

bool SearchValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  return true;
}

SearchValue::SearchValue(common::Value::Type type) : Value(type) {}

TimeSeriesValue::TimeSeriesValue(const string_t& value) : Value(TYPE_TSDB), value_(value) {}

TimeSeriesValue::~TimeSeriesValue() {}

bool TimeSeriesValue::GetAsString(string_t* out_value) const {
  if (out_value) {
    *out_value = value_;
  }

  return true;
}

TimeSeriesValue* TimeSeriesValue::DeepCopy() const {
  return new TimeSeriesValue(value_);
}

bool TimeSeriesValue::Equals(const Value* other) const {
  if (other->GetType() != GetType()) {
    return false;
  }

  string_t lhs, rhs;
  return GetAsString(&lhs) && other->GetAsString(&rhs) && lhs == rhs;
}

common::Value* CreateEmptyValueFromType(common::Value::Type value_type) {
  const uint8_t cvalue_type = value_type;
  switch (cvalue_type) {
    case common::Value::TYPE_NULL: {
      return common::Value::CreateNullValue();
    }
    case common::Value::TYPE_BOOLEAN: {
      return common::Value::CreateBooleanValue(false);
    }
    case common::Value::TYPE_INTEGER32: {
      return common::Value::CreateInteger32Value(0);
    }
    case common::Value::TYPE_UINTEGER32: {
      return common::Value::CreateUInteger32Value(0);
    }
    case common::Value::TYPE_INTEGER64: {
      return common::Value::CreateInteger64Value(0);
    }
    case common::Value::TYPE_UINTEGER64: {
      return common::Value::CreateUInteger64Value(0);
    }
    case common::Value::TYPE_DOUBLE: {
      return common::Value::CreateDoubleValue(0);
    }
    case common::Value::TYPE_STRING: {
      return common::Value::CreateEmptyStringValue();
    }
    case common::Value::TYPE_ARRAY: {
      return common::Value::CreateArrayValue();
    }
    case common::Value::TYPE_BYTE_ARRAY: {
      return common::Value::CreateByteArrayValue(common::byte_array_t());
    }
    case common::Value::TYPE_SET: {
      return common::Value::CreateSetValue();
    }
    case common::Value::TYPE_ZSET: {
      return common::Value::CreateZSetValue();
    }
    case common::Value::TYPE_HASH: {
      return common::Value::CreateHashValue();
    }
    // extended
    case StreamValue::TYPE_STREAM: {
      return new StreamValue;
    }
    case JsonValue::TYPE_JSON: {
      return new JsonValue(JsonValue::string_t());
    }
    case GraphValue::TYPE_GRAPH: {
      return new GraphValue;
    }
    case BloomValue::TYPE_BLOOM: {
      return new BloomValue;
    }
    case SearchValue::TYPE_FT_INDEX: {
      return SearchValue::CreateSearchIndex();
    }
    case SearchValue::TYPE_FT_TERM: {
      return SearchValue::CreateSearchDocument();
    }
    case TimeSeriesValue::TYPE_TSDB: {
      return new TimeSeriesValue(TimeSeriesValue::string_t());
    }
  }

  NOTREACHED() << "Not handled type: " << value_type;
  return nullptr;
}

const char* GetTypeName(common::Value::Type value_type) {
  if (value_type <= common::Value::TYPE_HASH) {
    return kValueTypes[value_type];
  } else if (value_type == StreamValue::TYPE_STREAM) {
    return "TYPE_STREAM";
  } else if (value_type == JsonValue::TYPE_JSON) {
    return "TYPE_JSON";
  } else if (value_type == GraphValue::TYPE_GRAPH) {
    return "TYPE_GRAPH";
  } else if (value_type == BloomValue::TYPE_BLOOM) {
    return "TYPE_BLOOM";
  } else if (value_type == SearchValue::TYPE_FT_INDEX) {
    return "TYPE_FT_INDEX";
  } else if (value_type == SearchValue::TYPE_FT_TERM) {
    return "TYPE_FT_TERM";
  } else if (value_type == TimeSeriesValue::TYPE_TSDB) {
    return "TYPE_TSDB";
  }

  NOTREACHED() << "Not handled type: " << value_type;
  return "UNKNOWN";
}

convert_to_t ConvertValue(common::Value* value, const std::string& delimiter) {
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  common::Value::Type value_type = value->GetType();
  if (value_type == common::Value::TYPE_NULL) {
    return GEN_CMD_STRING("(nil)");
  } else if (value_type == common::Value::TYPE_BOOLEAN) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_INTEGER32) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_UINTEGER64) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_INTEGER64) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_UINTEGER64) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_DOUBLE) {
    return ConvertValue(static_cast<common::FundamentalValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_STRING) {
    return ConvertValue(static_cast<common::StringValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_ARRAY) {
    return ConvertValue(static_cast<common::ArrayValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_BYTE_ARRAY) {
    return ConvertValue(static_cast<common::ByteArrayValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_SET) {
    return ConvertValue(static_cast<common::SetValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_ZSET) {
    return ConvertValue(static_cast<common::ZSetValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_HASH) {
    return ConvertValue(static_cast<common::HashValue*>(value), delimiter);
  } else if (value_type == StreamValue::TYPE_STREAM) {
    return ConvertValue(static_cast<StreamValue*>(value), delimiter);
    // extended
  } else if (value_type == JsonValue::TYPE_JSON) {
    return ConvertValue(static_cast<JsonValue*>(value), delimiter);
  } else if (value_type == GraphValue::TYPE_GRAPH) {
    return ConvertValue(static_cast<GraphValue*>(value), delimiter);
  } else if (value_type == BloomValue::TYPE_BLOOM) {
    return ConvertValue(static_cast<BloomValue*>(value), delimiter);
  } else if (value_type == SearchValue::TYPE_FT_INDEX) {
    return ConvertValue(static_cast<SearchValue*>(value), delimiter);
  } else if (value_type == SearchValue::TYPE_FT_TERM) {
    return ConvertValue(static_cast<SearchValue*>(value), delimiter);
  } else if (value_type == TimeSeriesValue::TYPE_TSDB) {
    return ConvertValue(static_cast<TimeSeriesValue*>(value), delimiter);
  }

  NOTREACHED() << "Not handled type: " << value_type;
  return convert_to_t();
}

convert_to_t ConvertValue(common::ArrayValue* array, const std::string& delimiter) {
  if (!array || array->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto last_iter = std::prev(array->end());
  for (auto it = array->begin(); it != array->end(); ++it) {
    common::Value* cur_val = *it;
    convert_to_t val_str = ConvertValue(cur_val, delimiter);
    if (val_str.empty()) {
      continue;
    }

    result += val_str;
    if (last_iter != it) {
      result += delimiter;
    }
  }

  return result;
}

convert_to_t ConvertValue(common::SetValue* set, const std::string& delimiter) {
  if (!set || set->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto last_it = std::prev(set->end());
  for (auto it = set->begin(); it != set->end(); ++it) {
    convert_to_t val = ConvertValue((*it), delimiter);
    if (val.empty()) {
      continue;
    }

    result += val;
    if (last_it != it) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValue(common::ZSetValue* zset, const std::string& delimiter) {
  if (!zset || zset->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto last_it = std::prev(zset->end());
  for (auto it = zset->begin(); it != zset->end(); ++it) {
    auto v = *it;
    convert_to_t key = ConvertValue(v.first, delimiter);
    convert_to_t val = ConvertValue(v.second, delimiter);
    if (val.empty() || key.empty()) {
      continue;
    }

    result += key;
    result += SPACE_CHAR;
    result += val;
    if (last_it != it) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValue(common::HashValue* hash, const std::string& delimiter) {
  if (!hash) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t result;
  for (auto it = hash->begin(); it != hash->end(); ++it) {
    auto v = *it;
    convert_to_t key = v.first;
    convert_to_t val = ConvertValue(v.second, delimiter);
    if (val.empty() || key.empty()) {
      continue;
    }

    result += key;
    result += SPACE_CHAR;
    result += val;
    if (std::next(it) != hash->end()) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValue(common::FundamentalValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  const common::Value::Type value_type = value->GetType();
  if (value_type == common::Value::TYPE_BOOLEAN) {
    bool res;
    if (!value->GetAsBoolean(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_INTEGER32) {
    int res;
    if (!value->GetAsInteger32(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_UINTEGER32) {
    unsigned int res;
    if (!value->GetAsUInteger32(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_INTEGER64) {
    int64_t res;
    if (!value->GetAsInteger64(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_UINTEGER64) {
    uint64_t res;
    if (!value->GetAsUInteger64(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_DOUBLE) {
    double res;
    if (!value->GetAsDouble(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  }

  DNOTREACHED();
  return convert_to_t();
}

convert_to_t ConvertValue(common::StringValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  command_buffer_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

convert_to_t ConvertValue(common::ByteArrayValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  common::byte_array_t res;
  if (!value->GetAsByteArray(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return common::ConvertToCharBytes(res);
}

convert_to_t ConvertValue(StreamValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

convert_to_t ConvertValue(JsonValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

convert_to_t ConvertValue(GraphValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValue(BloomValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValue(SearchValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValue(TimeSeriesValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

// for command line

convert_to_t ConvertValueForCommandLine(common::Value* value, const std::string& delimiter) {
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  common::Value::Type value_type = value->GetType();
  if (value_type == common::Value::TYPE_NULL) {
    return GEN_CMD_STRING("(nil)");
  } else if (value_type == common::Value::TYPE_BOOLEAN) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_INTEGER32) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_UINTEGER32) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_INTEGER64) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_UINTEGER64) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_DOUBLE) {
    return ConvertValueForCommandLine(static_cast<common::FundamentalValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_STRING) {
    return ConvertValueForCommandLine(static_cast<common::StringValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_ARRAY) {
    return ConvertValueForCommandLine(static_cast<common::ArrayValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_BYTE_ARRAY) {
    return ConvertValueForCommandLine(static_cast<common::ByteArrayValue*>(value), delimiter);

  } else if (value_type == common::Value::TYPE_SET) {
    return ConvertValueForCommandLine(static_cast<common::SetValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_ZSET) {
    return ConvertValueForCommandLine(static_cast<common::ZSetValue*>(value), delimiter);
  } else if (value_type == common::Value::TYPE_HASH) {
    return ConvertValueForCommandLine(static_cast<common::HashValue*>(value), delimiter);
  } else if (value_type == StreamValue::TYPE_STREAM) {
    return ConvertValueForCommandLine(static_cast<StreamValue*>(value), delimiter);
    // extended
  } else if (value_type == JsonValue::TYPE_JSON) {
    return ConvertValueForCommandLine(static_cast<JsonValue*>(value), delimiter);
  } else if (value_type == GraphValue::TYPE_GRAPH) {
    return ConvertValueForCommandLine(static_cast<GraphValue*>(value), delimiter);
  } else if (value_type == BloomValue::TYPE_BLOOM) {
    return ConvertValueForCommandLine(static_cast<BloomValue*>(value), delimiter);
  } else if (value_type == SearchValue::TYPE_FT_INDEX) {
    return ConvertValueForCommandLine(static_cast<SearchValue*>(value), delimiter);
  } else if (value_type == TimeSeriesValue::TYPE_TSDB) {
    return ConvertValueForCommandLine(static_cast<TimeSeriesValue*>(value), delimiter);
  }

  NOTREACHED() << "Not handled type: " << value_type;
  return convert_to_t();
}

convert_to_t ConvertValueForCommandLine(common::ArrayValue* array, const std::string& delimiter) {
  if (!array || array->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto last_iter = std::prev(array->end());
  for (auto it = array->begin(); it != array->end(); ++it) {
    common::Value* cur_val = *it;
    convert_to_t val_str = ConvertValueForCommandLine(cur_val, delimiter);
    if (val_str.empty()) {
      continue;
    }

    result += val_str;
    if (last_iter != it) {
      result += delimiter;
    }
  }

  return result;
}

convert_to_t ConvertValueForCommandLine(common::SetValue* set, const std::string& delimiter) {
  if (!set || set->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto lastIt = std::prev(set->end());
  for (auto it = set->begin(); it != set->end(); ++it) {
    convert_to_t val = ConvertValueForCommandLine((*it), delimiter);
    if (val.empty()) {
      continue;
    }

    result += val;
    if (lastIt != it) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValueForCommandLine(common::ZSetValue* zset, const std::string& delimiter) {
  if (!zset || zset->IsEmpty()) {
    return convert_to_t();
  }

  convert_to_t result;
  auto lastIt = std::prev(zset->end());
  for (auto it = zset->begin(); it != zset->end(); ++it) {
    auto v = *it;
    convert_to_t key = ConvertValueForCommandLine(v.first, delimiter);
    convert_to_t val = ConvertValueForCommandLine(v.second, delimiter);
    if (val.empty() || key.empty()) {
      continue;
    }

    result += key;
    result += SPACE_CHAR;
    result += val;
    if (lastIt != it) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValueForCommandLine(common::HashValue* hash, const std::string& delimiter) {
  if (!hash) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t result;
  for (auto it = hash->begin(); it != hash->end(); ++it) {
    auto v = *it;
    convert_to_t key = v.first;
    convert_to_t val = ConvertValueForCommandLine(v.second, delimiter);
    if (val.empty() || key.empty()) {
      continue;
    }

    result += key;
    result += SPACE_CHAR;
    result += val;
    if (std::next(it) != hash->end()) {
      result += delimiter;
    }
  }

  DCHECK(!result.empty());
  return result;
}

convert_to_t ConvertValueForCommandLine(common::FundamentalValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  const common::Value::Type value_type = value->GetType();
  if (value_type == common::Value::TYPE_BOOLEAN) {
    bool res;
    if (!value->GetAsBoolean(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_INTEGER32) {
    int res;
    if (!value->GetAsInteger32(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_UINTEGER32) {
    unsigned int res;
    if (!value->GetAsUInteger32(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_INTEGER64) {
    int64_t res;
    if (!value->GetAsInteger64(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_UINTEGER64) {
    uint64_t res;
    if (!value->GetAsUInteger64(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  } else if (value_type == common::Value::TYPE_DOUBLE) {
    double res;
    if (!value->GetAsDouble(&res)) {
      DNOTREACHED();
      return convert_to_t();
    }
    return common::ConvertToCharBytes(res);
  }

  DNOTREACHED();
  return convert_to_t();
}

convert_to_t ConvertValueForCommandLine(common::StringValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  command_buffer_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return ReadableString(res).GetForCommandLine();
}

convert_to_t ConvertValueForCommandLine(common::ByteArrayValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  common::byte_array_t res;
  if (!value->GetAsByteArray(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return common::ConvertToCharBytes(res);
}

convert_to_t ConvertValueForCommandLine(StreamValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

convert_to_t ConvertValueForCommandLine(JsonValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

convert_to_t ConvertValueForCommandLine(GraphValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValueForCommandLine(BloomValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValueForCommandLine(SearchValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  return convert_to_t();
}

convert_to_t ConvertValueForCommandLine(TimeSeriesValue* value, const std::string& delimiter) {
  UNUSED(delimiter);
  if (!value) {
    DNOTREACHED();
    return convert_to_t();
  }

  convert_to_t res;
  if (!value->GetAsString(&res)) {
    DNOTREACHED();
    return convert_to_t();
  }

  return res;
}

}  // namespace core
}  // namespace fastonosql
