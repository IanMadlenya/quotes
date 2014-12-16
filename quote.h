struct opraquote
{
  double bid;
  double ask;
  int64_t bidsz;
  int64_t asksz;
  char bidexch;  // bid-specific exchange
  char p1[7];    // padding
  char askexch;  // ask-specific exchange
  char p2[7];    // padding
};

struct quotes
{
  int64_t time;  // timestamp for all quotes stored here
  int map[256];  // quick quote lookup in vector by exchannge
  vector<opraquote> list;
};

void serialize_to_scidb_valuep(quotes q, scidb::Value *v);
void serialize_to_scidb_value(quotes q, scidb::Value &v);
quotes deserialize(void *v);
