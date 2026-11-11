#pragma once
namespace ecology
{
inline int threads = 4;
// Starting bots are constructed only by the main thread. Descendants copy IDs.
inline unsigned nextFounder = 1;
}
