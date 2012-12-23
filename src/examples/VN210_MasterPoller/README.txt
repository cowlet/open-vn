Copyright (C) 2012 University of Strathclyde

The Master Poller application generally shouldn't be run.  It was used during API 
development to verify the API functions, and is designed to be run on a second 
Arduino connected to an Arduino running the Simple API.  It implements a set of 
message examples listed in the Simple API specification, effectively running 
a set of unit tests comparing expected and actual received bytes from the 
Simple API application processor node.
