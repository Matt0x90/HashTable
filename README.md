# HashTable

For this assignment I implemented a hash table with separate chaining. The table stores Bid objects. Each bucket is a head Node with a Bid, a key, and a next pointer; collisions form a linked list in that bucket. Buckets live in vector<Node> nodes.

I built the constructors, destructor, Insert, PrintAll, Remove, Search, SaveCSV, checkAndResize, and various other helper methods. The default table has M = 179 buckets. I added automatic resizing triggered when the bucket chain length ≥ 4, or when an insertion traverses many nodes in that bucket (my collision counter for that insert). On resize I double M and take the next prime to reduce clustering, then reinsert all items. Rebuild uses the size constructor and std::swap to replace internals.

Let N be the number of stored items and α = N/M the load factor.

Hash index computation: O(1).

Insert / Search / Remove: average O(1 + α), worst-case O(N) (everything in one bucket). With automatic resizing that keeps α bounded and rehashing to the next prime size, the expected time is close to constant (amortized O(1)).

PrintAll: O(N + M).

Resize: reinserts all items in O(N) time. Peak extra space O(N+M) until the old table is released.
