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
      quotes q;
      opraquote o;
      o.bid = -1;
      o.bidsz = 0;
      o.ask = INFINITY;
      o.asksz = 0;
      o.bidexch = 0;
      o.askexch = 0;
      q.list.push_back(o);
      q.time = 0;
      for(int j=0;j<256;++j) q.map[j]=-1;
      serialize_to_scidb_value(q, state);
    }

// The reducer will produce a state value with one quote in it, the best.  It
// might have different bid and ask exchanges (unlike the input quotes). That
// is, we compute the national best bid and offer among the input quotes.
// This is designed to be used with cumulate.
    void accumulate(Value& state, Value const& input)
    {
      if(input.isNull()) return;
      quotes state_q = deserialize(state.data());
      quotes input_q = deserialize(input.data());
// Find the best quote
fprintf(stderr,"input.ms=%ld state ms=%ld, state.size=%ld input.size=%ld\n", input_q.time, state_q.time, state_q.list.size(), input_q.list.size());
      opraquote best = state_q.list.at(0);
      opraquote o;
      bool replace;
      for(unsigned int j=0;j < input_q.list.size(); ++j)
      { 
        replace = false;
        o = input_q.list.at(j);
        if(o.bidexch == best.bidexch && state_q.time > input_q.time) continue;
        else if(o.bidexch == best.bidexch) replace = true;
        if(o.bid > best.bid || replace)
        { 
          best.bid = o.bid;
          best.bidsz = o.bidsz;
          best.bidexch = o.bidexch;
        }
fprintf(stderr,"j=%d o.ask=%f best.ask=%f\n",j,o.ask,best.ask);
        if(o.ask < best.ask || replace)
        { 
          best.ask = o.ask;
          best.asksz = o.asksz;
          best.askexch = o.askexch;
        }
      }
fprintf(stderr,"OK best ask=%f\n",best.ask);
      state_q.list.at(0) = best;
      if(input_q.time > state_q.time) state_q.time = input_q.time;
      serialize_to_scidb_value(state_q, state);
    }

    void merge(Value& dstState, Value const& srcState)
    {
      accumulate(dstState, srcState);
    }

    void finalResult(Value& result, Value const& state)
    {
      quotes state_q = deserialize(state.data());
      serialize_to_scidb_value(state_q, result);
    }
};



class quote_accumulator : public Aggregate
{
private:

public:
    quote_accumulator(const string& name, Type const& aggregateType):
        Aggregate(name, aggregateType, aggregateType)
    {
    }

    virtual AggregatePtr clone() const
    {
        return AggregatePtr(new quote_accumulator(getName(), getAggregateType()));
    }

    AggregatePtr clone(Type const& aggregateType) const
    {
        return AggregatePtr(new quote_accumulator(getName(), aggregateType));
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
      quotes q;
      q.time = 0;
      for(int j=0;j<256;++j) q.map[j]=-1;
      serialize_to_scidb_value(q, state);
    }

// Add the input quotes to the state, over-writing existing quotes
// on the same bid exchange with newer values. Note that this aggregate
// assumes that the bid and ask exchanges are the same (from a bbo quote).
    void accumulate(Value& state, Value const& input)
    {
      if(input.isNull()) return;
      quotes state_q = deserialize(state.data());
      quotes input_q = deserialize(input.data());
      opraquote o;
      for(unsigned int i=0;i<input_q.list.size();++i)
      {
        o = input_q.list.at(i);
        int k = (int)o.bidexch;
        int j = state_q.map[k];                          // Check for collision
        if(j > -1)
        {
          if(input_q.time > state_q.time) state_q.list.at(j) = o; // take newest val
        } else                                        // No collision, just add
        {
          state_q.list.push_back(o);
          state_q.map[k] = state_q.list.size()-1;
        }
      }
      if(input_q.time > state_q.time) state_q.time = input_q.time;
      serialize_to_scidb_value(state_q, state);
    }

    void merge(Value& dstState, Value const& srcState)
    {
      accumulate(dstState, srcState);
    }

    void finalResult(Value& result, Value const& state)
    {
      quotes state_q = deserialize(state.data());
      serialize_to_scidb_value(state_q, result);
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
        _aggregates.push_back(AggregatePtr(new quote_accumulator("quote_accumulate", TypeLibrary::getType(TID_VOID))));
        _aggregates.push_back(AggregatePtr(new quote_reducer("quote_best", TypeLibrary::getType(TID_VOID))));
    }
} _aggregateGeneratorInstance;

}
