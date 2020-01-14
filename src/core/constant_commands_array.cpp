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

#include <fastonosql/core/constant_commands_array.h>

namespace fastonosql {
namespace core {

#ifndef NDEBUG
ConstantCommandsArray::ConstantCommandsArray(std::initializer_list<CommandHolder> l) {
  for (auto it = l.begin(); it != l.end(); ++it) {
    CommandHolder cmd = *it;
    for (auto jt = begin(); jt != end(); ++jt) {
      CommandHolder cmd2 = *jt;
      if (cmd2.IsEqualName(cmd.name)) {
        DNOTREACHED() << "Only unique commands can be in array, but command with name: \"" << cmd.name
                      << "\" already exists!";
      }
    }
    push_back(cmd);
  }
}
#else
ConstantCommandsArray::ConstantCommandsArray(std::initializer_list<CommandHolder> l) : base_class(l) {}
#endif

}  // namespace core
}  // namespace fastonosql
