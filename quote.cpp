/*
 * @file quote.cpp
 *
 * @authors B. W. Lewis <blewis@paradigm4.com>,
 * @brief An example user-defined type called 'quote' that represents
 * one or more opra quotes.
 */
#include <boost/assign.hpp>     // Required by the list_of macro used to register functions
// Minimum SciDB headers required for user-defined types
#include "query/Operator.h"
#include "query/FunctionLibrary.h"
#include "query/FunctionDescription.h"
#include "query/TypeSystem.h"
// Other include
#include  "quote.h"

using namespace std;
using namespace scidb;
using boost::assign::list_of;

// My TypeId
static const char TID_QUOTE[] = "quote";

static void new_quote(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  opraquote o;
  o.bid = (float)args[0]->getFloat();
  o.bidsz = args[1]->getInt64();
  o.ask = (float)args[2]->getFloat();
  o.asksz = args[3]->getInt64();
  o.bidexch = args[4]->getChar();
  o.askexch = args[4]->getChar();

  quotes q;
  q.list.push_back(o);
  q.time = args[5]->getInt64();

  serialize_to_scidb_valuep(q, res);
}

// an empty constructor is required of all types
static void nothing(const Value** args, Value *res, void*)
{
}

// a "to string" conversion is required of all types for printing
static void quote_to_string(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  if(q.list.size()>0)
  {
    char ans[4096];
    memset(ans,0,4096);
    char *p = ans;
    size_t l;
    opraquote o;
    for(unsigned int j=0;j < q.list.size(); ++j)
    {
      o = q.list.at(j);
      l = 4096 - (p-ans);
      if(l>0)
      {
        p = p + snprintf(p, l, "%.2f, %ld, %c, %.2f, %ld, %c,",
                       o.bid, o.bidsz, o.bidexch, o.ask, o.asksz, o.askexch);
      }
    }
    res->setString(ans);
  } else
  {
    res->setString("");
  }
}

// Convenience functions to pick out parts of the struct
static void quote_bid(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setFloat(o.bid);
}
static void quote_bidsz(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setInt64(o.bidsz);
}
static void quote_bidexch(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setChar(o.bidexch);
}
static void quote_ask(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setFloat(o.ask);
}
static void quote_asksz(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setInt64(o.asksz);
}
static void quote_askexch(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  opraquote o = q.list.at(0);
  res->setChar(o.askexch);
}
static void quote_time(const Value** args, Value* res, void*)
{
  if(args[0]->isNull())
  {
    res->setNull();
    return;
  }
  quotes q = deserialize(args[0]->data());
  res->setInt64(q.time);
}

// Register the type, functions and implicit converter functions 
REGISTER_TYPE(quote, 0);
// Type system trivia...we require a function with no arguments.
REGISTER_FUNCTION(quote, ArgTypes(), TID_QUOTE, nothing);
REGISTER_FUNCTION(quote, list_of("float")("int64")("float")("int64")("char")("int64"), TID_QUOTE, new_quote);
REGISTER_FUNCTION(quote_bid, list_of(TID_QUOTE), TID_FLOAT, quote_bid);
REGISTER_FUNCTION(quote_bidsz, list_of(TID_QUOTE), TID_INT64, quote_bidsz);
REGISTER_FUNCTION(quote_bidexch, list_of(TID_QUOTE), TID_CHAR, quote_bidexch);
REGISTER_FUNCTION(quote_ask, list_of(TID_QUOTE), TID_FLOAT, quote_ask);
REGISTER_FUNCTION(quote_asksz, list_of(TID_QUOTE), TID_INT64, quote_asksz);
REGISTER_FUNCTION(quote_askexch, list_of(TID_QUOTE), TID_CHAR, quote_askexch);
REGISTER_FUNCTION(quote_time, list_of(TID_QUOTE), TID_INT64, quote_time);
REGISTER_CONVERTER(quote, string, EXPLICIT_CONVERSION_COST, quote_to_string);
