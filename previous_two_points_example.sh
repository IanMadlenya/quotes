#!/bin/bash

# Example task: find the previous two data points in the 'data' array  along
# the i axis for each point in the 'seek' array, including the i coordinates of
# the points in seek.

data="build(<x:double null>[i=1:10,10,0],'{1}[(0),(0),(0),(0),(0),(),(0),(),(0)]',true)"
seek="build(<x:double null>[i=1:10,10,0],'{1}[( ),( ),( ),( ),( ),(),(0),(),( ),(0)]',true)"

# merge the arrays
q="merge($data, $seek)"

# Apply the i coordinate to a value called 'p'
q="apply($q, p, i)"

# Set 'p' to the previous coordinate value, changing names to i
q1="project(cross_join(variable_window($q, i, 1, 0, min(p) as p) as x, $seek as y, x.i, y.i),p)"

# Cross-join with seek and redimension, renaming dimension conformably with seek
q1="cast(redimension($q1, <i:int64>[p=1:10,10,0]), <_:int64>[i=1:10,10,0])"
q1="project(apply($q1, x, double(null)),x)"

# Same as above, but now find the next-to-last values
q2="project(cross_join(variable_window($q, i, 2, 0, min(p) as p) as x, $seek as y, x.i, y.i),p)"
q2="cast(redimension($q2, <i:int64>[p=1:10,10,0]), <_:int64>[i=1:10,10,0])"
q2="project(apply($q2, x, double(null)),x)"

# Merge the last and 2nd to last values in to seek
ans="merge(merge($seek, $q1), $q2)"

# The 'ans' array may now be cross_joined or merged with the data array to pick
# out the last two points before each specified point in see.
