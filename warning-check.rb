#!/usr/bin/env ruby
# Copyright (C) 2011 Harald Sitter <sitter@kde.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library.  If not, see <http://www.gnu.org/licenses/>.

for file in Dir.glob("src/**")
    line_count = 1
    has_open_ifdef = false

    next if File.directory?(file)

    File.open(file, 'r').each_line do | line |
        if line.start_with?("#ifdef __GNUC")
            has_open_ifdef = true
        end

        if line.start_with?("#endif")
            has_open_ifdef = false
        end

        if line.start_with?("#warning") and not has_open_ifdef
            puts ("unprotected warning in (#{file}:#{line_count}), please add #ifdef __GNUC__")
            return -1
        end

        line_count += 1
    end
end

puts ("All good :)")
