/*
**
* BEGIN_COPYRIGHT
*
* This file is part of SciDB.  Copyright (C) 2008-2014 SciDB, Inc.
*
* Superfunpack is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License version 2 as published by the
* Free Software Foundation.
*
* SciDB is distributed "AS-IS" AND WITHOUT ANY WARRANTY OF ANY KIND, INCLUDING
* ANY IMPLIED WARRANTY OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU General Public License version 2 for the
* complete license terms.
*
* END_COPYRIGHT
*/
#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <boost/assign.hpp>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/tools/roots.hpp>

#include "query/FunctionLibrary.h"
#include "query/FunctionDescription.h"
#include "system/ErrorsLibrary.h"


using namespace std;
using namespace scidb;
using namespace boost::assign;

/* 
 * @brief  Parse the data string into a ms time value.
 * @param data (string) input data, assumed to be in the form HH:MM:SS.MLS
 * @returns int64
 * XXX use strtol here instead of atoi...
 */
static void
tm2ms(const Value** args, Value *res, void*)
{
  int j, k;
  char *data, *p1, *p2, *token, *subtoken, *str2;
  int64_t ms = 0;
  for (j = 1, data = (char *) args[0]->getString(); ; j++, data=NULL)
  {
    token = strtok_r(data, ":", &p1);
    if (token == NULL) break;
    switch(j)
    {
      case 1: ms = ms + 3600000 * ((int64_t) atoi(token)); break;
      case 2: ms = ms + 60000 * ((int64_t) atoi(token)); break;
      case 3:
        for (k=1, str2=token; ; k++, str2=NULL)
        {
          subtoken = strtok_r(str2, ".", &p2);
          if (subtoken == NULL) break;
          if(k==1) ms = ms + 1000*((int64_t) atoi(subtoken));
          else if(k==2)
          {
            ms = ms + (int64_t) atoi(subtoken);
            break;
          }
        }
      default:
        break;
    }
  }
  res->setInt64(ms);
}

REGISTER_FUNCTION(tm2ms, list_of("string"), "int64", tm2ms);
