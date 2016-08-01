#ifndef LINK_H
#define LINK_H

#define addToLink(item, first, last) do {         \
  (item)->prev = (last);                          \
  if (last) {                                     \
    (last)->next = (item);                        \
  } else {                                        \
    (first) = (item);                             \
  }                                               \
  (last) = (item);                                \
} while (0)

#define removeFromLink(item, first, last) do {    \
  if ((item)->prev) {                             \
    (item)->prev->next = (item)->next;            \
  }                                               \
  if ((item)->next) {                             \
    (item)->next->prev = (item)->prev;            \
  }                                               \
  if ((item) == (first)) {                        \
    (first) = (item)->next;                       \
  }                                               \
  if ((item) == (last)) {                         \
    (last) = (item)->prev;                        \
  }                                               \
} while (0)

#endif
