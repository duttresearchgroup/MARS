/*****************************************************************************
 *                                McPAT/CACTI
 *                      SOFTWARE LICENSE AGREEMENT
 *            Copyright 2012 Hewlett-Packard Development Company, L.P.
 *                          All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.”
 *
 ***************************************************************************/

#ifndef __EASSERT__
#define __EASSERT__

#include <exception>

enum {
	EASSERT_CACTI_DEFAULT,
	EASSERT_CACTI_WIRE_LTZERO_ERROR,
	EASSERT_MCPAT_DEFAULT,
	EASSERT_MCPAT_LINKLEN_ERROR,
};

namespace CACTI_EASSERT {

class easert_exception: public std::exception
{
	virtual const char* what() const throw()  {
		switch(_reason){
		case EASSERT_CACTI_DEFAULT:				return "{CACTI default exception}";
		case EASSERT_CACTI_WIRE_LTZERO_ERROR:	return "{CACTI wire less than zero error}";
		case EASSERT_MCPAT_DEFAULT:				return "{McPAT default exception}";
		case EASSERT_MCPAT_LINKLEN_ERROR:		return "{McPAT link len error}";
		default:								return "{unknown exception}";
		}

	}
public:
	easert_exception(int reason): std::exception(), _reason(reason){}
private:
	int _reason;
};

};

namespace CACTI {
void inline cactiassert(bool exp, int id=EASSERT_CACTI_DEFAULT){
	if(!exp) throw CACTI_EASSERT::easert_exception(id);
}
};

namespace McPAT {
void inline mcpatassert(bool exp, int id=EASSERT_MCPAT_DEFAULT){
	if(!exp) throw CACTI_EASSERT::easert_exception(id);
}
};

#endif
