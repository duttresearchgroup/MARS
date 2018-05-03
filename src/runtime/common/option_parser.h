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

#ifndef __arm_rt_common_option_parser_h
#define __arm_rt_common_option_parser_h

#include <unordered_map>
#include <string>
#include <vector>

#include <base/base.h>

#include "gen_store.h"
#include "strings.h"

// Parses command line options.
// Options must follow the format:
//    opt=val if val is a single value
//    opt=val,val.. if val is a list of values (multiple values separated by ",")
// Options are separated by one or more blank spaces.
// Values with blank spaces must be enclosed in "", e.g.:
//    opt="hello world"
// Note that options are parsed on demand when the get/getVector functions are
// called. If the option was not provided, an exception is triggered. The caller
// must ensure the option was provided (using the isSet function) before calling
// get or getVector. Any option may be parsed as any type. If an option value
// cannot be coverted to the requested type, the behavior is undefined.
//
// TODO add a function that returns true if the option can be converted to
// a given type


class OptionParser {

    static OptionParser* _optionParser;

    std::string _progName;

    std::unordered_map<std::string,std::string> _options;

    GenericStoreMap<std::string,
        std::string, int, double, std::vector<std::string>>
    _parsedOptions;

    OptionParser(int argc, char * argv[])
    {
        assert_true(argc > 0);

        // 1st is the prog name
        _progName = argv[0];

        for(int i = 1; i < argc; ++i){
            std::vector<std::string> v = splitstr<std::string>(argv[i],'=');
            if(v.size() != 2)
                arm_throw(OptionParserException,"Invalid options format");
            _options[v[0]] = v[1];
        }
    }

    OptionParser(std::unordered_map<std::string,std::string> opts)
    {
        _options = opts;
    }

  public:

    template<typename T>
    T& parse(const std::string &opt)
    {
        auto& parsedMap = _parsedOptions.access<T>();
        auto parsedIter = parsedMap.find(opt);
        if(parsedIter != parsedMap.end()){
            return parsedIter->second;
        }
        else{
            // parses
            auto iter = _options.find(opt);
            if(iter == _options.end())
                arm_throw(OptionParserException,"Option %s not provided",opt.c_str());

            parsedMap[opt] = fromstr<T>(iter->second);

            return parsedMap[opt];
        }
    }

    template<typename T>
    std::vector<T>& parseVector(const std::string &opt)
    {
        auto& parsedMap = _parsedOptions.access<std::vector<T>>();
        auto parsedIter = parsedMap.find(opt);
        if(parsedIter != parsedMap.end()){
            return parsedIter->second;
        }
        else{
            // parses
            auto iter = _options.find(opt);
            if(iter == _options.end())
                arm_throw(OptionParserException,"Option %s not provided",opt.c_str());

            parsedMap[opt] = splitstr<T>(iter->second,',');

            return parsedMap[opt];
        }
    }

    bool optSet(const std::string &opt)
    {
        auto iter = _options.find(opt);
        return iter != _options.end();
    }

    void printOpts()
    {
        for(auto& i : _options){
            pinfo("\t%s = %s\n",i.first.c_str(),i.second.c_str());
        }
    }

    std::string& progName()
    {
        return _progName;
    }

    static void init(int argc, char * argv[])
    {
        assert_true(_optionParser == nullptr);
        _optionParser = new OptionParser(argc, argv);
    }

    static void init(std::unordered_map<std::string,std::string> opts)
    {
        assert_true(_optionParser == nullptr);
        _optionParser = new OptionParser(opts);
    }

    static void dealloc()
    {
        assert_true(_optionParser != nullptr);
        delete _optionParser;
        _optionParser = nullptr;
    }

    template<typename T>
    static T& get(const std::string &opt)
    {
        assert_true(_optionParser != nullptr);
        return _optionParser->parse<T>(opt);
    }

    template<typename T>
    static std::vector<T>& getVector(const std::string &opt)
    {
        assert_true(_optionParser != nullptr);
        return _optionParser->parseVector<T>(opt);
    }

    static bool isSet(const std::string &opt)
    {
        assert_true(_optionParser != nullptr);
        return _optionParser->optSet(opt);
    }

    static OptionParser& parser()
    {
        assert_true(_optionParser != nullptr);
        return *_optionParser;
    }

};


// Helper class for getting predefined options

enum PredefinedOption {
    OPT_OUTDIR = 0,
    OPT_MODELPATH,
    ///////////
    SIZE_OPT
};

template<PredefinedOption OPT>
struct OptionsTraits;

template<>
struct OptionsTraits<OPT_OUTDIR> {
    using Type = std::string;
    static const std::string str;
    static const std::string desc;
};
template<>
struct OptionsTraits<OPT_MODELPATH> {
    using Type = std::string;
    static const std::string str;
    static const std::string desc;
};

struct Options {

    template<PredefinedOption OPT>
    static typename OptionsTraits<OPT>::Type& get()
    {
        return OptionParser::get<typename OptionsTraits<OPT>::Type>(OptionsTraits<OPT>::str);
    }

    template<PredefinedOption OPT>
    static typename OptionsTraits<OPT>::Type& getVector()
    {
        return OptionParser::getVector<typename OptionsTraits<OPT>::Type>(OptionsTraits<OPT>::str);
    }

    template<PredefinedOption OPT>
    static bool isSet()
    {
        return OptionParser::isSet(OptionsTraits<OPT>::str);
    }

};


#endif

