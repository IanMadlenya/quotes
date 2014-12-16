#include "query/Operator.h"
#include "query/FunctionLibrary.h"
#include "query/FunctionDescription.h"
#include "query/TypeSystem.h"

#include  "quote.h"

using namespace std;
using namespace scidb;

// Serialize a quotes struct into a binary blob with structure
// size_t N (#quotes), size_t time, opraquote_1, opraquote_2, ..., opraquote_N
void serialize_to_scidb_valuep(quotes q, Value *v)
{
  size_t len = q.list.size();
  size_t N = sizeof(size_t) + sizeof(int64_t) + 256*sizeof(int) + len*sizeof(opraquote);
  v->setSize(N);
  char *p = (char *)v->data();
  memcpy((void *)p, &len, sizeof(size_t));
  p = p + sizeof(size_t);
  memcpy((void *)p, &q.time, sizeof(int64_t));
  p = p + sizeof(int64_t);
  memcpy((void *)p, &q.map, 256*sizeof(int));
  p = p + 256*sizeof(int);
  if(len>0) memcpy(p, q.list.data(), len*sizeof(opraquote));
}

void serialize_to_scidb_value(quotes q, Value &v)
{
  size_t len = q.list.size();
  size_t N = sizeof(size_t) + sizeof(int64_t) + 256*sizeof(int) + len*sizeof(opraquote);
  v.setSize(N);
  char *p = (char *)v.data();
  memcpy((void *)p, &len, sizeof(size_t));
  p = p + sizeof(size_t);
  memcpy((void *)p, &q.time, sizeof(int64_t));
  p = p + sizeof(int64_t);
  memcpy((void *)p, &q.map, 256*sizeof(int));
  p = p + 256*sizeof(int);
  if(len>0) memcpy(p, q.list.data(), len*sizeof(opraquote));
}

quotes deserialize(void *v)
{
  quotes q;
  char *p = (char *)v;
  size_t N;
  memcpy(&N, p, sizeof(size_t));
  p = p + sizeof(size_t);
  memcpy(&q.time, p, sizeof(int64_t));
  p = p + sizeof(int64_t);
  memcpy(q.map, p, 256*sizeof(int));
  p = p + 256*sizeof(int);
  if(N>0)
  {
    q.list.resize(N);
    memcpy(q.list.data(), p, N*sizeof(opraquote));
  }
  return q;
}
