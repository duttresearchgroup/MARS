/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

/*
 * Some helper functions to work with strings
 */

#include <string>
#include <sstream>
#include <vector>

/*
 * Converts a string to another value (e.g. double) of type T
 * If type T is also std::string then a ref or copy is returned
 */
template<typename T>
inline T fromstr(const std::string &str){
    std::stringstream s(str);
    T val;
    s >> val;
    return val;
}
template<>
inline const std::string& fromstr<const std::string&>(const std::string &str)
{ return str; }
template<>
inline std::string fromstr<std::string>(const std::string &str)
{ return str; }
template<>
inline const char* fromstr<const char*>(const std::string &str)
{ return str.data(); }

/*
 * Splits string into tokens and converts each token to values of
 * type T.
 *
 * delim defines the separator between tokens
 */
template<typename T, typename DelimT>
inline std::vector<T> splitstr(const std::string& text, DelimT delims)
{
    std::vector<T> tokens;
    std::size_t start = text.find_first_not_of(delims), end = 0;

    while((end = text.find_first_of(delims, start)) != std::string::npos)
    {
        tokens.push_back(fromstr<T>(text.substr(start, end - start)));
        start = text.find_first_not_of(delims, end);
    }
    if(start != std::string::npos)
        tokens.push_back(fromstr<T>(text.substr(start)));

    return tokens;
}

/*
 * Returns a formated C string with up-to 64 chars
 * All formated string are allocated in the same buffer
 * so use caution with these.
 */
extern thread_local char __formatstr_buff[64];
template<typename... Args>
inline const char * formatstr(const char *s, Args... args){
    std::snprintf(__formatstr_buff,64,s,args...);
    return __formatstr_buff;
}

