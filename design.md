`Bank` class

- deque of `Trans`
- deque of `User` 
- unordered_map of `User` pointer with key as id
- curTime uint64
- a pq of approved but not executed trans pointer, by ascending time
- a vector of executed trans pointer by ascending exec time
- 

`User` class

- logged in IP. need to be set
- a vector of incoming trans pointer
- a vector of outgoing trans pointer

`Trans` class

- time uint64
- sender (pointer)
- receiver (pointer)
- amount uint32