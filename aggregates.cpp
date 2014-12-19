/*
 *    _____      _ ____  ____
 *   / ___/_____(_) __ \/ __ )
 *   \__ \/ ___/ / / / / __  |
 *  ___/ / /__/ / /_/ / /_/ / 
 * /____/\___/_/_____/_____/  
 *
 *
 * BEGIN_COPYRIGHT
 *
 * This file is part of SciDB.
 * Copyright (C) 2008-2014 SciDB, Inc.
 *
 * SciDB is free software: you can redistribute it and/or modify
 * it under the terms of the AFFERO GNU General Public License as published by
 * the Free Software Foundation.
 *
 * SciDB is distributed "AS-IS" AND WITHOUT ANY WARRANTY OF ANY KIND,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,
 * NON-INFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE. See
 * the AFFERO GNU General Public License for the complete license terms.
 *
 * You should have received a copy of the AFFERO GNU General Public License
 * along with SciDB.  If not, see <http://www.gnu.org/licenses/agpl-3.0.html>
 *
 * END_COPYRIGHT
 */
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "query/Operator.h"
#include "system/Exceptions.h"
#include "query/LogicalExpression.h"

#include <math.h>
#include "quote.h"

using namespace std;
using namespace scidb;

namespace quote_aggregates
{

class quote_reducer : public Aggregate
{
private:

public:
    quote_reducer(const string& name, Type const& aggregateType):
        Aggregate(name, aggregateType, aggregateType)
    {
    }

    virtual AggregatePtr clone() const
    {
        return AggregatePtr(new quote_reducer(getName(), getAggregateType()));
    }

    AggregatePtr clone(Type const& aggregateType) const
    {
        return AggregatePtr(new quote_reducer(getName(), aggregateType));
    }

    bool ignoreNulls() const
    {
        return true;
    }

    Type getStateType() const
    {
        return getAggregateType();
    }

    void initializeState(Value& state)
    {
      opraquote o;
      o.bid = -1;
      o.bidsz = 0;
      o.ask = INFINITY;
      o.asksz = 0;
      o.bidexch = 0;
      o.askexch = 0;
      o.time = 0;
      state.setData(&o, sizeof(opraquote));
    }

    void accumulate(Value& state, Value const& input)
    {
      if(input.isNull()) return;
// Find the best quote
      opraquote *best = (opraquote *)state.data();
      opraquote *o = (opraquote *)input.data();
      bool replace = false;
      if(o->bidexch == best->bidexch)
      {
        if(best->time < o->time) replace = true;
      } else if(o->bid > best->bid) replace = true;
      if(replace)
      { 
        best->bid = o->bid;
        best->bidsz = o->bidsz;
        best->bidexch = o->bidexch;
      }
      replace = false;
      if(o->askexch == best->askexch)
      {
        if(best->time < o->time) replace = true;
      } else if(o->ask < best->ask) replace = true;
      if(replace)
      { 
        best->ask = o->ask;
        best->asksz = o->asksz;
        best->askexch = o->askexch;
      }
      if(o->time > best-> time) best->time = o->time;
      state.setData(best, sizeof(opraquote));
    }

    void merge(Value& dstState, Value const& srcState)
    {
      accumulate(dstState, srcState);
    }

    void finalResult(Value& result, Value const& state)
    {
      result.setData((opraquote *)state.data(), sizeof(opraquote));
    }
};


vector<AggregatePtr> _aggregates;
EXPORTED_FUNCTION const vector<AggregatePtr>& GetAggregates()
{
    return _aggregates;
}

class quote_accumulatorGeneratorInstance
{
public:
    quote_accumulatorGeneratorInstance()
    {
        // register the aggregate
        _aggregates.push_back(AggregatePtr(new quote_reducer("quote_best", TypeLibrary::getType(TID_VOID))));
    }
} _aggregateGeneratorInstance;

}
