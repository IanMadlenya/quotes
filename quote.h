struct opraquote
{
  int64_t time;  // timestamp
  double bid;
  double ask;
  int64_t bidsz;
  int64_t asksz;
  char bidexch;  // bid-specific exchange
  char p1[7];    // padding
  char askexch;  // ask-specific exchange
  char p2[7];    // padding
};
