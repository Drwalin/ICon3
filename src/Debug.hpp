/*
 *  Debugging methods and macros.
 *  Copyright (C) 2020 Marek Zalewski aka Drwalin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <cstdio>

#define DEBUG(x) {fprintf(stderr,"\n %s : %s:%i",x,__FILE__,__LINE__); fflush(stderr);}

#endif

