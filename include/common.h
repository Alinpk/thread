#pragma once

#define DISALLOW_COPY(Typename) Typename(const Typename&) = delete;
#define DISALLOW_ASSIGN(Typename) Typename& operator=(const Typename&) = delete;
#define DISALLOW_MOVE(Typename) Typename(Typename&&) = delete;
#define DEFAULT_COPY(Typename) Typename(const Typaname&) = delete;

#define DISTALLOW_COPY_AND_ASSIGN(Typename) DISALLOW_ASSIGN(Typename)\
                                            DISALLOW_COPY(Typename);

#define DISTALLOW_COPY_AND_MOVE(Typename)   DISALLOW_ASSIGN(Typename)\
                                            DISALLOW_COPY(Typename)\
                                            DISALLOW_MOVE(Typename);