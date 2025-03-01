#include "domain.h"

namespace domain {

std::size_t StopDistanceHasher::operator()(const std::pair<const Stop *, const Stop *> &interval) const {
    std::hash<const void *> hasher;
    std::size_t h1 = hasher(interval.first);
    std::size_t h2 = hasher(interval.second);
    return combineHashes(std::move(h1), std::move(h2));
}

std::size_t StopDistanceHasher::combineHashes(const std::size_t h1, const std::size_t h2) const {
    return h1 ^ (h2 << 1);
}
}