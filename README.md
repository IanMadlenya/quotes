quotes
======

An OPRA options nbbo/book consolidation example. The example computes national
best bid and ask prices (NBBO) across quotes on one or more exchanges. Each
exchange quote is assumed to represent that exchange's best bid and ask price.

The example program is a SciDB plugin that includes a 'quote' user-defined type,
several supporting functions for working with the quote type and supporting
aggregate function 'quote_best' to compute the best bid and offer between
two quotes.

The example scripts illustrate loading a tiny fake OPRA data example designed
to illustrate the computation. The scripts require Pardigm4's enterprise
extensions to SciDB as well as several add-on SciDB plugins available on
GitHub. The easiest way to install the plugins is to follow the instructions
here:

https://github.com/paradigm4/dev_tools

and then run the following SciDB queries:
```
iquery -aq "load_library('dev_tools')"
iquery -aq "install_github('paradigm4/superfunpack')"
iquery -aq "install_github('paradigm4/chunk_unique')"
iquery -aq "install_github('paradigm4/load_tools')"
iquery -aq "install_github('paradigm4/quotes')"
```

## Contrived example

The repository includes a fake OPRA data file called `opraqte_test.csv.bz2`
that was constructed to illustrate key ideas in the computation. The file
contains 11 data rows of two option IDs for one ticker symbol as follows:
```
      date  tkr optid           tm bid bidsz ask asksz exch flag feed seqnum
2014-10-02 TEST     1 00:00:01.000   3    10   5    50    A   67   2c    100
2014-10-02 TEST     1 00:00:02.000   2    20   4    10    A   67   2c    200
2014-10-02 TEST     1 00:00:03.000   1    10   5    20    A   67   2c    500
2014-10-02 TEST     1 00:00:03.000   2    20   6    20    B   67   2c    600
2014-10-02 TEST     1 00:00:03.000   0    10   7    30    C   67   2c    700
2014-10-02 TEST     1 00:00:03.000   3    30   7    40    D   67   2c    800
2014-10-02 TEST     1 00:00:03.000   4    40   7    50    E   67   2c    900
2014-10-02 TEST     1 00:00:04.000   5    10   6    10    B   67   2c   1000
2014-10-02 TEST     1 00:00:05.000   1    20   6    11    A   67   2c   1100
2014-10-02 TEST     2 00:00:01.000   1    10   6    10    A   68   2c    300
2014-10-02 TEST     2 00:00:01.000   1    20   4    10    A   67   2c    400
```
The NBBO computation is outlined:

1. At each point in time where data are present for any exchange, look up for
*all* exchanges the most recently available quotes.
2. Record among all exchanges the best bid price and its exchange and the
best ask price and its exchange. If more than one exchange exhibits matching
best bid or ask price, pick one (this behavior can be changed to report all
matching exchanges).

The first 9 rows of the fake data illustrate normal NBBO computation and will
produce 5 rows of output, one for each reported time period (the many available
values at time=00:00:03.00 will be coalesced by the program into a best value).

The last 2 rows illustrate a special case of a time collision on the same
exchange (with different quote values, something that should not technically
be possible), and only serve to illustrate handling of this special case.

The expected output for these input data should look like:
```
      date  tkr optid           tm bid bidsz ask asksz flag feed seqnum bidexch askexch
2014-10-02 TEST     1 00:00:01.000   3    10   5    50   67   2c    100       A       A
2014-10-02 TEST     1 00:00:02.000   2    20   4    10   67   2c    200       A       A
2014-10-02 TEST     1 00:00:03.000   4    40   5    20   67   2c    900       E       A
2014-10-02 TEST     1 00:00:04.000   5    10   5    20   67   2c   1000       B       A
2014-10-02 TEST     1 00:00:05.000   5    10   6    11   67   2c   1100       B       A
2014-10-02 TEST     2 00:00:01.000   1    20   4    10   68   2c    300       A       A
```

Note that at time=00:00:05.000 exchanges B and A have equivalent best ask
prices. The example program lists only one best price now on output, chosen in
lexicographical exchange ID order. This behavior could be modified to return
all exchanges matching the best price.

Note that the order sizes are not aggregated and simply represent the order size of
the best listed quote at that point.


# Using the ScIDB plugin with the example data

## Description of the 'quote' user-defined type and functions
The plugin defines the 'quote' type that consists of the fields:

* bid (float)
* bidsz (int64)
* bidexch (char)
* ask (float)
* asksz (int64)
* askexch (char)
* ms time stamp (int64)

The `quote_best` aggregation function takes two quotes and returns a new quote
with the best bid and corresponding bidsz and bidexch, and the best ask and its
corresponding asksz and askexch, unless the two quotes have the same ms time
stamp in which case the newest timestamp takes precedence, and ignoring null
quotes.

## Load the example data into SciDB

Run the supplied `load-opraqte` script with the example data file:

```
./load-opraqte ./opraqte_test.csv.bz2
```
If you don't have the `pbzip2` program installed, either install it or modify
the script to use bunzip2.

The load script produces a ScIDB array called `opraqte` with schema:
```
<quote:quote NULL DEFAULT null,
 flag:string NULL DEFAULT null,
 feed:string NULL DEFAULT null,
 seqnum:int64 NULL DEFAULT null>
[synth=0:999,1000,0,
 exch=0:255,256,0,
 instrument_id=0:*,2000,0,
 day=0:*,1,0,
 ms=0:86399999,600000,0]
```
The array has five coordinate axes along exchange, instrument_id, day, ms, and
a SciDB synthetic dimension called synth to handle data collisions (rows 10
and 11 in the example data).

The instrument_id axis is an integer that enumerates unique combinations
of tkr and optid values. There are about 850,000 such ids among exchange
traded US options. That enumeration is maintained in the auxiliary SciDB
array called `opra_instruments`.

Note that the data seqnum field can instead be used as a coordinate axis to
separate data collisions on the other axes. However, seqnum is unique for
every data element. Since there aren't many collisions (there should not be
any in practice, but there seem to be somehow in typical data), use of the
synthetic dimension makes chunk size selection simpler (most all values are
in coordinate position zero in the synthetic dimension).

Once loaded, the data in SciDB look like the following output,
where the 'quote' values are
displayed with the format "bid, bidsz, bidexch, ask, asksz, askexch,":
```
iquery -aq "scan(opraqte)"

{synth,exch,instrument_id,day,ms} quote,flag,feed,seqnum
{0,65,1,1412222400,1000} '3.00, 10, A, 5.00, 50, A,','67','2c',100
{0,65,1,1412222400,2000} '2.00, 20, A, 4.00, 10, A,','67','2c',200
{0,65,1,1412222400,3000} '1.00, 10, A, 5.00, 20, A,','67','2c',500
{0,65,1,1412222400,5000} '1.00, 20, A, 6.00, 11, A,','67','2c',1100
{0,65,2,1412222400,1000} '1.00, 10, A, 6.00, 10, A,','68','2c',300
{0,66,1,1412222400,3000} '2.00, 20, B, 6.00, 20, B,','67','2c',600
{0,66,1,1412222400,4000} '5.00, 10, B, 6.00, 10, B,','67','2c',1000
{0,67,1,1412222400,3000} '0.00, 10, C, 7.00, 30, C,','67','2c',700
{0,68,1,1412222400,3000} '3.00, 30, D, 7.00, 40, D,','67','2c',800
{0,69,1,1412222400,3000} '4.00, 40, E, 7.00, 50, E,','67','2c',900
{1,65,2,1412222400,1000} '1.00, 20, A, 4.00, 10, A,','67','2c',400
```
The formatting of the quote type display can be adjusted in the
quote.cpp source file.

We can count the collisions in the loaded data (among instrument_id 2 values)
with a redimension aggregate:
```
iquery -aq "
redimension(opraqte, 
  <count: uint64 null>
    [exch=0:255,256,0,instrument_id=0:*,2000,0,day=0:*,1,0,ms=0:86399999,600000,0],
  count(*) as count) "

{exch,instrument_id,day,ms} count
{65,1,1412222400,1000} 1
{65,1,1412222400,2000} 1
{65,1,1412222400,3000} 1
{65,1,1412222400,5000} 1
{65,2,1412222400,1000} 2
{66,1,1412222400,3000} 1
{66,1,1412222400,4000} 1
{67,1,1412222400,3000} 1
{68,1,1412222400,3000} 1
{69,1,1412222400,3000} 1
```

## Choosing best quote among data collisions

We can find the best quote among the data that collide and eliminate the synth
dimension with:
```
iquery -aq "
redimension(opraqte,
  <quote: quote null>
  [exch=0:255,256,0,instrument_id=0:*,2000,0,day=0:*,1,0,ms=0:86399999,600000,0],
  quote_best(quote) as quote) "

{exch,instrument_id,day,ms} quote
{65,1,1412222400,1000} '3.00, 10, A, 5.00, 50, A,'
{65,1,1412222400,2000} '2.00, 20, A, 4.00, 10, A,'
{65,1,1412222400,3000} '1.00, 10, A, 5.00, 20, A,'
{65,1,1412222400,5000} '1.00, 20, A, 6.00, 11, A,'
{65,2,1412222400,1000} '1.00, 20, A, 4.00, 10, A,'
{66,1,1412222400,3000} '2.00, 20, B, 6.00, 20, B,'
{66,1,1412222400,4000} '5.00, 10, B, 6.00, 10, B,'
{67,1,1412222400,3000} '0.00, 10, C, 7.00, 30, C,'
{68,1,1412222400,3000} '3.00, 30, D, 7.00, 40, D,'
{69,1,1412222400,3000} '4.00, 40, E, 7.00, 50, E,'
```

## Full NBBO computation

To compute NBBO:

1. Remove the synth dimension using the custom quote_best aggregate to
   determine the best quote among data collisions (if any).
2. Select a set of time points over the day to compute the nbbo at.
3. Build a lattice of nulls at the time points and merge in to the data array.
4. Use P4's last_value aggregate to find the last known best bid and offer on each
   exchange before each time point.
5. Use the custom quote_best aggregate to compute the nbbo at each time point.

Example (available in the nbbo script):
```
# Data array:
data="project(opraqte, quote)"

# Step 1.
# Aggregate out the synthetic dimension, computing best bid/ask prices over
# that dimension returning an array with axes [exch, instrumend_id, day, ms].
x="aggregate(opraqte, quote_best(quote) as quote, exch, instrument_id, day, ms)"


# Step 2. For each time interval, fill in null quote values for exchanges
# witout data.

# 2a: Construct a plane of null quote values along the ms and instrument axes
# If you are interested in a different set of time values than all the time
# values in the data itself, for example a subset of them, then change this
# array to reflect the time points of interest.
null_plane="apply(aggregate($data, count(*), instrument_id, ms), quote, quote(null))"

# 2b: Construct a lattice of nulls of the same shape as the data array
lattice="
project(
  cross_join(
    aggregate($data, count(quote), exch, day),
    $null_plane),
  quote)
"
# Unfortunately, lattice is no longer conformable with $x and we need to
# redimension it. I can't find a way around this
lattice="
redimension($lattice, 
            <quote: quote null>
            [exch=0:255,1,0,
             instrument_id=0:*,100,0,
             day=0:*,1,0,
             ms=0:86399999,600000,0])"

# 2c: Construct the fill-in array
fill="merge( $x, $lattice )"

# Step 3. Compute last-known best bid and ask prices at every time interval,
# filling in the null values introduced in step 2.
fill="variable_window($fill, ms, 1, 0, last_value(quote) as quote)"

# Step 4. Compute the nbbo across the exchanges
nbbo="aggregate($fill, quote_best(quote) as nbbo, instrument_id, day, ms)"

# Run this!
iquery -aq "$nbbo"
```

Output:
```
{instrument_id,day,ms} nbbo
{1,1412222400,1000} '3.00, 10, A, 5.00, 50, A,'
{1,1412222400,2000} '2.00, 20, A, 4.00, 10, A,'
{1,1412222400,3000} '4.00, 40, E, 5.00, 20, A,'
{1,1412222400,4000} '5.00, 10, B, 5.00, 20, A,'
{1,1412222400,5000} '5.00, 10, B, 6.00, 11, A,'
{2,1412222400,1000} '1.00, 10, A, 6.00, 10, A,'
```
Recall that the quote values display in the format bid, bidsz, bidexch, ask,
asksz, askexch, and compare with the expected output above.

**See the nbbo2 script for a more comprehensive real-world example.**

## Functions

The quotes plugin includes two experimental conversion functions. Each
conversion can be achieved using safer and more standard techniques in SciDB,
but these functions are geared only for performance. Very little error checking
is performed, use them on valid input data!

### int64_t tm2ms (string)
Convert a time value string into milliseconds from midnight. Example:
```
iquery -aq "apply( build(<time:string>[i=1:1,1,0],'09:30:01.456'), ms, tm2ms(time))"
{i} time,           ms
{1} '09:30:01.456', 34201456
```

### int64_t fastdate (string)
Quickly convert a date string in the format "YYYY-MM-DD" into an
order-preserving easy to read integer representation (suitable for use
as a SciDB coordinate axis). Example:
```
 iquery -aq "apply( build(<date:string>[i=1:1,1,0],'2014-05-12'), day, fastdate(date))"
{i} date,         day
{1} '2014-05-12', 20140512
```
