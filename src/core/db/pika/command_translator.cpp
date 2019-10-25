/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <fastonosql/core/db/pika/command_translator.h>

#include <fastonosql/core/connection_types.h>

namespace fastonosql {
namespace core {
namespace pika {

CommandTranslator::CommandTranslator(const std::vector<CommandHolder>& commands) : base_class(commands) {}

const char* CommandTranslator::GetDBName() const {
  typedef ConnectionTraits<PIKA> connection_traits_class;
  return connection_traits_class::GetDBName();
}

}  // namespace pika
}  // namespace core
}  // namespace fastonosql