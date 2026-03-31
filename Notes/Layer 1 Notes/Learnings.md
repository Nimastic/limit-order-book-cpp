Bugs Encounted:

pointer invalidation bug
bids[o.price].push(o);
index[o.id] = &bids[o.price].back();

std::queue is backed by std::deque internally. When a deque grows and reallocates, all existing pointers into it are invalidated. So this index pointer could be pointing at garbage after enough orders are added to the same price level.
will corrupt memory unpredictably under load.

solution: used list, std::list nodes never move in memory so pointers stay valid forever.