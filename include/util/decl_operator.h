#ifndef DECL_OPERATOR_H
#define DECL_OPERATOR_H

#define IMPL_CNT_PARAM(_14, _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0, cnt, ...) cnt

#define CNT_PARAM(...)  \
    IMPL_CNT_PARAM(, __VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#endif