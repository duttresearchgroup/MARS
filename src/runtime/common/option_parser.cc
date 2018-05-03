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

#include "option_parser.h"

OptionParser* OptionParser::_optionParser = nullptr;

const std::string OptionsTraits<OPT_OUTDIR>::str = "outdir";
const std::string OptionsTraits<OPT_OUTDIR>::desc = "Path to the directory that stores output files";

const std::string OptionsTraits<OPT_MODELPATH>::str = "model_path";
const std::string OptionsTraits<OPT_MODELPATH>::desc = "ath to the directory containing models";
